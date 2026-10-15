// Condemned 2 VR: both eyes of a pair at the SAME moment (C2_EYE_SYNC=1). Reader draft, 2026-10-12.
//
// What the code says (static reading of the generated code, [inferred-static] unless noted):
//
//  * ONE thread runs the frame. xstart -> sub_82178E90 -> sub_8217CEE8 loops
//    `while (!sub_821802A8(app)) {}`; sub_821802A8 is the whole frame: client/server update,
//    the render (through interface vtables, so not visible as direct calls), and at the end a
//    frame limiter (MaxFPS at 0x8292864C) that spins on the clock with Sleep(1) (lr 0x821806D4).
//    Live, FlipScreen and every logged set-view ran on the same host thread [verified-live
//    2026-10-12, n=3 runs, first 3 calls per view kind]. No render-command queue like The
//    Darkness's packet queue was found: no queue/thread strings, no render-thread class.
//    So the eye chosen for a frame is the eye of the views built in it, provided it is counted
//    by frames that really reached VdSwap.
//  * The only clocks game code reads: sub_82650F50 = QueryPerformanceCounter(LARGE_INTEGER*)
//    (mftb; 5 callers, the main one the wrapper sub_822CE2D0, 14 callers) and sub_82650F40 =
//    GetTickCount (reads KeTimeStampBundle +16, the ONLY reader of 0x82000830; under the wrappers
//    sub_822CE290 "ms since start", 55 callers, and sub_822CE230 "seconds", 11 callers).
//    Raw mftb also appears inline in D3D/library code (fences, profiling); not game time.
//  * Every VdSwap goes through sub_8265F2B0, from two places: FlipScreen (lr 0x82122F00) and the
//    D3D keep-alive THREAD sub_82671908 (lr 0x826719F4; created by the device-create
//    sub_8265EA28, wakes every 30 ms and re-presents the stored front buffer while a device flag
//    bit is set). Its swaps are not new pictures. sub_8265F2B0 skips VdSwap when
//    device+21532 != 0 [verified-numerically 2026-10-12: return addresses read from the code].
//
// Design (The Darkness's corrected freeze, adapted to one thread):
//  * Eye = parity of g_seq, the number of FlipScreen presents that reached VdSwap. Views take
//    the eye of g_seq while they are built; FlipScreen advances it only when VdSwap really ran,
//    so a skipped present redraws the same eye at the same held moment.
//  * Every VdSwap is entered in a numbered table BEFORE it is issued: FlipScreen swaps carry the
//    frame's eye and g_seq, keep-alive swaps carry -4 ("repeat the last pair", SDK patch 08c) or
//    -1 with C2_SBS_SKIP_FILLERS=0. SDK patch 08 asks dk_sbs_swap_eye(n) per IssueSwap.
//  * Freeze (period 1 only): the right-eye frame sees the clocks as they were at the START of
//    its left-eye frame (first read of each clock), +1 tick per read so polling loops still end.
//    Default window: from the right frame's entry into sub_821802A8 to its present, so the
//    limiter never runs pinned. C2_FREEZE_FROM=present pins from the left present instead (use
//    it if the log shows flips_outside_frame > 0); then the limiter's Sleep releases the clock.
//    Only the frame thread is ever pinned. Cap C2_FREEZE_CAP_MS (250) real ms per pin.

#include "generated/default/condemned2recomp_pch.h"

#include <rex/hook.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <thread>

#include "vr_eye_sync.h"

DECLARE_REX_FUNC(sub_821802A8);  // the frame
DECLARE_REX_FUNC(sub_8265F2B0);  // D3D swap -> VdSwap
DECLARE_REX_FUNC(sub_82650F50);  // QueryPerformanceCounter
DECLARE_REX_FUNC(sub_82650F40);  // GetTickCount
DECLARE_REX_FUNC(sub_822CE208);  // Sleep(ms) wrapper

