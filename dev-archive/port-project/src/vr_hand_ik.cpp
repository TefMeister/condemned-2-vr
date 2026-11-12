// Condemned 2 VR: the hands on the motion controllers, step 3: the game's own arm IK, looked at live (2026-11-12).
//
// The reader's pass (modding-notes/2026-11-12c-the-games-own-arm-ik.md) found a two-bone IK chain class in the game
// (LithTech Jupiter EX, database category "Character/Models/IKChain": RootNode, JointNode, TargetNode,
// JointTargetDist, DownJointSign). Every character's client object has a manager at +2264 with TWO chain slots of
// 216 bytes (manager+8, +224). A model key "IK ON <chain> SyncTarget|Weapon <socket> [blend]" / "IK OFF [blend]"
// (sub_82396D30) fills a slot and sub_823966F8 sets it up: node handles, bone lengths, and three node-control
// callbacks registered with ILTModel (*(0x829643F0), vtable +96): root sub_82396130, joint sub_82395E68, target
// sub_823957E8, user data = the chain. The root callback reads the target each frame: when the chain's target object
// (+56) is set it asks ILTModel for that object's socket transform (vtable +12) and stores position at +64 and
// rotation at +76; WHEN THE TARGET OBJECT IS 0 IT SOLVES TOWARDS WHATEVER +64 / +76 ALREADY HOLD. That is the way in.
//
// This file, first build (a probe; everything [compile-verified] at most until it has run):
//  1. logs every IK key (its words), every chain set-up (nodes, lengths) and every chain the root callback solves;
//  2. ik.force=1 (bridge) makes every solved chain reach for the world point ik.tx / ik.ty / ik.tz instead of its
//     own target (the target object is hidden from the callback for that one call, then put back);
//  3. starts the MCP Game Bridge (C2_BRIDGE=1) with guest tools: `gmem <guest addr> [n]`, `gcall <guest fn> [r3..r8]`
//     (run at frame end on the game's thread), `gstr <text>` (a guest copy of a string, for name lookups), `gbuf`
//     (a 4 KB guest scratch block for out-parameters), `ik` (what has been seen).
// The frame-end hook (vr_camera_probe.cpp, FlipScreen) calls C2HandsOnFlip.

#include "generated/default/condemned2recomp_pch.h"

#include <windows.h>

#include <rex/hook.h>
#include <rex/system/kernel_state.h>
#include <rex/logging/macros.h>

extern "C" {
#include "bridge/bridge.h"
}

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

DECLARE_REX_FUNC(sub_82396D30);   // the "IK ..." model key: r3 = manager, r4 = model, r5 = tracker byte, r6 = the words
DECLARE_REX_FUNC(sub_823966F8);   // chain set-up: r3 = chain, r4 = model, r5 = database record
DECLARE_REX_FUNC(sub_82396130);   // root node callback: r3 = node control data, r4 = chain

void C2HandsOnFlip(PPCContext& ctx, uint8_t* base);

