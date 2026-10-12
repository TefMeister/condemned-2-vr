// Condemned 2 VR probe, step 1: find the main camera, then (optionally) move it per eye.
//
// Static findings this rests on (reader, 2026-10-12, all [inferred-static] until a run):
//
//  * Every 3D view the engine draws goes through sub_8212BAB0 ("set view"), which copies the
//    camera into a view block and then calls sub_8212B010 (its only caller) to build the
//    matrices. Four call sites, all through the weak symbol, so a strong definition here
//    sees every call.
//      r3 = render state RS (0x82925950)        r4 = destination view block V
//      r5 = camera transform: pos xyz + rotation quaternion xyzw (7 floats, caller-owned)
//      r6 = FOV pair (2 floats -> V+48/52)      r7 = 2 floats -> V+56/60 (projection offset)
//      r8 = viewport rect (4 ints -> V+4..16)   r9 = mode -> V+0 (0 perspective, 1 ortho, 2 screen)
//      f1 = near, f2 = far (-> V+64 / V+68)
//    View block: +20 pos, +32 quat, +72 world->camera (3x4), +120 projection, +184
//    world->clip, +248 world->screen. The "current" view is V = RS+8 = 0x82925958; a second
//    block at RS+416 = 0x82925AF0 is used by sub_821474C8 (a save/render/restore pass).
//  * The quaternion is the camera's orientation: sub_8212B7F0 uploads q*(0,0,1) as
//    k_vWorldSpaceCameraDir. So right = q*(1,0,0) (LithTech: +X right, +Y up, +Z forward).
//  * Frame end: ILTRenderer::FlipScreen = sub_82122EA8 (vtable 0x82091E28 slot 9), which
//    calls sub_8265F2B0 (D3D swap) -> VdSwap at 0x8265F4FC. A second VdSwap path,
//    sub_82671908 -> sub_8265F2B0, presents while the game waits on something; it is not a
//    new picture and is counted separately.
//
// What this file does:
//  1. Logs every DISTINCT view set (caller, block, mode, rect, FOV, near, far) with a count,
//     and once per 600 flips how many views per frame, so a run shows which call is the
//     player's camera. Read-only unless C2_CAM_EYE is set.
//  2. C2_CAM_EYE=<units> moves the camera along its own right axis before the view is built,
//     for views matching the filter (default: block RS+8, perspective). The caller's own
//     transform buffer is restored afterwards, because it may be the camera object itself.
//     C2_CAM_EYE_LR=<hex> narrows it to one caller (use the lr the log prints for the player).
//  3. C2_STEREO_PERIOD=N flips the sign every N flips (0 = fixed eye, +e = right eye).
//
// Not here yet (on purpose): holding the world still for the second eye, and side-by-side.
// Those come after a run shows which view is the player's and how many sets there are per frame.

#include "generated/default/condemned2recomp_pch.h"

#include <rex/hook.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

DECLARE_REX_FUNC(sub_8212BAB0);
DECLARE_REX_FUNC(sub_82122EA8);

namespace {

constexpr uint32_t kRenderState = 0x82925950;
constexpr uint32_t kCurrentView = kRenderState + 8;    // 0x82925958
constexpr uint32_t kModePerspective = 0;
constexpr uint64_t kReportEveryFlips = 600;
constexpr int kMaxSignatures = 32;
constexpr int kMaxLoggedPerSignature = 3;

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

struct Settings {
  float cam_eye = 0.0f;
  uint32_t only_lr = 0;
  int period = 0;
};

const Settings& Get() {
  static const Settings s = [] {
    Settings r;
    if (const char* v = std::getenv("C2_CAM_EYE")) r.cam_eye = static_cast<float>(std::atof(v));
    if (const char* v = std::getenv("C2_CAM_EYE_LR")) r.only_lr = static_cast<uint32_t>(std::strtoul(v, nullptr, 16));
    if (const char* v = std::getenv("C2_STEREO_PERIOD")) r.period = std::atoi(v);
    REXGPU_INFO("[C2VR] settings cam_eye={} only_lr=0x{:08X} period={}", r.cam_eye, r.only_lr, r.period);
    return r;
  }();
  return s;
}

struct Signature {
  uint32_t lr, view, mode;
  int32_t rect[4];
  float fov[2], nearz, farz;
  uint64_t count;
  int logged;
};

Signature g_sigs[kMaxSignatures];
int g_nsigs = 0;
uint64_t g_flips = 0;
float g_sign = 1.0f;
uint32_t g_views_this_frame = 0, g_shifted_this_frame = 0;
uint32_t g_views_hist[6] = {}, g_shifted_hist[6] = {};

bool SameSig(const Signature& a, const Signature& b) {
  return a.lr == b.lr && a.view == b.view && a.mode == b.mode &&
         std::memcmp(a.rect, b.rect, sizeof(a.rect)) == 0 && a.fov[0] == b.fov[0] &&
         a.fov[1] == b.fov[1] && a.nearz == b.nearz && a.farz == b.farz;
}

}  // namespace

