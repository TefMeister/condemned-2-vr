/* bridge.c - see bridge.h. */
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bridge.h"

/* ---- settings ------------------------------------------------------------------------------------------------ */
#define MAX_VARS        256
#define MAX_CMDS        128
#define MAX_ARGS        32
#define LINE_MAX        2048
#define LOG_RING        256
#define LOG_LINE        256
#define SHOT_WAIT_MS    2500
#define ONFRAME_WAIT_MS 3000
#define MEM_MAX         4096
#define PIPE_BUF_SIZE   65536
#define LOG_DEFAULT_N   30      /* lines `log` returns when no count is given */
#define STOP_WAIT_MS    2000    /* how long bridge_stop waits for the pipe thread */

/* ---- state --------------------------------------------------------------------------------------------------- */
typedef struct {
    const char *name, *help;
    bridge_type type;
    void *ptr;
    size_t room;
    int writable;
    bridge_get_fn get;
    bridge_set_fn set;
    void *ctx;
} var_t;
typedef struct { const char *name, *help; bridge_cmd_fn fn; void *ctx; unsigned flags; } cmd_t;

struct bridge_reply { HANDLE pipe; char id[32]; int failed; int lines; };

static var_t g_vars[MAX_VARS];
static cmd_t g_cmds[MAX_CMDS];
static volatile LONG g_nvars, g_ncmds;
static CRITICAL_SECTION g_lock;                 /* the log ring and the registries' growth */
static volatile LONG g_started, g_stop;
static HANDLE g_thread;
static char g_host[64] = "host", g_hostver[32] = "?", g_api[32] = "?";
static HWND g_hwnd;
static int (*g_input)(const char *, char *, size_t);
static volatile LONG g_mem_ok;
static void (*g_log)(const char *);
static char g_ring[LOG_RING][LOG_LINE];
static volatile LONG g_ring_next, g_ring_count;
static volatile LONG g_frames, g_fps_x100, g_fps_frames, g_fps_tick;
static volatile LONG g_requests, g_shots;

/* the one pending picture */
static volatile LONG g_shot_wanted;
static HANDLE g_shot_event;
static BYTE *g_shot_buf;
static size_t g_shot_cap, g_shot_len;
static unsigned g_shot_w, g_shot_h, g_shot_stride, g_shot_flags;

/* the one pending on-frame job */
static struct { volatile LONG pending; cmd_t *cmd; int argc; char *argv[MAX_ARGS]; bridge_reply *r; HANDLE done; } g_job;

/* ---- logging ------------------------------------------------------------------------------------------------- */
static volatile LONG g_init_done;
static void ensure_init(void) {                 /* the lock may be needed before bridge_start (a host log line, a var) */
    if (InterlockedCompareExchange(&g_started, 1, 0) == 0) { InitializeCriticalSection(&g_lock); InterlockedExchange(&g_init_done, 1); }
    else while (!g_init_done) Sleep(0);
}
void bridge_log_line(const char *line) {
    if (!line) return;
    ensure_init();
    EnterCriticalSection(&g_lock);
    LONG i = g_ring_next;
    _snprintf(g_ring[i], LOG_LINE - 1, "%s", line);
    g_ring[i][LOG_LINE - 1] = 0;
    g_ring_next = (i + 1) % LOG_RING;
    if (g_ring_count < LOG_RING) g_ring_count++;
    LeaveCriticalSection(&g_lock);
}
static void blog(const char *fmt, ...) {
    char line[LOG_LINE];
    va_list a;
    va_start(a, fmt);
    _vsnprintf(line, sizeof line - 1, fmt, a);
    va_end(a);
    line[sizeof line - 1] = 0;
    if (g_log) g_log(line); else bridge_log_line(line);
}
void bridge_set_log(void (*fn)(const char *)) { g_log = fn; }

