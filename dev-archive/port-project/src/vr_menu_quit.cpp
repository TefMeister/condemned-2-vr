// Condemned 2 VR: QUIT on the main menu, in Multiplayer's place (2026-11-12).
//
// Tefa: "we need to do the same on Condemned 2" (as The Darkness: a quit in the main menu), "use multiplayer please"
// (Multiplayer is Xbox LIVE / System Link: dead in the port).
//
// The menus are Flash-style screens that call named handlers in the game (a table of {name, code, kind}; found live
// 2026-11-12 at guest 0x82934EE4: "Multiplayer.QueryMultiplayerAllowed" -> 0x822F9420, "Multiplayer.IsConnectedOnline"
// -> 0x822F6FA0, "Multiplayer.HasOnlineAccount" -> 0x822F7068, "Multiplayer.IsConnectedToLive" -> 0x822F7130,
// "Multiplayer.IsNetworkCordConnected" -> 0x822F5E20, "Multiplayer.OnlineConnectionRequired" -> 0x822F5E98,
// "Multiplayer.FlagIntentionalDisconnect" -> 0x822F80C8). Logged: they run while the Multiplayer line is HIGHLIGHTED,
// none when it is chosen (the page opens inside the menu's own script), so none of them can be the quit; they stay as
// log lines only. The quit itself goes through the game's text lookup: THE QUIT LINE, at the bottom of this file.

#include "generated/default/condemned2recomp_pch.h"

#include <windows.h>

#include <rex/hook.h>
#include <rex/system/kernel_state.h>
#include <rex/logging/macros.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <string>

DECLARE_REX_FUNC(sub_822F9420);
DECLARE_REX_FUNC(sub_822F6FA0);
DECLARE_REX_FUNC(sub_822F7068);
DECLARE_REX_FUNC(sub_822F7130);
DECLARE_REX_FUNC(sub_822F5E20);
DECLARE_REX_FUNC(sub_822F5E98);
DECLARE_REX_FUNC(sub_822F80C8);
DECLARE_REX_FUNC(sub_822F5838);   // "Multiplayer.OnConstruct"
DECLARE_REX_FUNC(sub_822F2F40);   // "Global.Exit"
DECLARE_REX_FUNC(sub_822F3320);   // "Global.GoToMainMenuScreen"
DECLARE_REX_FUNC(sub_82324238);   // the text lookup: r3 = the text id (guest char*), returns the text
DECLARE_REX_FUNC(sub_822F31D0);   // "Global.OnConstruct": sets the menu's platform
DECLARE_REX_FUNC(sub_822F2E30);   // sets "_global.g_nMonolithGlobalPlatform" (r4 = the movie, r5 = the number)

namespace {
bool EnvOn(const char* n, bool d) { const char* e = std::getenv(n); return e ? e[0] == '1' : d; }
std::atomic<int> g_lines{0};
void Note(const char* name) {
  static const bool on = EnvOn("C2_MENU_LOG", true);
  if (on && g_lines.fetch_add(1) < 200) REXLOG_INFO("[MENU] the menu asked {}", name);
}
BOOL CALLBACK CloseOurs(HWND h, LPARAM) {
  DWORD pid = 0;
  GetWindowThreadProcessId(h, &pid);
  wchar_t t[64] = {};
  GetWindowTextW(h, t, 63);
  if (pid == GetCurrentProcessId() && IsWindowVisible(h) && wcsncmp(t, L"condemned2recomp", 16) == 0) PostMessageW(h, WM_CLOSE, 0, 0);
  return TRUE;
}
}  // namespace

REX_HOOK_RAW(sub_822F9420) { Note("Multiplayer.QueryMultiplayerAllowed"); __imp__sub_822F9420(ctx, base); }
REX_HOOK_RAW(sub_822F6FA0) { Note("Multiplayer.IsConnectedOnline"); __imp__sub_822F6FA0(ctx, base); }
REX_HOOK_RAW(sub_822F7068) { Note("Multiplayer.HasOnlineAccount"); __imp__sub_822F7068(ctx, base); }
REX_HOOK_RAW(sub_822F7130) { Note("Multiplayer.IsConnectedToLive"); __imp__sub_822F7130(ctx, base); }
REX_HOOK_RAW(sub_822F5E20) { Note("Multiplayer.IsNetworkCordConnected"); __imp__sub_822F5E20(ctx, base); }
REX_HOOK_RAW(sub_822F5E98) { Note("Multiplayer.OnlineConnectionRequired"); __imp__sub_822F5E98(ctx, base); }
REX_HOOK_RAW(sub_822F80C8) { Note("Multiplayer.FlagIntentionalDisconnect"); __imp__sub_822F80C8(ctx, base); }
REX_HOOK_RAW(sub_822F5838) { Note("Multiplayer.OnConstruct"); __imp__sub_822F5838(ctx, base); }
// the menu's own "exit" line (CMainMenu adds it at the BOTTOM of the main menu when the platform is the PC one): the
// game's Exit handler, then our clean close (the console's exit does nothing useful in the port)
REX_HOOK_RAW(sub_822F2F40) {
  Note("Global.Exit");
  __imp__sub_822F2F40(ctx, base);
  static const bool on = EnvOn("C2_MENU_EXIT_CLOSES", true);
  if (on) { REXLOG_INFO("[MENU] Global.Exit: closing the game"); EnumWindows(CloseOurs, 0); }
}
// THE PLATFORM THE MENU IS TOLD (2026-11-12): Global.OnConstruct (0x822F31D0, r4 = the menu's movie object) sets
// "_global.g_nMonolithGlobalPlatform" from a double in the program's data (2.0 = Xbox 360, read live; that data is
// write-protected: writing it stopped the game at start). CMainMenu adds its own "exit" line at the bottom of the main
// menu only for the PC value [disproved 2026-11-12]. Right after OnConstruct, the game's own one-line setter for that
// variable (0x822F2E30: r4 = the movie object, r5 = the number) is called once more with C2_MENU_PLATFORM. Measured:
// 0 = no change, 1 = one more line at the bottom, "undefined", which is the developers' Custom level page (no exit),
// 3 = the PS3 legal screen at start. A PROBE, off unless C2_MENU_PLATFORM is set.
REX_HOOK_RAW(sub_822F31D0) {
  const uint64_t movie = ctx.r4.u64;
  __imp__sub_822F31D0(ctx, base);
  const char* e = std::getenv("C2_MENU_PLATFORM");
  if (movie == 0 || e == nullptr || e[0] == '\0') return;
  const int32_t want = std::atoi(e);
  const uint64_t r3 = ctx.r3.u64, r4 = ctx.r4.u64, r5 = ctx.r5.u64, lr = ctx.lr;
  ctx.r4.u64 = movie;
  ctx.r5.s64 = want;
  sub_822F2E30(ctx, base);
  ctx.r3.u64 = r3; ctx.r4.u64 = r4; ctx.r5.u64 = r5; ctx.lr = lr;
  REXLOG_INFO("[MENU] the menu was told platform {} (the game's own: 2 = Xbox 360)", want);
}
REX_HOOK_RAW(sub_822F3320) { Note("Global.GoToMainMenuScreen"); __imp__sub_822F3320(ctx, base); }

