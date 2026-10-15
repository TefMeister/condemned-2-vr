// Condemned 2 VR: the headset turns the game camera (C2_XR_HEAD=1), 2026-10-15, /pd draft.
//
// The SDK's patch 09 (DK_XR=1) sends each pair to the headset. This file is the game's half:
//  * once per pair, at the first player view, it samples the head through the SDK's
//    rex_xr_sample_head and keeps the sample, so BOTH eyes of a pair use the same head pose;
//  * vr_camera_probe.cpp turns the player camera by that head rotation (vr_head_math.h) before
//    the eye shift, so the shift follows the turned right axis;
//  * dk_xr_pair_sample tells the SDK which sample a pair was drawn with, so the headset gets the
//    exact pose of the picture and can correct for the time in between;
//  * dk_xr_eye_fov hands the SDK the field of view the player camera really draws with.
//
// Settings (all off unless set): C2_XR_HEAD=1 head rotation; C2_XR_HEAD_POS=1 head movement as
// well, measured from where the head was at the first sample; C2_XR_UNITS_PER_M (default 100,
// a guess: positions look like centimetres) scales it. Needs C2_EYE_SYNC=1 for pair numbers.
// C2_XR_FOV=1 (2026-10-15, /lm) draws the player view at the headset's own width and height
// (SDK patch 09b, rex_xr_headset_tangents) instead of the game's 16:9 view.
// Nothing here has run in the game yet: [compile-verified 2026-10-15] at most.

#include "generated/default/condemned2recomp_pch.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

#include <windows.h>

#include "vr_eye_sync.h"
#include "vr_head_math.h"
#include "vr_head_pose.h"

namespace {

constexpr uint32_t kPairRing = 64;
constexpr uint64_t kReportEverySamples = 600;  // one log line per this many head samples

struct Settings {
  bool head = false, pos = false, fov = false;
  float units_per_m = 100.0f;
};

const Settings& Get() {
  static const Settings s = [] {
    Settings r;
    if (const char* v = std::getenv("C2_XR_HEAD")) r.head = v[0] == '1';
    if (const char* v = std::getenv("C2_XR_HEAD_POS")) r.pos = v[0] == '1';
    if (const char* v = std::getenv("C2_XR_FOV")) r.fov = v[0] == '1';
    if (const char* v = std::getenv("C2_XR_UNITS_PER_M")) r.units_per_m = float(std::atof(v));
    REXGPU_INFO("[C2VR] XR head={} pos={} fov={} units_per_m={}", r.head, r.pos, r.fov,
                r.units_per_m);
    return r;
  }();
  return s;
}

using TangentsFn = int (*)(float*);
TangentsFn HeadsetTangents() {  // SDK patch 09b; null on an older SDK
  static const TangentsFn fn = [] {
    HMODULE gpu = GetModuleHandleW(L"rexgpu-xenos.dll");
    return gpu ? reinterpret_cast<TangentsFn>(
                     reinterpret_cast<void*>(GetProcAddress(gpu, "rex_xr_headset_tangents")))
               : nullptr;
  }();
  return fn;
}

using SampleFn = int (*)(uint32_t*, float*, float*);
SampleFn Sampler() {
  static const SampleFn fn = [] {
    HMODULE gpu = GetModuleHandleW(L"rexgpu-xenos.dll");
    return gpu ? reinterpret_cast<SampleFn>(
                     reinterpret_cast<void*>(GetProcAddress(gpu, "rex_xr_sample_head")))
               : nullptr;
  }();
  return fn;
}

struct PairHead {
  std::atomic<uint32_t> pair{UINT32_MAX};
  std::atomic<uint32_t> id{0};
  c2vr::Quat q{0, 0, 0, 1};
  c2vr::Vec3 p{0, 0, 0};
};
PairHead g_pairs[kPairRing];
bool g_have_origin = false;
c2vr::Vec3 g_origin{0, 0, 0};
std::atomic<uint32_t> g_fov_bits[4] = {0, 0, 0, 0};  // left, right, up, down tangents
uint64_t g_samples = 0, g_no_sample = 0;

}  // namespace

