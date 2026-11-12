// Condemned 2 VR: the motion controllers as a pad (2026-11-12, step 2 of the hands-and-melee plan).
//
// Tefa 2026-11-12: "get hands to follow motion controllers and if you really can, already look into physical melee
// attacks as well, not button press ones ... blocking physical too". Before the hands can be steered, the controllers
// have to reach the game at all: this file makes them a pad, the way The Darkness's vr_bridge.cpp does (its pad part,
// lifted; no bridge pipe here). The SDK (The Darkness's copy, patch 11) samples the controllers through OpenXR and hands
// them over with rex_xr_sample_buttons(); this driver lays them over a virtual pad that the input system merges with any
// real pad (buttons OR'd, the larger stick wins), so the Xbox pad keeps working beside them.
//
// The pad Condemned 2 reads (condemned2recomp.toml): A use, B flashlight, X reload, Y check, LT swing left / aim, RT swing
// right / shoot, LB run, RB throw the weapon, the right stick's click kick, START pause, BACK the map / select.
// Quest -> pad:
//   A -> A, Y -> Y; B -> X and X -> B (C2_XR_SWAP_BX=1, The Darkness's swap: Tefa 2026-10-29 "quest A - A, Quest B - X,
//   Quest X - B and Quest Y - Y on a gamepad"); C2_XR_SWAP_BX=0 = as printed on the controllers
//   triggers -> triggers, sticks -> sticks, stick clicks -> stick clicks
//   left grip -> LB (run); right grip -> nothing by default: RB THROWS THE WEAPON in this game, and the grips will hold
//   the weapon once the hands follow the controllers (C2_XR_RGRIP_RB=1 gives it RB anyway)
//   the left menu button held 0.6 s -> START, a short click -> one tap of BACK
// On with DK_XR=1 unless C2_XR_PAD=0. [compile-verified] at most until it has run.

#include <windows.h>

#include <rex/input/input_driver.h>
#include <rex/input/input_system.h>
#include <rex/logging/macros.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {

using rex::X_RESULT;
using rex::X_STATUS;

constexpr rex::input::DeviceId kPadId = static_cast<rex::input::DeviceId>(0xC2C0A70000001ull);
constexpr uint8_t kDevTypeGamepad = 0x01, kDevSubTypeGamepad = 0x01, kTriggerFull = 0xFF;
constexpr uint16_t kAllButtons = 0xFFFF;
constexpr float kStickMax = 32767.0f;
constexpr float kStickDead = 0.15f;      // a stick this far from the middle is handed over
constexpr uint32_t kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000, kMenu = 0x0010;
constexpr uint32_t kStart = 0x0010, kBack = 0x0020, kLB = 0x0100, kRB = 0x0200;
constexpr int kMenuHoldMs = 600;         // held this long = START; let go sooner = one tap of BACK
constexpr int kMenuTapReads = 6;         // how many pad reads the BACK tap is held for

bool EnvOn(const char* name, bool fallback) {
  const char* e = std::getenv(name);
  return e ? e[0] == '1' : fallback;
}

using XrButtonsFn = int (*)(uint32_t*, float*);
XrButtonsFn XrButtons() {
  static const XrButtonsFn fn = [] {
    if (!EnvOn("C2_XR_PAD", EnvOn("DK_XR", false))) return XrButtonsFn(nullptr);
    HMODULE gpu = GetModuleHandleW(L"rexgpu-xenos.dll");
    XrButtonsFn f = gpu ? reinterpret_cast<XrButtonsFn>(reinterpret_cast<void*>(GetProcAddress(gpu, "rex_xr_sample_buttons"))) : nullptr;
    REXLOG_INFO("[C2XR] controller pad {}", f ? "on: the motion controllers work the pad (rex_xr_sample_buttons)" : "NOT in this SDK build");
    return f;
  }();
  return fn;
}