namespace {

constexpr uint32_t kFlipPresentLr = 0x82122F00;  // FlipScreen's call of sub_8265F2B0
constexpr uint32_t kKeepAliveLr = 0x826719F4;    // keep-alive thread's call of sub_8265F2B0
constexpr uint32_t kLimiterSleepLr = 0x821806D4; // frame limiter's Sleep(1) in sub_821802A8
constexpr uint32_t kDeviceNoSwap = 21532;        // device field: VdSwap skipped when != 0
constexpr double kTicksPerMs = 50000.0;          // guest time base, 50 MHz
constexpr uint64_t kSwapSlots = 1024;

int EnvInt(const char* name, int def) {
  const char* v = std::getenv(name);
  return (v != nullptr && v[0] != '\0') ? std::atoi(v) : def;
}

struct Settings {
  bool on = false;
  int period = 1;
  bool freeze = false;
  double cap_ms = 250.0;
  bool exact = true;
  bool from_present = false;
  bool repeat_keepalive = true;
  bool eye_swap = false;
};

const Settings& Get() {
  static const Settings s = [] {
    Settings r;
    r.on = EnvInt("C2_EYE_SYNC", 0) == 1;
    r.period = EnvInt("C2_STEREO_PERIOD", 1);
    if (r.period < 1) r.period = 1;
    r.freeze = r.on && EnvInt("C2_FREEZE", 1) == 1 && r.period == 1;
    r.cap_ms = EnvInt("C2_FREEZE_CAP_MS", 250);
    r.exact = EnvInt("C2_FREEZE_EXACT", 1) == 1;
    const char* from = std::getenv("C2_FREEZE_FROM");
    r.from_present = from != nullptr && from[0] == 'p';
    r.repeat_keepalive = EnvInt("C2_SBS_SKIP_FILLERS", 1) == 1;
    r.eye_swap = EnvInt("C2_EYE_SWAP", 0) == 1;
    const int sdk_period = EnvInt("DK_STEREO_PERIOD", 1);
    REXGPU_INFO("[C2VR] SYNC settings on={} period={} freeze={} from={} cap_ms={} exact={} "
                "keepalive={} eye_swap={}{}",
                r.on, r.period, r.freeze, r.from_present ? "present" : "frame", r.cap_ms, r.exact,
                r.repeat_keepalive ? "repeat(-4)" : "mono(-1)", r.eye_swap,
                (r.on && sdk_period != r.period)
                    ? " | WARNING: DK_STEREO_PERIOD differs; the SDK pairs by DK_STEREO_PERIOD"
                    : "");
    if (r.on && EnvInt("C2_FREEZE", 1) == 1 && r.period != 1)
      REXGPU_INFO("[C2VR] SYNC freeze ignored: it needs C2_STEREO_PERIOD=1");
    return r;
  }();
  return s;
}

std::atomic<uint64_t> g_seq{0};  // FlipScreen presents that reached VdSwap
inline int EyeNow() {
  return static_cast<int>((g_seq.load(std::memory_order_relaxed) /
                           static_cast<uint64_t>(Get().period)) & 1u);
}

thread_local bool t_frame_thread = false;  // this thread runs sub_821802A8
thread_local bool t_in_frame = false;
thread_local bool t_swapped = false;       // FlipScreen's sub_8265F2B0 call issued VdSwap
thread_local bool t_exempt = false;        // clock reads in the limiter stay real

// Freeze state: touched by the frame thread only.
struct HeldClock {
  bool first_pending = true;  // the next read is this frame's start, taken as the hold value
  bool valid = false;
  uint64_t hold = 0, last = 0, k = 0;
};
HeldClock g_qpc, g_tick;
bool g_pin = false, g_capped_this_pair = false;
std::chrono::steady_clock::time_point g_pin_since;
// Pair measure (freeze on or off): the first QPC value each eye's frame saw at its start.
bool g_frame_first_pending = false;
uint64_t g_first[2] = {};
bool g_first_valid[2] = {};

// Swap table, read by SDK patch 08 on the GPU thread. tag = ((index + 1) << 8) | (eye + 8).
struct SwapSlot {
  std::atomic<uint64_t> tag{0};
  std::atomic<uint32_t> serial{0};
};
SwapSlot g_slots[kSwapSlots];
std::atomic<uint64_t> g_swap_index{0};

// Counters for the report.
std::atomic<uint64_t> g_flip_swaps{0}, g_skipped_vdswap{0}, g_keepalive_swaps{0},
    g_other_swaps{0}, g_lookup_miss{0}, g_views_off_thread{0}, g_flips_off_thread{0},
    g_other_qpc{0}, g_other_tick{0};
uint64_t g_eye_count[2] = {}, g_flips_in_frame = 0, g_flips_outside = 0, g_no_present = 0,
         g_flip_not_swapped = 0, g_pins = 0, g_capped = 0, g_pinned_qpc = 0, g_pinned_tick = 0,
         g_exempt = 0, g_limiter_sleeps = 0, g_main_qpc = 0, g_main_tick = 0, g_pairs = 0;
double g_max_hold_ms = 0.0, g_pair_dt_sum_ms = 0.0, g_pair_dt_max_ms = 0.0;

void StartPin() {
  if (!g_qpc.valid || g_pin || g_capped_this_pair) return;
  g_pin = true;
  g_pin_since = std::chrono::steady_clock::now();
  g_qpc.k = g_tick.k = 0;
  ++g_pins;
}

void ArmLeftFrame() {
  g_pin = false;
  g_capped_this_pair = false;
  g_qpc.first_pending = g_tick.first_pending = true;
}

// One clock read on the frame thread. real/v in the clock's own unit; per_tick converts the
// +1-per-read nudge (in 50 MHz ticks) into that unit.
uint64_t Filter(HeldClock& c, uint64_t real, double per_tick, uint64_t& pinned_count, bool is_qpc) {
  uint64_t v = real;
  if (g_pin && t_exempt) {
    ++g_exempt;
  } else if (g_pin && c.valid) {  // a clock never read in the left frame has no hold: stays real
    const double held = std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - g_pin_since).count();
    if (held > Get().cap_ms) {
      g_pin = false;  // never stall the game: let go, and count it
      g_capped_this_pair = true;
      ++g_capped;
    } else {
      ++c.k;
      v = Get().exact ? c.hold + static_cast<uint64_t>(static_cast<double>(c.k) * per_tick)
                      : c.last;
      ++pinned_count;
      if (is_qpc && real > v) {
        const double ms = static_cast<double>(real - v) / kTicksPerMs;
        if (ms > g_max_hold_ms) g_max_hold_ms = ms;
      }
    }
  }
  if (!g_pin && v < c.last) v = c.last;  // after a hold the real clock is ahead; a guard only
  if (c.first_pending && !t_exempt) {
    c.first_pending = false;
    c.hold = v;
    c.valid = true;
  }
  if (is_qpc && g_frame_first_pending && !t_exempt) {
    g_frame_first_pending = false;
    const int eye = EyeNow();
    g_first[eye] = v;
    g_first_valid[eye] = true;
  }
  if (!(g_pin && t_exempt)) c.last = v;
  return v;
}

}  // namespace