// The frame end. Counts frames, flips the eye, prints the per-frame histograms.
REX_HOOK_RAW(sub_82122EA8) {
  __imp__sub_82122EA8(ctx, base);
  if (ctx.r3.u32 == 0) return;  // returned 0: not in a state to present, nothing shown

  static bool thread_logged = false;
  if (!thread_logged) {
    thread_logged = true;
    REXGPU_INFO("[C2VR] flip thread={}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }

  ++g_flips;
  g_views_hist[g_views_this_frame < 5 ? g_views_this_frame : 5]++;
  g_shifted_hist[g_shifted_this_frame < 5 ? g_shifted_this_frame : 5]++;
  g_views_this_frame = g_shifted_this_frame = 0;

  const Settings& s = Get();
  if (s.period > 0 && (g_flips % static_cast<uint64_t>(s.period)) == 0) g_sign = -g_sign;

  if ((g_flips % kReportEveryFlips) == 0) {
    REXGPU_INFO("[C2VR] flips={} views/frame 0:{} 1:{} 2:{} 3:{} 4:{} 5+:{} | shifted/frame 0:{} 1:{} 2:{} 3+:{}",
                g_flips, g_views_hist[0], g_views_hist[1], g_views_hist[2], g_views_hist[3],
                g_views_hist[4], g_views_hist[5], g_shifted_hist[0], g_shifted_hist[1],
                g_shifted_hist[2], g_shifted_hist[3] + g_shifted_hist[4] + g_shifted_hist[5]);
    for (int i = 0; i < g_nsigs; ++i) {
      const Signature& g = g_sigs[i];
      REXGPU_INFO("[C2VR]   sig{} lr=0x{:08X} view=0x{:08X} mode={} rect=({},{},{},{}) fov=({:.4f},{:.4f}) near={:.3f} far={:.1f} count={}",
                  i, g.lr, g.view, g.mode, g.rect[0], g.rect[1], g.rect[2], g.rect[3], g.fov[0],
                  g.fov[1], g.nearz, g.farz, g.count);
    }
  }
}

// "Set view". Logs it, and moves the eye for the views the filter picks.
REX_HOOK_RAW(sub_8212BAB0) {
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t view = ctx.r4.u32;
  const uint32_t xf = ctx.r5.u32;
  const uint32_t fov = ctx.r6.u32;
  const uint32_t rect = ctx.r8.u32;
  const uint32_t mode = ctx.r9.u32;

  Signature cur{};
  cur.lr = lr;
  cur.view = view;
  cur.mode = mode;
  for (int i = 0; i < 4; ++i)
    cur.rect[i] = rect ? static_cast<int32_t>(REX_LOAD_U32(rect + 4u * i)) : 0;
  cur.fov[0] = fov ? LoadF(base, fov) : 0.0f;
  cur.fov[1] = fov ? LoadF(base, fov + 4u) : 0.0f;
  cur.nearz = static_cast<float>(ctx.f1.f64);
  cur.farz = static_cast<float>(ctx.f2.f64);

  ++g_views_this_frame;
  int idx = -1;
  for (int i = 0; i < g_nsigs; ++i)
    if (SameSig(g_sigs[i], cur)) { idx = i; break; }
  if (idx < 0 && g_nsigs < kMaxSignatures) {
    idx = g_nsigs++;
    g_sigs[idx] = cur;
  }
  if (idx >= 0) {
    Signature& g = g_sigs[idx];
    ++g.count;
    if (g.logged < kMaxLoggedPerSignature && xf != 0u) {
      ++g.logged;
      float t[7];
      for (int i = 0; i < 7; ++i) t[i] = LoadF(base, xf + 4u * i);
      REXGPU_INFO("[C2VR] view sig{} lr=0x{:08X} thread={} pos=({:.2f} {:.2f} {:.2f}) quat=({:.4f} {:.4f} {:.4f} {:.4f})",
                  idx, lr, std::hash<std::thread::id>{}(std::this_thread::get_id()), t[0], t[1],
                  t[2], t[3], t[4], t[5], t[6]);
    }
  }

  const Settings& s = Get();
  const bool pick = s.cam_eye != 0.0f && xf != 0u && view == kCurrentView &&
                    mode == kModePerspective && (s.only_lr == 0u || s.only_lr == lr);
  float saved[3] = {};
  if (pick) {
    const float px = LoadF(base, xf), py = LoadF(base, xf + 4u), pz = LoadF(base, xf + 8u);
    float qx = LoadF(base, xf + 12u), qy = LoadF(base, xf + 16u), qz = LoadF(base, xf + 20u),
          qw = LoadF(base, xf + 24u);
    const float n = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    if (n > 0.0f) { qx /= n; qy /= n; qz /= n; qw /= n; }
    // right = q * (1,0,0)
    const float rx = 1.0f - 2.0f * (qy * qy + qz * qz);
    const float ry = 2.0f * (qx * qy + qz * qw);
    const float rz = 2.0f * (qx * qz - qy * qw);
    const float e = g_sign * s.cam_eye;
    saved[0] = px; saved[1] = py; saved[2] = pz;
    StoreF(base, xf, px + e * rx);
    StoreF(base, xf + 4u, py + e * ry);
    StoreF(base, xf + 8u, pz + e * rz);
    ++g_shifted_this_frame;
    static int shown = 0;
    if (shown < 3) {
      ++shown;
      REXGPU_INFO("[C2VR] shift lr=0x{:08X} e={} right=({:.4f} {:.4f} {:.4f}) |right|={:.4f}", lr, e,
                  rx, ry, rz, std::sqrt(rx * rx + ry * ry + rz * rz));
    }
  }

  __imp__sub_8212BAB0(ctx, base);

  if (pick) {  // the caller's buffer may be the camera object: give it back untouched
    StoreF(base, xf, saved[0]);
    StoreF(base, xf + 4u, saved[1]);
    StoreF(base, xf + 8u, saved[2]);
  }
}