uint32_t Remap(uint32_t b) {
  static const bool swap_bx = EnvOn("C2_XR_SWAP_BX", true);
  static const bool rgrip_rb = EnvOn("C2_XR_RGRIP_RB", false);
  static std::chrono::steady_clock::time_point menu_since;
  static bool menu_was = false, start_sent = false;
  static int back_reads = 0;
  uint32_t out = b & ~(kA | kB | kX | kY | kMenu | kRB);
  if (b & kA) out |= kA;
  if (b & kY) out |= kY;
  if (b & kB) out |= swap_bx ? kX : kB;
  if (b & kX) out |= swap_bx ? kB : kX;
  if ((b & kRB) && rgrip_rb) out |= kRB;
  const bool menu = (b & kMenu) != 0;
  const auto now = std::chrono::steady_clock::now();
  if (menu && !menu_was) { menu_since = now; start_sent = false; }
  if (menu) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - menu_since).count() >= kMenuHoldMs) {
      out |= kStart;
      start_sent = true;
    }
  } else if (menu_was && !start_sent) {
    back_reads = kMenuTapReads;
  }
  if (back_reads > 0) { out |= kBack; --back_reads; }
  menu_was = menu;
  return out;
}

class XrPadDriver final : public rex::input::InputDriver {
 public:
  XrPadDriver() : InputDriver(nullptr, 0) {}
  X_STATUS Setup() override { return X_STATUS_SUCCESS; }

  void EnumerateDevices(std::vector<rex::input::DeviceInfo>& out) override {
    rex::input::DeviceInfo info;
    info.id = kPadId;
    info.name = "Quest controllers (Condemned 2 VR)";
    info.synthetic = true;
    out.push_back(info);
  }

  X_RESULT GetDeviceState(rex::input::DeviceId id, rex::input::X_INPUT_STATE* out) override {
    if (id != kPadId) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (!out) return X_ERROR_SUCCESS;
    std::memset(out, 0, sizeof(*out));
    out->packet_number = ++packet_;
    const XrButtonsFn xr = XrButtons();
    uint32_t b = 0;
    float ax[6] = {0, 0, 0, 0, 0, 0};
    if (xr == nullptr || !xr(&b, ax)) return X_ERROR_SUCCESS;   // no headset session: a pad at rest
    const auto trig = [](float v) { return static_cast<uint8_t>(v <= 0 ? 0 : v >= 1 ? 255 : v * 255.0f + 0.5f); };
    const auto axis = [](float v) { return static_cast<int16_t>(v <= -1 ? -32768 : v >= 1 ? 32767 : v * 32767.0f); };
    out->gamepad.buttons = static_cast<uint16_t>(Remap(b));
    out->gamepad.left_trigger = trig(ax[0]);
    out->gamepad.right_trigger = trig(ax[1]);
    if (std::fabs(ax[2]) > kStickDead || std::fabs(ax[3]) > kStickDead) {
      out->gamepad.thumb_lx = axis(ax[2]);
      out->gamepad.thumb_ly = axis(ax[3]);
    }
    if (std::fabs(ax[4]) > kStickDead || std::fabs(ax[5]) > kStickDead) {
      out->gamepad.thumb_rx = axis(ax[4]);
      out->gamepad.thumb_ry = axis(ax[5]);
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT GetDeviceCapabilities(rex::input::DeviceId id, uint32_t flags, rex::input::X_INPUT_CAPABILITIES* out) override {
    (void)flags;
    if (id != kPadId) return X_ERROR_DEVICE_NOT_CONNECTED;
    if (out) {
      std::memset(out, 0, sizeof(*out));
      out->type = kDevTypeGamepad;
      out->sub_type = kDevSubTypeGamepad;
      out->gamepad.buttons = kAllButtons;
      out->gamepad.left_trigger = kTriggerFull;
      out->gamepad.right_trigger = kTriggerFull;
      out->gamepad.thumb_lx = out->gamepad.thumb_ly = out->gamepad.thumb_rx = out->gamepad.thumb_ry =
          static_cast<int16_t>(kStickMax);
    }
    return X_ERROR_SUCCESS;
  }

  X_RESULT SetDeviceVibration(rex::input::DeviceId id, rex::input::X_INPUT_VIBRATION* v) override {
    (void)v;
    return id == kPadId ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_RESULT GetDeviceKeystroke(rex::input::DeviceId id, uint32_t flags, rex::input::X_INPUT_KEYSTROKE* out) override {
    (void)flags;
    (void)out;
    return id == kPadId ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
  }

 private:
  uint32_t packet_ = 0;
};

}  // namespace

// The input factory (condemned2recomp_app.h): the SDK's own drivers, plus the controllers as one more pad.
std::unique_ptr<rex::system::IInputSystem> C2CreateInputSystem(bool tool_mode) {
  auto input = rex::input::CreateDefaultInputSystem(tool_mode);
  if (input && !tool_mode) input->AddDriver(std::make_unique<XrPadDriver>());
  return input;
}