namespace c2vr {

bool SyncOn() { return Get().on; }

uint32_t CurrentPair() {
  return static_cast<uint32_t>(g_seq.load(std::memory_order_relaxed) /
                               (2u * static_cast<uint64_t>(Get().period)));
}

float EyeSignForView() {
  if (!t_frame_thread) g_views_off_thread.fetch_add(1, std::memory_order_relaxed);
  float s = EyeNow() == 0 ? -1.0f : 1.0f;  // left eye moves left (-right axis)
  return Get().eye_swap ? -s : s;
}

void OnFlipBegin() { t_swapped = false; }

void OnFlipEnd(bool presented) {
  if (!Get().on) return;
  if (!t_frame_thread) {
    g_flips_off_thread.fetch_add(1, std::memory_order_relaxed);
    return;  // the design assumes one thread; never touch the freeze from another
  }
  ++(t_in_frame ? g_flips_in_frame : g_flips_outside);
  if (!presented) { ++g_no_present; return; }
  if (!t_swapped) { ++g_flip_not_swapped; return; }  // nothing shown: same eye again next frame
  const int eye = EyeNow();
  ++g_eye_count[eye];
  if (eye == 1 && g_first_valid[0] && g_first_valid[1]) {
    // How far the right eye's moment is from the left eye's: ~0 with the freeze, ~1 frame without.
    const double dt = g_first[1] >= g_first[0]
                          ? static_cast<double>(g_first[1] - g_first[0]) / kTicksPerMs : 0.0;
    ++g_pairs;
    g_pair_dt_sum_ms += dt;
    if (dt > g_pair_dt_max_ms) g_pair_dt_max_ms = dt;
    g_first_valid[0] = g_first_valid[1] = false;
  }
  g_seq.fetch_add(1, std::memory_order_relaxed);
  if (!Get().freeze) return;
  if (Get().from_present) {
    if (eye == 0) StartPin(); else ArmLeftFrame();
  } else if (eye == 1) {
    g_pin = false;  // the right frame is shown; the next frame is a left one (armed at entry)
  }
}

void ReportSync() {
  if (!Get().on) return;
  REXGPU_INFO("[C2VR] SYNC seq={} L={} R={} flip_swaps={} skipped_vdswap={} keepalive_swaps={} "
              "other_swaps={} sdk_lookup_miss={} flips_in_frame={} flips_outside_frame={} "
              "flips_off_thread={} no_present={} not_swapped={} views_off_thread={}",
              g_seq.load(), g_eye_count[0], g_eye_count[1], g_flip_swaps.load(),
              g_skipped_vdswap.load(), g_keepalive_swaps.load(), g_other_swaps.load(),
              g_lookup_miss.load(), g_flips_in_frame, g_flips_outside, g_flips_off_thread.load(),
              g_no_present, g_flip_not_swapped, g_views_off_thread.load());
  REXGPU_INFO("[C2VR] FREEZE on={} pins={} capped={} pinned_qpc={} pinned_tick={} exempt={} "
              "limiter_sleeps={} max_hold_ms={:.2f} pairs={} pair_dt mean={:.3f}ms max={:.3f}ms | "
              "reads frame-thread qpc={} tick={} other-threads qpc={} tick={}",
              Get().freeze, g_pins, g_capped, g_pinned_qpc, g_pinned_tick, g_exempt,
              g_limiter_sleeps, g_max_hold_ms, g_pairs,
              g_pairs ? g_pair_dt_sum_ms / static_cast<double>(g_pairs) : 0.0, g_pair_dt_max_ms,
              g_main_qpc, g_main_tick, g_other_qpc.load(), g_other_tick.load());
}

}  // namespace c2vr

