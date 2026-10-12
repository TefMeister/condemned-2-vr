// Condemned 2: music off, and the VR screen-effects rule (everything painted on the screen off).
// Reader draft, 2026-10-12. Static findings, all [inferred-static] until a run shows them.
//
// The FIRST line of defence is not this file: it is four lines in Assets\autoexec.cfg, which the
// engine reads at start-up (d:\autoexec.cfg, sub_8217CD78) and which needs no sign-in:
//     "MusicEnable" "0"           music volume forced to 0 by the game's own sound apply
//     "EnableScreenEffects" "0"   the full-screen effect system (CScreenEffectFX, FX/ScreenEffect)
//     "EnableDamageFX" "0"        the damage FX manager clears itself and returns
//     "CamDamage" "0"             camera kick when hit
//     "MotionBlur_Enable" "0"     renderer motion blur (but level objects re-set it; see below)
// The game creates these console variables only if they do not exist yet (sub_82175B90 looks the
// name up first), so a value from autoexec.cfg survives. This file covers what autoexec cannot:
//
//  C2_MUSIC_OFF=1
//    sub_822DD240(f1 = effects, f2 = music, f3 = speech) pushes the three volumes to the mixer; it
//    is called only from the sound-settings apply sub_823C7578 (start-up, profile load, options
//    menu). f2 is forced to 0. Effects and speech untouched.
//
//  C2_VR_SCREEN_FX_OFF=1
//    1. sub_826A7EF8(r3 = name, r4 = bool) = "set console variable (bool)". The level object
//       MotionBlurProperties (sub_824EFDE0) turns MotionBlur_Enable back on in places; forced 0.
//    2. sub_826A7F90(r3 = name, f1 = value) = "set console variable (float)". The profile apply
//       (sub_823C7D18) writes HeadBob = profile value x 0.1; forced 0. (Head bob is camera motion,
//       not a picture effect: this is a choice for Tefa; C2_KEEP_HEADBOB=1 leaves it.)
//    3. sub_82123AA0(r4 = material, r5 = parameter name, f1 = value) = ILTRenderer slot 51, set a
//       material float. Depth of field is a material whose strength is "k_fDepthOfField"
//       (string 0x82017744), written by sub_82309E98 (player, every frame) and sub_823044B0.
//       Forced 0 for that one parameter name.
//    4. sub_82175B90(r3 = handle, r4 = client, r5 = name, r6 = string default, f1 = default) =
//       "register console variable". For EnableScreenEffects / EnableDamageFX / CamDamage the float
//       default becomes 0, so the switch holds even without the autoexec.cfg lines. It only
//       applies when the variable does not exist yet (the original checks).

#include "generated/default/condemned2recomp_pch.h"

#include <rex/hook.h>

#include <cstdlib>
#include <cstring>

DECLARE_REX_FUNC(sub_822DD240);
DECLARE_REX_FUNC(sub_826A7EF8);
DECLARE_REX_FUNC(sub_826A7F90);
DECLARE_REX_FUNC(sub_82123AA0);
DECLARE_REX_FUNC(sub_82175B90);

namespace {

// Guest string addresses (pre-title-update default.xex, the one this build uses).
constexpr uint32_t kStrMotionBlurEnable = 0x8201451C;   // "MotionBlur_Enable"
constexpr uint32_t kStrHeadBob = 0x82021F0C;            // "HeadBob"
constexpr uint32_t kStrDepthOfField = 0x82017744;       // "k_fDepthOfField"
constexpr uint32_t kStrEnableScreenEffects = 0x82024A80;
constexpr uint32_t kStrEnableDamageFX = 0x8201EC48;
constexpr int kMaxLogs = 4;

bool EnvOn(const char* name) {
  const char* v = std::getenv(name);
  return v != nullptr && v[0] == '1';
}

struct Switches {
  bool music_off, fx_off, keep_headbob;
};

const Switches& Get() {
  static const Switches s = [] {
    Switches r{EnvOn("C2_MUSIC_OFF"), EnvOn("C2_VR_SCREEN_FX_OFF"), EnvOn("C2_KEEP_HEADBOB")};
    REXLOG_INFO("[C2FX] music_off={} screen_fx_off={} keep_headbob={}", r.music_off, r.fx_off,
                r.keep_headbob);
    return r;
  }();
  return s;
}

// Compare a guest C string with a literal (for names whose address we do not pin).
bool GuestStrEq(uint8_t* base, uint32_t a, const char* s) {
  if (a == 0u) return false;
  const char* g = reinterpret_cast<const char*>(base + a + REX_PHYS_HOST_OFFSET(a));
  return std::strcmp(g, s) == 0;
}

}  // namespace

REX_HOOK_RAW(sub_822DD240) {
  if (Get().music_off) {
    static int logged = 0;
    if (logged < kMaxLogs) {
      ++logged;
      REXLOG_INFO("[C2FX] sound apply: effects={} music={} -> 0 speech={}", ctx.f1.f64, ctx.f2.f64,
                  ctx.f3.f64);
    }
    ctx.f2.f64 = 0.0;
  }
  __imp__sub_822DD240(ctx, base);
}

REX_HOOK_RAW(sub_826A7EF8) {
  if (Get().fx_off && ctx.r3.u32 == kStrMotionBlurEnable && ctx.r4.u32 != 0u) {
    static int logged = 0;
    if (logged < kMaxLogs) { ++logged; REXLOG_INFO("[C2FX] MotionBlur_Enable set 1 -> 0"); }
    ctx.r4.u64 = 0;
  }
  __imp__sub_826A7EF8(ctx, base);
}

REX_HOOK_RAW(sub_826A7F90) {
  const Switches& s = Get();
  if (s.fx_off && !s.keep_headbob && ctx.r3.u32 == kStrHeadBob) {
    static int logged = 0;
    if (logged < kMaxLogs) { ++logged; REXLOG_INFO("[C2FX] HeadBob set {} -> 0", ctx.f1.f64); }
    ctx.f1.f64 = 0.0;
  }
  __imp__sub_826A7F90(ctx, base);
}

REX_HOOK_RAW(sub_82123AA0) {
  if (Get().fx_off && ctx.r5.u32 == kStrDepthOfField) {
    static int logged = 0;
    if (logged < kMaxLogs && ctx.f1.f64 != 0.0) {
      ++logged;
      REXLOG_INFO("[C2FX] k_fDepthOfField {} -> 0", ctx.f1.f64);
    }
    ctx.f1.f64 = 0.0;
  }
  __imp__sub_82123AA0(ctx, base);
}

REX_HOOK_RAW(sub_82175B90) {
  if (Get().fx_off && ctx.r6.u32 == 0u) {
    const uint32_t name = ctx.r5.u32;
    if (name == kStrEnableScreenEffects || name == kStrEnableDamageFX ||
        GuestStrEq(base, name, "CamDamage")) {
      REXLOG_INFO("[C2FX] register 0x{:08X} default {} -> 0", name, ctx.f1.f64);
      ctx.f1.f64 = 0.0;
    }
  }
  __imp__sub_82175B90(ctx, base);
}