/* ---- replies ------------------------------------------------------------------------------------------------- */
static void pipe_write(HANDLE h, const void *p, size_t n) {
    const BYTE *b = (const BYTE *)p;
    while (n) {
        DWORD w = 0;
        if (!WriteFile(h, b, (DWORD)(n > 65536 ? 65536 : n), &w, NULL) || !w) return;
        b += w; n -= w;
    }
}
void bridge_reply_line(bridge_reply *r, const char *fmt, ...) {
    char text[LINE_MAX];
    va_list a;
    va_start(a, fmt);
    _vsnprintf(text, sizeof text - 1, fmt, a);
    va_end(a);
    text[sizeof text - 1] = 0;
    for (char *p = text; *p; ++p) if (*p == '\n' || *p == '\r') *p = ' ';
    char head[48];
    _snprintf(head, sizeof head, "%s + ", r->id);
    pipe_write(r->pipe, head, strlen(head));
    pipe_write(r->pipe, text, strlen(text));
    pipe_write(r->pipe, "\n", 1);
    r->lines++;
}
void bridge_reply_bytes(bridge_reply *r, const void *data, size_t n, const char *meta) {
    char head[LINE_MAX];
    _snprintf(head, sizeof head - 1, "%s # %lu %s\n", r->id, (unsigned long)n, meta ? meta : "");
    head[sizeof head - 1] = 0;
    pipe_write(r->pipe, head, strlen(head));
    pipe_write(r->pipe, data, n);
}
void bridge_reply_err(bridge_reply *r, const char *fmt, ...) {
    char text[LINE_MAX];
    va_list a;
    va_start(a, fmt);
    _vsnprintf(text, sizeof text - 1, fmt, a);
    va_end(a);
    text[sizeof text - 1] = 0;
    for (char *p = text; *p; ++p) if (*p == '\n' || *p == '\r') *p = ' ';
    char head[LINE_MAX + 48];
    _snprintf(head, sizeof head - 1, "%s . err %s\n", r->id, text);
    head[sizeof head - 1] = 0;
    pipe_write(r->pipe, head, strlen(head));
    r->failed = 1;
}
static void reply_ok(bridge_reply *r) {
    if (r->failed) return;
    char head[48];
    _snprintf(head, sizeof head, "%s . ok\n", r->id);
    pipe_write(r->pipe, head, strlen(head));
}

/* ---- registries ---------------------------------------------------------------------------------------------- */
static var_t *find_var(const char *name) {
    for (LONG i = 0; i < g_nvars; ++i) if (!_stricmp(g_vars[i].name, name)) return &g_vars[i];
    return NULL;
}
static cmd_t *find_cmd(const char *name) {
    for (LONG i = 0; i < g_ncmds; ++i) if (!_stricmp(g_cmds[i].name, name)) return &g_cmds[i];
    return NULL;
}
static int add_var(var_t v) {
    ensure_init();   /* registering before start is fine */
    EnterCriticalSection(&g_lock);
    int ok = 0;
    if (g_nvars < MAX_VARS && v.name && !find_var(v.name)) { g_vars[g_nvars] = v; InterlockedIncrement(&g_nvars); ok = 1; }
    LeaveCriticalSection(&g_lock);
    return ok;
}
int bridge_var(const char *name, bridge_type t, void *ptr, int writable, const char *help) {
    var_t v = { name, help ? help : "", t, ptr, 0, writable, NULL, NULL, NULL };
    return (t == BRIDGE_STR || t == BRIDGE_FN || !ptr) ? 0 : add_var(v);
}
int bridge_var_str(const char *name, char *buf, size_t room, int writable, const char *help) {
    var_t v = { name, help ? help : "", BRIDGE_STR, buf, room, writable, NULL, NULL, NULL };
    return (buf && room) ? add_var(v) : 0;
}
int bridge_var_fn(const char *name, bridge_get_fn get, bridge_set_fn set, void *ctx, const char *help) {
    var_t v = { name, help ? help : "", BRIDGE_FN, NULL, 0, set != NULL, get, set, ctx };
    return get ? add_var(v) : 0;
}
int bridge_cmd(const char *name, bridge_cmd_fn fn, void *ctx, unsigned flags, const char *help) {
    ensure_init();
    EnterCriticalSection(&g_lock);
    int ok = 0;
    if (g_ncmds < MAX_CMDS && name && fn && !find_cmd(name)) {
        cmd_t c = { name, help ? help : "", fn, ctx, flags };
        g_cmds[g_ncmds] = c;
        InterlockedIncrement(&g_ncmds);
        ok = 1;
    }
    LeaveCriticalSection(&g_lock);
    return ok;
}