// Asked by SDK patch 08 (IssueSwap) once per swap, numbered from 0. 0 left, 1 right, -1 no eye,
// -2 switched off (old parity rule), -3 not in the table, -4 repeat the last pair (08c).
extern "C" __declspec(dllexport) int dk_sbs_swap_eye(uint64_t swap_index, uint32_t* serial_out) {
  if (!Get().on) return -2;
  const SwapSlot& slot = g_slots[swap_index % kSwapSlots];
  const uint64_t tag = slot.tag.load(std::memory_order_acquire);
  if ((tag >> 8) != swap_index + 1u) {
    g_lookup_miss.fetch_add(1, std::memory_order_relaxed);
    return -3;
  }
  if (serial_out != nullptr) *serial_out = slot.serial.load(std::memory_order_relaxed);
  return static_cast<int>(tag & 0xFFu) - 8;
}

// The frame. Marks the frame thread; in frame mode starts the pin for a right-eye frame.
REX_HOOK_RAW(sub_821802A8) {
  if (!t_frame_thread) {
    t_frame_thread = true;
    REXGPU_INFO("[C2VR] frame thread={}", std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  t_in_frame = true;
  t_exempt = false;
  g_frame_first_pending = true;
  if (Get().freeze && !Get().from_present) {
    if (EyeNow() == 0) ArmLeftFrame(); else StartPin();
  }
  __imp__sub_821802A8(ctx, base);
  t_in_frame = false;
}

// The swap. Enters each VdSwap in the table before it is issued.
REX_HOOK_RAW(sub_8265F2B0) {
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  const uint32_t dev = ctx.r3.u32;
  if (Get().on) {
    const bool will_swap = dev != 0u && REX_LOAD_U32(dev + kDeviceNoSwap) == 0u;
    int eye;
    if (lr == kFlipPresentLr) {
      eye = EyeNow();
      g_flip_swaps.fetch_add(1, std::memory_order_relaxed);
      if (!will_swap) g_skipped_vdswap.fetch_add(1, std::memory_order_relaxed);
      t_swapped = will_swap;
    } else if (lr == kKeepAliveLr) {
      eye = Get().repeat_keepalive ? -4 : -1;
      g_keepalive_swaps.fetch_add(1, std::memory_order_relaxed);
    } else {
      eye = -1;
      g_other_swaps.fetch_add(1, std::memory_order_relaxed);
    }
    if (will_swap) {
      const uint64_t i = g_swap_index.fetch_add(1, std::memory_order_relaxed);
      SwapSlot& slot = g_slots[i % kSwapSlots];
      slot.serial.store(static_cast<uint32_t>(g_seq.load(std::memory_order_relaxed)),
                        std::memory_order_relaxed);
      slot.tag.store(((i + 1u) << 8) | static_cast<uint64_t>(eye + 8), std::memory_order_release);
    }
  }
  __imp__sub_8265F2B0(ctx, base);
}

// QueryPerformanceCounter(LARGE_INTEGER* out): 64-bit big-endian count at out.
REX_HOOK_RAW(sub_82650F50) {
  const uint32_t out = ctx.r3.u32;
  __imp__sub_82650F50(ctx, base);
  if (!Get().on || out == 0u) return;
  if (!t_frame_thread) { g_other_qpc.fetch_add(1, std::memory_order_relaxed); return; }
  ++g_main_qpc;
  const uint64_t real = (static_cast<uint64_t>(REX_LOAD_U32(out)) << 32) | REX_LOAD_U32(out + 4u);
  const uint64_t v = Filter(g_qpc, real, 1.0, g_pinned_qpc, true);
  if (v != real) {
    REX_STORE_U32(out, static_cast<uint32_t>(v >> 32));
    REX_STORE_U32(out + 4u, static_cast<uint32_t>(v & 0xFFFFFFFFu));
  }
}

// GetTickCount: milliseconds in r3.
REX_HOOK_RAW(sub_82650F40) {
  __imp__sub_82650F40(ctx, base);
  if (!Get().on) return;
  if (!t_frame_thread) { g_other_tick.fetch_add(1, std::memory_order_relaxed); return; }
  ++g_main_tick;
  const uint64_t real = ctx.r3.u32;
  const uint64_t v = Filter(g_tick, real, 1.0 / kTicksPerMs, g_pinned_tick, false);
  ctx.r3.u64 = static_cast<uint32_t>(v);
}

// Sleep(ms). The frame limiter's Sleep(1) lets its clock reads run real for the rest of the frame.
REX_HOOK_RAW(sub_822CE208) {
  if (t_frame_thread && g_pin && static_cast<uint32_t>(ctx.lr) == kLimiterSleepLr) {
    if (!t_exempt) ++g_limiter_sleeps;
    t_exempt = true;
  }
  __imp__sub_822CE208(ctx, base);
}