namespace {

// the chain's layout (reader, 2026-11-12)
constexpr uint32_t kChModel = 12u, kChRecord = 16u, kChTargetObj = 56u, kChTargetSocket = 60u;
constexpr uint32_t kChTargetPos = 64u, kChTargetRot = 76u;
constexpr uint32_t kChNodeRoot = 92u, kChNodeJoint = 96u, kChNodeTarget = 100u;
constexpr uint32_t kChLenUpper = 104u, kChLenLower = 108u, kChJointDist = 112u, kChJointSign = 116u, kChWeight = 208u;
constexpr int kSeenMax = 16;
constexpr uint32_t kScratchBytes = 4096u, kStringRoom = 256u;

bool EnvOn(const char* n, bool d) { const char* e = std::getenv(n); return e ? e[0] == '1' : d; }

inline float LoadF(uint8_t* base, uint32_t a) {
  const uint32_t bits = REX_LOAD_U32(a);
  float v;
  std::memcpy(&v, &bits, sizeof(v));
  return v;
}
inline void StoreF(uint8_t* base, uint32_t a, float v) {
  uint32_t bits;
  std::memcpy(&bits, &v, sizeof(bits));
  REX_STORE_U32(a, bits);
}

struct Seen {
  uint32_t chain = 0, model = 0, target_obj = 0;
  uint64_t solves = 0;
  float weight = 0, pos[3] = {};
};
std::mutex g_mu;
Seen g_seen[kSeenMax];
int g_seen_n = 0;
std::atomic<int> g_lines{0};

// bridge dials
int32_t g_force = 0;
float g_target[3] = {0, 0, 0};
int32_t g_only = 0;                 // 0 = every chain, else only this chain address

// the game's thread at frame end, for gcall
thread_local PPCContext* t_ctx = nullptr;
thread_local uint8_t* t_base = nullptr;
uint32_t g_scratch = 0;

void Say(const std::string& s) {
  REXLOG_INFO("[C2IK] {}", s);
  bridge_log_line(("[C2IK] " + s).c_str());
}

uint32_t Scratch() {
  if (g_scratch == 0) g_scratch = rex::system::kernel_memory()->SystemHeapAlloc(kScratchBytes);
  return g_scratch;
}

void CmdGmem(int argc, const char** argv, bridge_reply* r, void*) {
  uint8_t* base = t_base;
  if (base == nullptr || argc < 2) { bridge_reply_err(r, "gmem <guest addr hex> [bytes]"); return; }
  const uint32_t a = static_cast<uint32_t>(std::strtoul(argv[1], nullptr, 16));
  uint32_t n = argc > 2 ? static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 0)) : 64u;
  if (n > 1024u) n = 1024u;
  if (a < 0x10000u) { bridge_reply_err(r, "address too low"); return; }
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(base + a, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    bridge_reply_err(r, "not readable");
    return;
  }
  for (uint32_t o = 0; o < n; o += 16u) {
    char line[160];
    int k = std::snprintf(line, sizeof(line), "%08X:", a + o);
    for (uint32_t i = 0; i < 16u && o + i < n; i += 4u) {
      k += std::snprintf(line + k, sizeof(line) - k, " %08X", REX_LOAD_U32(a + o + i));
    }
    k += std::snprintf(line + k, sizeof(line) - k, "  |");
    for (uint32_t i = 0; i < 16u && o + i < n; i += 4u) {
      k += std::snprintf(line + k, sizeof(line) - k, " %.4g", static_cast<double>(LoadF(base, a + o + i)));
    }
    bridge_reply_line(r, "%s", line);
  }
}

void CmdGstr(int argc, const char** argv, bridge_reply* r, void*) {
  uint8_t* base = t_base;
  if (base == nullptr || argc < 2 || Scratch() == 0) { bridge_reply_err(r, "gstr <text>"); return; }
  const size_t n = std::strlen(argv[1]);
  if (n >= kStringRoom) { bridge_reply_err(r, "too long"); return; }
  const uint32_t at = Scratch() + kScratchBytes - kStringRoom;
  std::memcpy(base + at, argv[1], n + 1);
  bridge_reply_line(r, "%08X", at);
}

void CmdGbuf(int, const char**, bridge_reply* r, void*) {
  uint8_t* base = t_base;
  if (base == nullptr || Scratch() == 0) { bridge_reply_err(r, "no scratch"); return; }
  std::memset(base + Scratch(), 0, kScratchBytes - kStringRoom);
  bridge_reply_line(r, "%08X", Scratch());
}