static void var_text(const var_t *v, char *out, size_t room) {
    switch (v->type) {
    case BRIDGE_I32: _snprintf(out, room, "%ld", (long)*(volatile LONG *)v->ptr); break;
    case BRIDGE_U32: _snprintf(out, room, "%lu", (unsigned long)*(volatile ULONG *)v->ptr); break;
    case BRIDGE_F32: _snprintf(out, room, "%.6g", (double)*(volatile float *)v->ptr); break;
    case BRIDGE_F64: _snprintf(out, room, "%.10g", *(volatile double *)v->ptr); break;
    case BRIDGE_BOOL: _snprintf(out, room, "%s", *(volatile LONG *)v->ptr ? "1" : "0"); break;
    case BRIDGE_STR: _snprintf(out, room, "%s", (const char *)v->ptr); break;
    case BRIDGE_FN: out[0] = 0; v->get(out, room, v->ctx); break;
    }
    out[room - 1] = 0;
}
static int var_set(var_t *v, const char *text) {
    if (!v->writable) return -1;
    char *end;
    switch (v->type) {
    case BRIDGE_I32: { long x = strtol(text, &end, 0); if (end == text) return -1; InterlockedExchange((volatile LONG *)v->ptr, (LONG)x); return 0; }
    case BRIDGE_U32: { unsigned long x = strtoul(text, &end, 0); if (end == text) return -1; InterlockedExchange((volatile LONG *)v->ptr, (LONG)x); return 0; }
    case BRIDGE_F32: { float x = (float)strtod(text, &end); if (end == text) return -1; LONG bits; memcpy(&bits, &x, 4); InterlockedExchange((volatile LONG *)v->ptr, bits); return 0; }
    case BRIDGE_F64: { double x = strtod(text, &end); if (end == text) return -1; *(volatile double *)v->ptr = x; return 0; }
    case BRIDGE_BOOL: { int on = !_stricmp(text, "1") || !_stricmp(text, "on") || !_stricmp(text, "true") || !_stricmp(text, "yes");
                        int off = !_stricmp(text, "0") || !_stricmp(text, "off") || !_stricmp(text, "false") || !_stricmp(text, "no");
                        if (!on && !off) return -1; InterlockedExchange((volatile LONG *)v->ptr, on); return 0; }
    case BRIDGE_STR: _snprintf((char *)v->ptr, v->room - 1, "%s", text); ((char *)v->ptr)[v->room - 1] = 0; return 0;
    case BRIDGE_FN: return v->set ? v->set(text, v->ctx) : -1;
    }
    return -1;
}
static const char *type_name(bridge_type t) {
    static const char *n[] = { "i32", "u32", "f32", "f64", "bool", "str", "fn" };
    return n[t];
}
/* name=value, the value quoted when it is text (so a reader's name=value parser keeps its spaces) */
static void reply_var(bridge_reply *r, const var_t *v) {
    char val[LINE_MAX];
    var_text(v, val, sizeof val);
    if (v->type == BRIDGE_STR || v->type == BRIDGE_FN) bridge_reply_line(r, "%s=\"%s\"", v->name, val);
    else bridge_reply_line(r, "%s=%s", v->name, val);
}

