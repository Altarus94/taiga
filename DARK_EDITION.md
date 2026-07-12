# Taiga — Dark Edition

This is a fork of [Taiga](https://github.com/erengy/taiga) that adds a **dark
mode** to the v1 (v1.4.1) desktop app. It is unofficial and not affiliated with
erengy; it exists because dark mode has been one of the most-requested Taiga
features for years ([#320](https://github.com/erengy/taiga/issues/320),
[#679](https://github.com/erengy/taiga/issues/679),
[#807](https://github.com/erengy/taiga/issues/807)) and was deferred to the
unreleased Qt v2 rewrite.

Grab a ready-to-run build from the [Releases](../../releases) page, or build
from the `dark-edition` branch. Everything else works exactly like stock Taiga.

## How it works

Taiga v1 paints its own UI with Win32/GDI, so the OS dark-mode setting does
nothing on its own. This fork adds a small `win::dark` module (in the
`deps/src/windows` submodule) and routes the app's colors and custom drawing
through it:

- Enables process-wide dark mode via the undocumented `uxtheme` APIs
  (`SetPreferredAppMode`, `AllowDarkModeForWindow`, …) and dark DWM title bars.
- Replaces `GetSysColor`/`GetSysColorBrush` with dark-aware versions and swaps
  the theme palette at startup (`ui::InitDarkColors`).
- Themes standard controls (ListView + header, TreeView, edit, combo box,
  scrollbars, status bar, tabs, rebar, toolbars, SysLink).
- Owner-draws the controls Windows won't recolor: group boxes, check boxes,
  radio buttons, the tab control, the status bar, and — because the list sends
  no custom-draw notification for them — **ListView group headers** (repainted
  over the default drawing).

Falls back to the normal light UI when dark mode is unavailable or under High
Contrast. The change is opt-out by nature: nothing else about Taiga changes.

## Credits

- Original [Taiga](https://github.com/erengy/taiga) and
  [windows](https://github.com/erengy/windows) library by **erengy** (GPL-3.0 /
  MIT respectively).
- Dark-mode techniques adapted from
  [ysc3839/win32-darkmode](https://github.com/ysc3839/win32-darkmode) and
  Notepad++'s `NppDarkMode`.
