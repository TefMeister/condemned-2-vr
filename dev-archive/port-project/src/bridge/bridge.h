#pragma once
/* MCP Game Bridge³ - the in-game half.
 *
 * Compiled into a mod (a proxy DLL, a plugin, anything that runs inside the game), it opens a named pipe
 * \\.\pipe\mcp-game-bridge-<pid> and answers a small text protocol, so a program on the PC (the MCP server in
 * server/) can ask the running game what it is doing, read and set the values the mod chose to expose, run the mod's
 * commands, take a picture of the frame, read the mod's log, and feed keys, mouse and pad (through 0ut-0f-f0cus).
 *
 * Nothing in here knows any one game. What a game can answer is whatever its mod registers:
 *     bridge_var("head.yaw", BRIDGE_F32, &g_yaw, 0, "headset yaw, degrees");
 *     bridge_cmd("console", console_cmd, NULL, 0, "run a console command: console <text>");
 * so the bridge grows with every mod, one registration at a time. A few things every host gets for free: ping,
 * info, list, get, set, call, shot, log, stats, window, modules, mem (opt-in), in (input).
 *
 * Threads: the pipe is served on its own thread. Vars are read and written in place (32-bit values are atomic; a
 * 64-bit value may tear: make it a BRIDGE_FN var if that matters). A command flagged BRIDGE_CMD_ON_FRAME is run from
 * bridge_frame_mark(), i.e. on the thread that presents frames, and the pipe thread waits for it.
 *
 * Protocol (one request per line, UTF-8):  <id> <command> [args...]      quotes "..." keep spaces together
 *   replies:  <id> + <text line>            zero or more
 *             <id> # <nbytes> <meta words>  then exactly nbytes of binary (a picture: w= h= stride= fmt=bgra eyes=)
 *             <id> . ok | <id> . err <why>  always last */
#include <windows.h>
#include <stddef.h>

#define BRIDGE_VERSION "0.1.0"

typedef enum { BRIDGE_I32, BRIDGE_U32, BRIDGE_F32, BRIDGE_F64, BRIDGE_BOOL, BRIDGE_STR, BRIDGE_FN } bridge_type;

typedef struct bridge_reply bridge_reply;       /* where a command's answer goes */
typedef void (*bridge_cmd_fn)(int argc, const char **argv, bridge_reply *r, void *ctx);
typedef void (*bridge_get_fn)(char *out, size_t room, void *ctx);          /* text of the value */
typedef int (*bridge_set_fn)(const char *text, void *ctx);                 /* 0 ok, -1 refused */

enum { BRIDGE_CMD_ON_FRAME = 1 };               /* run the command from bridge_frame_mark() */
enum { BRIDGE_SHOT_SBS = 1 };                   /* the picture holds two eyes side by side */

/* ---- life ---- */
int bridge_start(const char *host_name, const char *host_version);   /* opens the pipe thread; 1 = running */
void bridge_stop(void);
int bridge_running(void);
void bridge_set_api(const char *api);           /* "d3d9", "d3d11", "vulkan" ... shown by info */
void bridge_set_window(HWND hwnd);              /* the game's window (info, window) */
void bridge_set_input(int (*command)(const char *line, char *reply, size_t room));  /* ooff_command, or your own */
void bridge_allow_mem(int on);                  /* lets `mem <addr> <n>` read this process's memory */
void bridge_set_log(void (*fn)(const char *line));   /* the bridge's own lines (default: the ring only) */
void bridge_log_line(const char *line);         /* mirror a host log line into the ring `log` answers from */

/* ---- what the game can answer ---- */
int bridge_var(const char *name, bridge_type t, void *ptr, int writable, const char *help);
int bridge_var_str(const char *name, char *buf, size_t room, int writable, const char *help);
int bridge_var_fn(const char *name, bridge_get_fn get, bridge_set_fn set, void *ctx, const char *help);
int bridge_cmd(const char *name, bridge_cmd_fn fn, void *ctx, unsigned flags, const char *help);

/* ---- frames and pictures (call from the thread that presents) ---- */
void bridge_frame_mark(void);                   /* once per presented frame: fps, on-frame commands */
int bridge_shot_wanted(void);                   /* 1 while a `shot` is waiting for bridge_shot_submit */
void bridge_shot_submit(unsigned w, unsigned h, unsigned stride, const void *bgra, unsigned flags);
unsigned bridge_frames(void);
float bridge_fps(void);

/* ---- answering, inside a command ---- */
void bridge_reply_line(bridge_reply *r, const char *fmt, ...);
void bridge_reply_bytes(bridge_reply *r, const void *data, size_t n, const char *meta);
void bridge_reply_err(bridge_reply *r, const char *fmt, ...);   /* the command's status becomes err */