/* ---- frames and pictures ------------------------------------------------------------------------------------- */
void bridge_frame_mark(void) {
    InterlockedIncrement(&g_frames);
    LONG f = InterlockedIncrement(&g_fps_frames);
    DWORD now = GetTickCount();
    DWORD since = now - (DWORD)g_fps_tick;
    if (since >= 1000) {
        InterlockedExchange(&g_fps_x100, (LONG)(f * 100000 / since));
        InterlockedExchange(&g_fps_frames, 0);
        InterlockedExchange(&g_fps_tick, (LONG)now);
    }
    if (g_job.pending && InterlockedCompareExchange(&g_job.pending, 2, 1) == 1) {
        g_job.cmd->fn(g_job.argc, (const char **)g_job.argv, g_job.r, g_job.cmd->ctx);
        SetEvent(g_job.done);
    }
}
unsigned bridge_frames(void) { return (unsigned)g_frames; }
float bridge_fps(void) { return g_fps_x100 / 100.0f; }
int bridge_shot_wanted(void) { return g_shot_wanted == 1; }
void bridge_shot_submit(unsigned w, unsigned h, unsigned stride, const void *bgra, unsigned flags) {
    if (InterlockedCompareExchange(&g_shot_wanted, 2, 1) != 1) return;   /* nobody is waiting */
    size_t need = (size_t)stride * h;
    if (need > g_shot_cap) { free(g_shot_buf); g_shot_buf = (BYTE *)malloc(need); g_shot_cap = g_shot_buf ? need : 0; }
    if (g_shot_buf) { memcpy(g_shot_buf, bgra, need); g_shot_len = need; }
    g_shot_w = w; g_shot_h = h; g_shot_stride = stride; g_shot_flags = flags;
    SetEvent(g_shot_event);
}