// THE QUIT LINE (2026-11-12, Tefa: "replace multiplayer with quit ... use multiplayer please"). Every menu word comes
// through the game's text lookup (0x82324238: r3 = the text id, a guest char*; returns the text). Logged live, in order:
// "IDS_MULTIPLAYER" -> "Multiplayer" builds the line, "IDS_MULTIPLAYER_TOOLTIP" is its hint line when highlighted, and
// "IDS_MULTIPLAYER_MENU" is looked up the moment A opens the Multiplayer page (and not before). So, with C2_MENU_QUIT
// on (default): the line reads "Quit" (C2_MENU_WORD), the hint "Close the game" (C2_MENU_HINT), and the page's title
// lookup closes the game the clean way (WM_CLOSE to its own window). Our words live in the SDK's system heap.
// C2_MENU_LOG=1 also logs every text id looked up, once each.
uint32_t GuestText(uint8_t* base, const char* env, const char* fallback) {
  const char* e = std::getenv(env);
  const char* text = (e != nullptr && e[0] != '\0') ? e : fallback;
  const size_t n = std::strlen(text);
  if (n >= 120) return 0;
  const uint32_t g = rex::system::kernel_memory()->SystemHeapAlloc(128);
  if (g != 0) std::memcpy(base + g, text, n + 1);
  return g;
}
REX_HOOK_RAW(sub_82324238) {
  static const bool on = EnvOn("C2_MENU_QUIT", true);
  static const bool log = EnvOn("C2_MENU_LOG", false);
  const uint32_t gid = ctx.r3.u32;
  const char* id = gid != 0 ? reinterpret_cast<const char*>(base + gid) : "";
  __imp__sub_82324238(ctx, base);
  if (log) {
    static std::set<std::string> seen;
    static std::mutex m;
    std::lock_guard<std::mutex> lk(m);
    if (seen.size() < 2000 && seen.insert(id).second) {
      const uint32_t r = ctx.r3.u32;
      REXLOG_INFO("[TEXT] {} -> \"{}\"", id, r != 0 ? std::string(reinterpret_cast<const char*>(base + r)).substr(0, 60) : std::string("(null)"));
    }
  }
  // 2026-11-12 (b004): A on ANY main-menu line builds every page's texts, IDS_MULTIPLAYER_MENU among them, so b003
  // closed the game on Single Player too [verified-live 2026-11-12, n=2]. The highlighted line is known from its hint:
  // the last *_TOOLTIP text looked up. Quit only when that is the Quit (Multiplayer) line's.
  static bool on_quit_line = false;
  {
    const size_t len = std::strlen(id);
    if (len > 8 && std::strcmp(id + len - 8, "_TOOLTIP") == 0) on_quit_line = std::strcmp(id, "IDS_MULTIPLAYER_TOOLTIP") == 0;
  }
  if (!on || id[0] != 'I' || std::strncmp(id, "IDS_MULTIPLAYER", 15) != 0) return;
  if (std::strcmp(id, "IDS_MULTIPLAYER") == 0) {
    static const uint32_t word = GuestText(base, "C2_MENU_WORD", "Quit");
    if (word != 0) ctx.r3.u64 = word;
  } else if (std::strcmp(id, "IDS_MULTIPLAYER_TOOLTIP") == 0) {
    static const uint32_t hint = GuestText(base, "C2_MENU_HINT", "Close the game");
    if (hint != 0) ctx.r3.u64 = hint;
  } else if (std::strcmp(id, "IDS_MULTIPLAYER_MENU") == 0) {
    static bool sent = false;
    if (!sent && on_quit_line) { sent = true; REXLOG_INFO("[MENU] Quit chosen on the main menu: closing the game"); EnumWindows(CloseOurs, 0); }
  }
}
