# Quit on the main menu, in Multiplayer's place (2026-11-12, Opus)

Tefa: *"we need to do the same on Condemned 2"* (a quit in the main menu, as The Darkness got), then *"ah no use
multiplayer please"*, and *"not sure if instant action is online or just a game feature"*.

## Result

**Build b003 (installed in `build-local`): the main menu's second line reads "Quit" in the game's own font, its hint
line says "Close the game", and choosing it closes the game the clean way** (the port's own "Window closing, shutting
down" path) `[verified-live 2026-11-12, n=2, simulator + pretend controllers]`. Instant Action is untouched and is
online-only: choosing it says "This gamer profile is not signed in to Xbox LIVE" `[verified-live 2026-11-12, n=1]`.
Code: `dev-archive/port-project/src/vr_menu_quit.cpp` (`C2_MENU_QUIT=0` turns it off; `C2_MENU_WORD`, `C2_MENU_HINT`
change the words; `C2_MENU_LOG=1` logs every text id the game looks up).

## How it works

Every menu word goes through one text lookup in the game (`0x82324238`: r3 = the text id as a guest `char*`, returns
the text). Logged live, in order: `IDS_MULTIPLAYER` builds the line, `IDS_MULTIPLAYER_TOOLTIP` is its hint when
highlighted, and `IDS_MULTIPLAYER_MENU` (the Multiplayer page's title) is looked up the moment A is pressed on the
line, never before `[measured 2026-11-12]`. The hook hands back our own words (kept in the SDK's system heap) for the
first two and closes the game (WM_CLOSE to its own window) on the third.

## Dead ends (kept so nobody walks them again)

- **The Flash handlers.** The menus call named game handlers from a table of `{name, code, kind<<24}` at guest
  `0x829348f0..0x82935200` (`Global.Exit` -> `0x822F2F40`, `Global.Localize` -> `0x822F34A0`, `Global.OnConstruct` ->
  `0x822F31D0`, the `Multiplayer.*` ones). None runs when Multiplayer is chosen: `Multiplayer.OnConstruct` runs at
  start, `IsConnectedOnline` / `OnlineConnectionRequired` while the line is only highlighted; the page opens inside the
  menu's own script. `Global.Localize` is never called by the main menu.
- **The platform number.** The menu script adds an `exit` line for the PC platform. `Global.OnConstruct` sets
  `_global.g_nMonolithGlobalPlatform` from a double in the program's data (`0x82010EB0` = 2.0, Xbox 360); that data is
  write-protected (writing it stopped the game). Calling the game's own setter again: 0 = no change, 1 = one more line
  reading "undefined" which is the developers' **Custom level** page, not an exit, 3 = the PS3 legal screen at start
  `[measured 2026-11-12]`. Kept as an opt-in probe (`C2_MENU_PLATFORM`).
- **Rewriting the text table in memory** (`"Multiplayer"` at a heap address, same length padded with spaces) also
  shows "Quit" once the menu is rebuilt, but the address is a heap one; the lookup hook needs no address.