/* ---- built-in commands --------------------------------------------------------------------------------------- */
static void cmd_ping(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    bridge_reply_line(r, "pong %lu", (unsigned long)GetTickCount());
}
static void cmd_info(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    char exe[MAX_PATH] = "?", title[256] = "";
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    const char *base = strrchr(exe, '\\');
    if (g_hwnd) GetWindowTextA(g_hwnd, title, sizeof title);
    bridge_reply_line(r, "bridge=%s host=%s host_version=%s api=%s", BRIDGE_VERSION, g_host, g_hostver, g_api);
    bridge_reply_line(r, "pid=%lu exe=%s bits=%d", GetCurrentProcessId(), base ? base + 1 : exe, (int)(sizeof(void *) * 8));
    bridge_reply_line(r, "hwnd=0x%p title=\"%s\"", (void *)g_hwnd, title);
    bridge_reply_line(r, "frames=%ld fps=%.2f vars=%ld cmds=%ld input=%d mem=%d requests=%ld shots=%ld",
                      g_frames, bridge_fps(), g_nvars, g_ncmds, g_input != NULL, g_mem_ok != 0, g_requests, g_shots);
}
static void cmd_list(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    char val[LINE_MAX];
    for (LONG i = 0; i < g_nvars; ++i) {
        var_text(&g_vars[i], val, sizeof val);
        bridge_reply_line(r, "var %s %s %s %s | %s", g_vars[i].name, type_name(g_vars[i].type), g_vars[i].writable ? "rw" : "ro", val, g_vars[i].help);
    }
    for (LONG i = 0; i < g_ncmds; ++i) bridge_reply_line(r, "cmd %s%s | %s", g_cmds[i].name, (g_cmds[i].flags & BRIDGE_CMD_ON_FRAME) ? " (on frame)" : "", g_cmds[i].help);
}
static void cmd_get(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)ctx;
    if (argc < 2) { bridge_reply_err(r, "get <name> [<name>...]"); return; }
    for (int i = 1; i < argc; ++i) {
        var_t *v = find_var(argv[i]);
        if (!v) { bridge_reply_err(r, "no var called %s", argv[i]); return; }
        reply_var(r, v);
    }
}
static void cmd_set(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)ctx;
    if (argc < 3) { bridge_reply_err(r, "set <name> <value>"); return; }
    var_t *v = find_var(argv[1]);
    if (!v) { bridge_reply_err(r, "no var called %s", argv[1]); return; }
    if (!v->writable) { bridge_reply_err(r, "%s is read-only", v->name); return; }
    if (var_set(v, argv[2]) != 0) { bridge_reply_err(r, "%s refused the value %s", v->name, argv[2]); return; }
    reply_var(r, v);
}
static void cmd_log(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)ctx;
    int n = argc > 1 ? atoi(argv[1]) : LOG_DEFAULT_N;
    if (n < 1) n = 1;
    EnterCriticalSection(&g_lock);
    if (n > g_ring_count) n = g_ring_count;
    LONG start = (g_ring_next - n + LOG_RING) % LOG_RING;
    for (int i = 0; i < n; ++i) bridge_reply_line(r, "%s", g_ring[(start + i) % LOG_RING]);
    LeaveCriticalSection(&g_lock);
}
static void cmd_stats(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    bridge_reply_line(r, "frames=%ld fps=%.2f requests=%ld shots=%ld log_lines=%ld", g_frames, bridge_fps(), g_requests, g_shots, g_ring_count);
}
static void cmd_window(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    if (!g_hwnd) { bridge_reply_err(r, "the host has not told the bridge its window yet"); return; }
    RECT wr = { 0 }, cr = { 0 };
    POINT o = { 0, 0 };
    char title[256] = "";
    GetWindowRect(g_hwnd, &wr);
    GetClientRect(g_hwnd, &cr);
    ClientToScreen(g_hwnd, &o);
    GetWindowTextA(g_hwnd, title, sizeof title);
    HWND fg = GetForegroundWindow();
    bridge_reply_line(r, "hwnd=0x%p title=\"%s\" visible=%d minimized=%d in_front=%d", (void *)g_hwnd, title,
                      IsWindowVisible(g_hwnd), IsIconic(g_hwnd), fg == g_hwnd);
    bridge_reply_line(r, "window=%ld,%ld %ldx%ld client=%ld,%ld %ldx%ld screen=%dx%d", wr.left, wr.top,
                      wr.right - wr.left, wr.bottom - wr.top, o.x, o.y, cr.right - cr.left, cr.bottom - cr.top,
                      GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}
static void cmd_modules(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    HMODULE mods[512];
    DWORD need = 0;
    if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &need)) { bridge_reply_err(r, "EnumProcessModules failed"); return; }
    DWORD n = need / sizeof(HMODULE);
    if (n > 512) n = 512;
    for (DWORD i = 0; i < n; ++i) {
        char name[MAX_PATH] = "?";
        MODULEINFO mi = { 0 };
        GetModuleBaseNameA(GetCurrentProcess(), mods[i], name, MAX_PATH);
        GetModuleInformation(GetCurrentProcess(), mods[i], &mi, sizeof mi);
        bridge_reply_line(r, "%s base=0x%p size=0x%lx", name, (void *)mods[i], (unsigned long)mi.SizeOfImage);
    }
}
static void cmd_mem(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)ctx;
    if (!g_mem_ok) { bridge_reply_err(r, "memory reads are not allowed by this host (bridge_allow_mem)"); return; }
    if (argc < 2) { bridge_reply_err(r, "mem <hex address> [bytes<=4096]"); return; }
    ULONG_PTR addr = (ULONG_PTR)_strtoui64(argv[1], NULL, 16);
    unsigned n = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 0) : 64;
    if (n > MEM_MAX) n = MEM_MAX;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery((void *)addr, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        bridge_reply_err(r, "0x%p is not readable", (void *)addr); return;
    }
    ULONG_PTR region_end = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;
    if (addr + n > region_end) n = (unsigned)(region_end - addr);
    const BYTE *p = (const BYTE *)addr;
    char line[LINE_MAX];
    for (unsigned off = 0; off < n; off += 16) {
        int k = _snprintf(line, sizeof line, "%p ", (void *)(addr + off));
        for (unsigned j = 0; j < 16 && off + j < n; ++j) k += _snprintf(line + k, sizeof line - k, "%02x ", p[off + j]);
        bridge_reply_line(r, "%s", line);
    }
}
static void cmd_in(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)ctx;
    if (!g_input) { bridge_reply_err(r, "this host has no input feed (bridge_set_input)"); return; }
    if (argc < 2) { bridge_reply_err(r, "in <ooff command...>"); return; }
    char line[LINE_MAX] = "", reply[LINE_MAX];
    for (int i = 1; i < argc; ++i) { if (i > 1) strncat(line, " ", sizeof line - strlen(line) - 1); strncat(line, argv[i], sizeof line - strlen(line) - 1); }
    if (g_input(line, reply, sizeof reply) != 0) { bridge_reply_err(r, "%s", reply[0] ? reply : "input refused"); return; }
    if (reply[0]) bridge_reply_line(r, "%s", reply);
}
static void cmd_shot(int argc, const char **argv, bridge_reply *r, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    if (InterlockedCompareExchange(&g_shot_wanted, 1, 0) != 0) { bridge_reply_err(r, "a picture is already being taken"); return; }
    ResetEvent(g_shot_event);
    DWORD w = WaitForSingleObject(g_shot_event, SHOT_WAIT_MS);
    if (w != WAIT_OBJECT_0 || !g_shot_len) {
        InterlockedExchange(&g_shot_wanted, 0);
        bridge_reply_err(r, "no frame arrived in %d ms (is the game presenting, and does the host call bridge_shot_submit?)", SHOT_WAIT_MS);
        return;
    }
    char meta[128];
    _snprintf(meta, sizeof meta, "w=%u h=%u stride=%u fmt=bgra eyes=%d", g_shot_w, g_shot_h, g_shot_stride, (g_shot_flags & BRIDGE_SHOT_SBS) ? 2 : 1);
    bridge_reply_bytes(r, g_shot_buf, g_shot_len, meta);
    InterlockedIncrement(&g_shots);
    InterlockedExchange(&g_shot_wanted, 0);
}

