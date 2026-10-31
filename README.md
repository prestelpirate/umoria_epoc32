# Umoria 5.7.15 for Psion EPOC32 (ER5)

A port of [Umoria](https://github.com/dungeons-of-moria/umoria) 5.7.15 to the Psion EPOC Release 5 machines
(Series 7, netBook, Series 5mx, Revo), built on Linux with
[psion_cpp_sdk_linux](https://github.com/static-void/psion_cpp_sdk_linux).

- Full-screen 80x24 (or larger) ASCII display, with the dungeon map in colour
  (terrain, items and monsters coloured by symbol; Umoria itself is monochrome)
- EPOC-style drop-down menus on the **Menu** key, built from Umoria's own command summaries
  (File / Items / Action / Magic / Info / Options / Help); they follow the original or roguelike keyset
- Extras bar icon
- No sound

## Builds

Each release has two builds:

| Build | Zip | Machines | Status |
|---|---|---|---|
| **StrongARM** | `umoria-5.7.15-epoc32-strongarm.zip` | Psion Series 7, netBook | **Tested** on a Series 7 |
| **ARM710** | `umoria-5.7.15-epoc32-arm710.zip` | Psion Series 5mx, Revo (and also Series 7 / netBook) | **Not tested** on a Psion 5. Use at your own risk. |

The StrongARM build uses ARMv4 instructions and will **not** run on a Series 5mx or Revo.
The ARM710 build runs on all ER5 machines but is a little slower on the StrongARM machines.

## Installing

1. Download the zip for your machine from the [Releases](../../releases) page.
2. Extract it **to the root of a CompactFlash card** (or copy its contents to the root of the C: drive).
   This creates `\System\Apps\Umoria\`, which holds `umoria.app`, `umoria.exe`, `umoria.rsc`,
   `umoria.aif`, `scores.dat` and the help/screen text in `data\`.
3. **Series 5mx / Revo only:** these machines do not have the EPOC C library (`estlib.dll`) in ROM.
   Install `stdlib.sis` (it's in the root of the ARM710 zip) using *Control panel → Add/remove* first.
   The Series 7 and netBook already have it, so the installer will say it is already installed.
4. Umoria appears on the Extras bar. Tap it to start.

The save file (`game.sav`) and the high scores (`scores.dat`) are kept in `\System\Apps\Umoria\` on the same drive.
When upgrading, keep your existing `scores.dat` rather than the empty one in the zip.

Start with the `.app` icon. The ER5 System screen cannot start `umoria.exe` directly; `umoria.app` is a small launcher that starts it.

### Keys

| Key | Action |
|---|---|
| Arrows | Move (sent as 8/2/4/6, or k/j/h/l with roguelike keys) |
| Fn + arrows (Home/End/PgUp/PgDn) | Diagonal moves (7/1/9/3, or y/b/u/n with roguelike keys) |
| Menu | Open the drop-down menu bar (←/→ menus, ↑/↓ items, Enter select, Esc/Menu close) |
| Esc | Escape |
| Ctrl + letter | Umoria control commands (e.g. Ctrl+X save and quit, Ctrl+P previous messages, Ctrl+R redraw) |

Choosing a menu item types that command's key. Use the menus at the main command prompt; at other prompts the key is
typed into the prompt.

## Building from source

Requirements: a working [psion_cpp_sdk_linux](https://github.com/static-void/psion_cpp_sdk_linux)
install (gcc 3.0 cross compiler, wine, makmake), Python 3 with Pillow (for the icon).

```sh
export SDK=/path/to/psion_cpp_sdk_linux
./build.sh                    # ARM710 build    -> $EPOCROOT/epoc32/release/marm/rel/umoria.exe
CPU=strongarm ./build.sh      # StrongARM build
launcher/build.sh umoria 0x10005a43   # umoria.app + umoria.rsc launcher (prebuilt copies are in launcher/)
./package.sh dist             # stage dist/System/Apps/Umoria for a CF card from the last build
```

`build.sh` symlinks this tree into `$EPOCROOT/umoria` because makmake needs the project under EPOCROOT.
Do a clean build when you switch CPU (`rm -rf $EPOCROOT/epoc32/build$EPOCROOT/umoria`).
`TRACE=1 ./build.sh` makes a crash-tracing build that writes `trace.txt` next to the exe on a CPU fault.

### Layout

| Path | Contents |
|---|---|
| `src/` | Umoria 5.7.15 sources, converted to C++98, with `__EPOC32__` branches in `headers.h`, `curses.h`, `ui_io.cpp`, `game.cpp` and `main.cpp` |
| `epoc/` | `epoc_curses.cpp` (curses subset, map colours, keys, menus) and `epocwin.cpp` (window-server display, font, menu drawing; shared with the Angband port) |
| `compat/` | `prelude.h` (forced include), `estring.h` (minimal `string` / `ostringstream` / `to_string`), `epoc_curses.h`, a fixed `va_start`, `stdint`, `snprintf`, crash tracing |
| `group/` | `umoria.mmp` makmake project file and `cxxflags.sed` (adds the C++ flags to the generated makefile) |
| `tools/` | The scripts that did the C++14 → C++98 conversion (already applied to `src/`) |
| `launcher/` | Generic `.app` launcher that starts `<name>.exe` from its own folder |
| `icon/` | Icon source image → 24/32/48 px EPOC icons → `umoria.aif` |
| `data/` | Umoria help and screen text (`*.in` files are filled in with the version by `package.sh`) |

### Porting notes

- **C++98:** Umoria 5.7 is C++14; the SDK's gcc 3.0 only knows C++98. `tools/` rewrote `auto`, range-based `for`,
  `enum class`, in-class member defaults, `Coord_t{y, x}`-style temporaries, `constexpr`, `nullptr` and raw strings;
  a few cases were fixed by hand. Globals no longer depend on C++ static constructors.
- **No C++ library:** EPOC has no libstdc++, so `compat/estring.h` supplies just the `string` features Umoria uses.
- **No console:** writing to stdout on EPOC opens the C library's own text console over the game, so the bell and the
  fatal-error message go through the game window instead.
- **va_start:** estlib's `va_start` is wrong when the last named argument is narrower than `int`, which gives an alignment
  fault (KERN-EXEC 3) on StrongARM. `compat/stdarg_e.h` replaces it with `__builtin_next_arg`.
- **Window server objects:** an exe must call `RFbsSession::Connect()` itself before using fonts, and must construct
  `RWindowGroup`/`RWindow` with the `RWsSession`.
- **Launching:** ER5 apps need a matching `.rsc` next to the `.app`, or the app fails with "not found".

## Licence

Umoria is copyright (c) 1981-86 Robert A. Koeneke, (c) 1987-94 James E. Wilson and the Umoria contributors,
and is distributed under the GNU GPL v3 or later (see `LICENSE`).
The EPOC port code in `epoc/`, `compat/`, `launcher/`, `tools/` and the build scripts is released under the same terms.
`stdlib.sis` is the redistributable EPOC C library from the Psion/Symbian ER5 C++ SDK.