// gcall <guest fn hex> [r3 r4 r5 r6 r7 r8, hex]: one call on the game's thread; answers r3 and f1
void CmdGcall(int argc, const char** argv, bridge_reply* r, void*) {
  if (t_ctx == nullptr || argc < 2) { bridge_reply_err(r, "gcall <guest fn hex> [r3..r8 hex]"); return; }
  PPCContext& ctx = *t_ctx;
  uint8_t* base = t_base;
  const uint32_t fn = static_cast<uint32_t>(std::strtoul(argv[1], nullptr, 16));
  if (fn < 0x82000000u || fn >= 0x82A00000u) { bridge_reply_err(r, "not a game function address"); return; }
  const PPCContext saved = ctx;
  PPCRegister* regs[6] = {&ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8};
  for (int i = 0; i < 6; ++i) regs[i]->u64 = (argc > 2 + i) ? std::strtoul(argv[2 + i], nullptr, 16) : 0u;
  ctx.r1.u64 = saved.r1.u64 - 1024u;   // well below the hooked function's frame
  ctx.lr = 0;
  REX_CALL_INDIRECT_FUNC(fn);
  const uint32_t r3 = ctx.r3.u32;
  const double f1 = ctx.f1.f64;
  ctx = saved;
  bridge_reply_line(r, "r3=%08X f1=%g", r3, f1);
}

void CmdIk(int, const char**, bridge_reply* r, void*) {
  uint8_t* base = t_base;
  std::lock_guard<std::mutex> lk(g_mu);
  bridge_reply_line(r, "ILTModel=%08X chains seen=%d force=%d", base ? REX_LOAD_U32(0x829643F0u) : 0u, g_seen_n, g_force);
  for (int i = 0; i < g_seen_n; ++i) {
    const Seen& s = g_seen[i];
    bridge_reply_line(r, "chain %08X model %08X target obj %08X weight %.2f solves %llu last target %.1f %.1f %.1f", s.chain, s.model,
                      s.target_obj, s.weight, static_cast<unsigned long long>(s.solves), s.pos[0], s.pos[1], s.pos[2]);
  }
}

void StartBridge() {
  static bool tried = false;
  if (tried) return;
  tried = true;
  if (!EnvOn("C2_BRIDGE", false)) return;
  bridge_set_api("d3d12 (ReXGlue xenos, Xbox 360 recompilation)");
  bridge_allow_mem(1);
  bridge_var("ik.force", BRIDGE_I32, &g_force, 1, "1 = every solved IK chain reaches for ik.tx / ik.ty / ik.tz (world units)");
  bridge_var("ik.tx", BRIDGE_F32, &g_target[0], 1, "forced IK target, world x");
  bridge_var("ik.ty", BRIDGE_F32, &g_target[1], 1, "forced IK target, world y");
  bridge_var("ik.tz", BRIDGE_F32, &g_target[2], 1, "forced IK target, world z");
  bridge_var("ik.only", BRIDGE_I32, &g_only, 1, "0 = force every chain, else only the chain at this address");
  bridge_cmd("gmem", CmdGmem, nullptr, BRIDGE_CMD_ON_FRAME, "guest memory as words and floats: gmem <guest addr hex> [bytes]");
  bridge_cmd("gcall", CmdGcall, nullptr, BRIDGE_CMD_ON_FRAME, "call a game function at frame end: gcall <fn hex> [r3..r8 hex]");
  bridge_cmd("gstr", CmdGstr, nullptr, BRIDGE_CMD_ON_FRAME, "put a text into guest memory, answers its address: gstr <text>");
  bridge_cmd("gbuf", CmdGbuf, nullptr, BRIDGE_CMD_ON_FRAME, "a zeroed guest scratch block (out-parameters), answers its address");
  bridge_cmd("ik", CmdIk, nullptr, BRIDGE_CMD_ON_FRAME, "the IK chains seen so far");
  if (bridge_start("condemned-2-vr", "b004-probe")) REXLOG_INFO("[C2IK] bridge up");
}

std::string GuestWord(uint8_t* base, uint32_t p) {
  if (p < 0x10000u) return "(null)";
  const char* s = reinterpret_cast<const char*>(base + p);
  return std::string(s, strnlen(s, 48));
}

}  // namespace