/* ---- the pipe ------------------------------------------------------------------------------------------------ */
static int split_args(char *s, char **argv, int max) {
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') *s++ = 0;
        if (!*s) break;
        if (*s == '"') {
            argv[n++] = ++s;
            while (*s && *s != '"') ++s;
            if (*s) *s++ = 0;
        } else {
            argv[n++] = s;
            while (*s && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') ++s;
        }
    }
    return n;
}
static void serve_line(HANDLE pipe, char *line) {
    char *argv[MAX_ARGS + 1];
    int argc = split_args(line, argv, MAX_ARGS + 1);
    if (argc < 2) return;                                   /* no id, or no command: nothing to answer to */
    bridge_reply r = { pipe, "", 0, 0 };
    _snprintf(r.id, sizeof r.id - 1, "%s", argv[0]);
    InterlockedIncrement(&g_requests);
    cmd_t *c = find_cmd(argv[1]);
    if (!c) { bridge_reply_err(&r, "unknown command %s (try list)", argv[1]); return; }
    if (c->flags & BRIDGE_CMD_ON_FRAME) {
        if (InterlockedCompareExchange(&g_job.pending, 1, 0) != 0) { bridge_reply_err(&r, "another on-frame command is still running"); return; }
        g_job.cmd = c; g_job.argc = argc - 1; g_job.r = &r;
        for (int i = 0; i < argc - 1; ++i) g_job.argv[i] = argv[i + 1];
        ResetEvent(g_job.done);
        if (WaitForSingleObject(g_job.done, ONFRAME_WAIT_MS) != WAIT_OBJECT_0) {
            if (InterlockedCompareExchange(&g_job.pending, 0, 1) == 1) { bridge_reply_err(&r, "no frame came in %d ms to run %s on", ONFRAME_WAIT_MS, c->name); return; }
            WaitForSingleObject(g_job.done, INFINITE);     /* it started late: let it finish before we leave */
        }
        InterlockedExchange(&g_job.pending, 0);
    } else {
        c->fn(argc - 1, (const char **)argv + 1, &r, c->ctx);
    }
    reply_ok(&r);
}
static DWORD WINAPI pipe_thread(LPVOID arg) {
    (void)arg;
    WCHAR name[128];
    _snwprintf(name, 128, L"\\\\.\\pipe\\mcp-game-bridge-%lu", GetCurrentProcessId());
    blog("bridge %s listening on pipe mcp-game-bridge-%lu (host %s %s)", BRIDGE_VERSION, GetCurrentProcessId(), g_host, g_hostver);
    char *buf = (char *)malloc(LINE_MAX * 4);
    while (!g_stop) {
        HANDLE h = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1,
                                    PIPE_BUF_SIZE, PIPE_BUF_SIZE, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) { blog("bridge: the pipe could not be made (error %lu)", GetLastError()); break; }
        if (ConnectNamedPipe(h, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            size_t have = 0;
            DWORD n;
            while (!g_stop && ReadFile(h, buf + have, (DWORD)(LINE_MAX * 4 - have - 1), &n, NULL) && n) {
                have += n;
                buf[have] = 0;
                char *start = buf, *nl;
                while ((nl = strchr(start, '\n')) != NULL) {
                    *nl = 0;
                    serve_line(h, start);
                    start = nl + 1;
                }
                have = strlen(start);
                memmove(buf, start, have + 1);
                if (have >= LINE_MAX * 4 - 2) have = 0;      /* a line that never ends: drop it */
            }
            FlushFileBuffers(h);
        }
        DisconnectNamedPipe(h);
        CloseHandle(h);
    }
    free(buf);
    return 0;
}