namespace c2vr {

bool HeadOn() { return Get().head && SyncOn(); }

bool HeadForPair(uint32_t pair, Quat* q, Vec3* offset_m) {
  PairHead& slot = g_pairs[pair % kPairRing];
  if (slot.pair.load(std::memory_order_acquire) != pair) {
    uint32_t id = 0;
    float o[4] = {0, 0, 0, 1}, p[3] = {0, 0, 0};
    const SampleFn fn = Sampler();
    const bool ok = fn && fn(&id, o, p);
    ++(ok ? g_samples : g_no_sample);
    slot.q = {o[0], o[1], o[2], o[3]};
    if (ok && !g_have_origin) {
      g_have_origin = true;
      g_origin = {p[0], p[1], p[2]};
    }
    slot.p = {p[0] - g_origin.x, p[1] - g_origin.y, p[2] - g_origin.z};
    slot.id.store(ok ? id : 0u, std::memory_order_relaxed);
    slot.pair.store(pair, std::memory_order_release);
    if ((g_samples + g_no_sample) % kReportEverySamples == 1)
      REXGPU_INFO("[C2VR] XR head samples={} none={} q=({:.3f} {:.3f} {:.3f} {:.3f})", g_samples,
                  g_no_sample, o[0], o[1], o[2], o[3]);
  }
  *q = slot.q;
  *offset_m = Get().pos ? slot.p : Vec3{0, 0, 0};
  return slot.id.load(std::memory_order_relaxed) != 0;
}

float UnitsPerMetre() { return Get().units_per_m; }

float HalfEyeUnits(float fallback) {
  using DistFn = int (*)(float*);
  static const DistFn fn = [] {
    HMODULE gpu = GetModuleHandleW(L"rexgpu-xenos.dll");
    return gpu ? reinterpret_cast<DistFn>(
                     reinterpret_cast<void*>(GetProcAddress(gpu, "rex_xr_eye_distance")))
               : nullptr;
  }();
  float m = 0.0f;
  if (!fn || !fn(&m) || m <= 0.0f) return fallback;
  const float half = 0.5f * m * Get().units_per_m;
  static bool shown = false;
  if (!shown) {
    shown = true;
    REXGPU_INFO("[C2VR] eye spread from the headset: {:.4f} m -> each eye {:.3f} units", m, half);
  }
  return half;
}

bool HeadsetFov(float t[4]) {
  if (!Get().fov) return false;
  const TangentsFn fn = HeadsetTangents();
  if (!fn || !fn(t)) return false;
  static bool shown = false;
  if (!shown) {
    shown = true;
    REXGPU_INFO("[C2VR] XR fov: player view drawn at the headset's tangents left={:.4f} right={:.4f} "
                "up={:.4f} down={:.4f}",
                t[0], t[1], t[2], t[3]);
  }
  return t[0] > 0.0f && t[1] > 0.0f && t[2] > 0.0f && t[3] > 0.0f;
}

void NotePlayerTangents(float left, float right, float up, float down) {
  const float v[4] = {left, right, up, down};
  for (int i = 0; i < 4; ++i) {
    uint32_t b;
    std::memcpy(&b, &v[i], 4);
    g_fov_bits[i].store(b, std::memory_order_relaxed);
  }
}

void NotePlayerFov(float tan_x, float tan_y) { NotePlayerTangents(tan_x, tan_x, tan_y, tan_y); }

}  // namespace c2vr

// Asked by SDK patch 09: which head sample was pair `pair` drawn with?
extern "C" __declspec(dllexport) int dk_xr_pair_sample(uint32_t pair, uint32_t* id) {
  const PairHead& slot = g_pairs[pair % kPairRing];
  if (slot.pair.load(std::memory_order_acquire) != pair) return 0;
  *id = slot.id.load(std::memory_order_relaxed);
  return *id != 0;
}

// Asked by SDK patch 09: the player camera's tangents (left, right, up, down), all positive; the
// view is off-centre when they differ (C2_XR_FOV=1 with set-view's offset pair).
extern "C" __declspec(dllexport) int dk_xr_eye_fov(float t[4]) {
  for (int i = 0; i < 4; ++i) {
    const uint32_t b = g_fov_bits[i].load(std::memory_order_relaxed);
    if (b == 0) return 0;
    std::memcpy(&t[i], &b, 4);
  }
  return 1;
}