void C2HandsOnFlip(PPCContext& ctx, uint8_t* base) {
  StartBridge();
  if (!bridge_running()) return;
  t_ctx = &ctx;
  t_base = base;
  bridge_frame_mark();
  t_ctx = nullptr;
}

REX_HOOK_RAW(sub_82396D30) {
  const uint32_t mgr = ctx.r3.u32, model = ctx.r4.u32, tracker = ctx.r5.u32 & 0xFFu, words = ctx.r6.u32;
  if (g_lines.fetch_add(1) < 400 && words != 0) {
    const uint32_t n = REX_LOAD_U32(words);
    std::string line;
    for (uint32_t i = 0; i < n && i < 8u; ++i) line += " " + GuestWord(base, REX_LOAD_U32(words + 12u + 8u * i + 4u));
    char head[128];
    std::snprintf(head, sizeof(head), "key: manager %08X (owner %08X) model %08X tracker %u, %u words:", mgr, REX_LOAD_U32(mgr), model, tracker, n);
    Say(head + line);
  }
  __imp__sub_82396D30(ctx, base);
}

REX_HOOK_RAW(sub_823966F8) {
  const uint32_t chain = ctx.r3.u32, model = ctx.r4.u32, record = ctx.r5.u32;
  __imp__sub_823966F8(ctx, base);
  if (g_lines.fetch_add(1) < 400) {
    char line[256];
    std::snprintf(line, sizeof(line), "set-up: chain %08X model %08X record %08X nodes root %u joint %u target %u, lengths %.2f %.2f, joint dist %.1f sign %.0f",
                  chain, model, record, REX_LOAD_U32(chain + kChNodeRoot), REX_LOAD_U32(chain + kChNodeJoint), REX_LOAD_U32(chain + kChNodeTarget),
                  static_cast<double>(LoadF(base, chain + kChLenUpper)), static_cast<double>(LoadF(base, chain + kChLenLower)),
                  static_cast<double>(LoadF(base, chain + kChJointDist)), static_cast<double>(LoadF(base, chain + kChJointSign)));
    Say(line);
  }
}

REX_HOOK_RAW(sub_82396130) {
  const uint32_t chain = ctx.r4.u32;
  const uint32_t obj = REX_LOAD_U32(chain + kChTargetObj);
  const bool force = g_force != 0 && (g_only == 0 || static_cast<uint32_t>(g_only) == chain);
  if (force) {
    REX_STORE_U32(chain + kChTargetObj, 0u);   // the callback then solves towards +64 as it stands
    for (uint32_t i = 0; i < 3u; ++i) StoreF(base, chain + kChTargetPos + 4u * i, g_target[i]);
  }
  __imp__sub_82396130(ctx, base);
  if (force) REX_STORE_U32(chain + kChTargetObj, obj);
  std::lock_guard<std::mutex> lk(g_mu);
  int at = -1;
  for (int i = 0; i < g_seen_n; ++i) if (g_seen[i].chain == chain) at = i;
  if (at < 0 && g_seen_n < kSeenMax) {
    at = g_seen_n++;
    g_seen[at].chain = chain;
    char line[160];
    std::snprintf(line, sizeof(line), "solving: chain %08X model %08X target obj %08X socket %d weight %.2f", chain, REX_LOAD_U32(chain + kChModel),
                  obj, static_cast<int32_t>(REX_LOAD_U32(chain + kChTargetSocket)), static_cast<double>(LoadF(base, chain + kChWeight)));
    Say(line);
  }
  if (at >= 0) {
    Seen& s = g_seen[at];
    s.model = REX_LOAD_U32(chain + kChModel);
    s.target_obj = obj;
    s.weight = LoadF(base, chain + kChWeight);
    ++s.solves;
    for (uint32_t i = 0; i < 3u; ++i) s.pos[i] = LoadF(base, chain + kChTargetPos + 4u * i);
  }
}