/* ---- life ---------------------------------------------------------------------------------------------------- */
int bridge_start(const char *host_name, const char *host_version) {
    ensure_init();
    if (g_thread) return 1;
    if (host_name) _snprintf(g_host, sizeof g_host - 1, "%s", host_name);
    if (host_version) _snprintf(g_hostver, sizeof g_hostver - 1, "%s", host_version);
    g_shot_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    g_job.done = CreateEventA(NULL, TRUE, FALSE, NULL);
    g_fps_tick = (LONG)GetTickCount();
    bridge_cmd("ping", cmd_ping, NULL, 0, "are you there -> pong <tick>");
    bridge_cmd("info", cmd_info, NULL, 0, "who and what this game is");
    bridge_cmd("list", cmd_list, NULL, 0, "every var and command this game answers");
    bridge_cmd("get", cmd_get, NULL, 0, "get <name>... -> name=value");
    bridge_cmd("set", cmd_set, NULL, 0, "set <name> <value>");
    bridge_cmd("shot", cmd_shot, NULL, 0, "the current frame as raw BGRA (binary reply)");
    bridge_cmd("log", cmd_log, NULL, 0, "log [n] -> the last n host log lines");
    bridge_cmd("stats", cmd_stats, NULL, 0, "frames, fps, requests");
    bridge_cmd("window", cmd_window, NULL, 0, "the game window: title, place, size, in front");
    bridge_cmd("modules", cmd_modules, NULL, 0, "loaded modules with base and size");
    bridge_cmd("mem", cmd_mem, NULL, 0, "mem <hex addr> [n]: hex dump (host must allow it)");
    bridge_cmd("in", cmd_in, NULL, 0, "in <key w 1 | tap enter | text .. | move dx dy | click | pad ..>: feed input");
    g_thread = CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL);
    return g_thread != NULL;
}
void bridge_stop(void) {
    InterlockedExchange(&g_stop, 1);
    if (g_thread) {
        WCHAR name[128];                                    /* wake ConnectNamedPipe by connecting once */
        _snwprintf(name, 128, L"\\\\.\\pipe\\mcp-game-bridge-%lu", GetCurrentProcessId());
        HANDLE h = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
        WaitForSingleObject(g_thread, STOP_WAIT_MS);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
}
int bridge_running(void) { return g_thread != NULL && !g_stop; }
void bridge_set_api(const char *api) { if (api) _snprintf(g_api, sizeof g_api - 1, "%s", api); }
void bridge_set_window(HWND hwnd) { g_hwnd = hwnd; }
void bridge_set_input(int (*command)(const char *, char *, size_t)) { g_input = command; }
void bridge_allow_mem(int on) { InterlockedExchange(&g_mem_ok, on != 0); }
