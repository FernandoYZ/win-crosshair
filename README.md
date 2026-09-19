# crosshair

A minimal crosshair overlay for Windows, written in C with the Win32 API and GDI.

**Status: v1.0.0 (stable).** It draws a configurable cross, with an optional
outline, in the center of the selected monitor. The window is topmost,
click-through, never takes focus, is hidden from Alt+Tab and is DPI-aware. It
recenters itself when the desktop layout changes, works from folders with any
characters in their name, and is self-contained: the C runtime is linked
statically, so it needs no Visual C++ Redistributable. See `docs/ROADMAP.md` for
what may come next.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The result is `build\Release\crosshair.exe`.

## Run

```powershell
.\crosshair.exe
```

There is no window to close. Stop it with Task Manager or:

```powershell
taskkill /IM crosshair.exe
```

## Using it with games

The crosshair is a regular window drawn over the desktop. It shows over a game
whenever Windows composes that game together with the desktop, which is the case
for **windowed** and **borderless** ("fullscreen windowed") display modes.

It does **not** show over a game in **exclusive fullscreen**, where the game takes
over the display. If the crosshair is missing in a game:

1. Open the game's video settings and switch the display mode to windowed,
   borderless or "fullscreen windowed".
2. If the game has no such option, look for a borderless-window launch option in
   its documentation or community guides.
3. Set the game to the monitor's native resolution, so borderless looks the same
   as fullscreen.

There is no workaround from this project. The tools that draw over exclusive
fullscreen do it by injecting code into the game, and this project deliberately
never does: it does not read or modify game memory, inject DLLs or hook graphics
APIs. Doing so can also get an account banned by the game's anti-cheat.

Games and anti-cheat systems have different policies about external overlays. Check
the rules of the game you play before using one.

## Configuration

`crosshair.exe` reads `config.toml` from **the same folder as the executable**
(not the working directory). If the file does not exist, defaults are used, so
the file is optional. Edit it, then restart `crosshair.exe`.

```toml
[crosshair]
color = "#00FF00"          # "#RRGGBB"
size = 12                  # length of each arm in pixels (1-500)
thickness = 2              # arm thickness in pixels (1-100)
gap = 4                    # distance from the center to each arm (0-500)
opacity = 255              # 0 (invisible) to 255 (opaque)

outline = true             # true or false
outline_color = "#000000"  # "#RRGGBB"
outline_thickness = 1      # extra pixels around every arm (0-50)

[display]
monitor = 0                # 0 = primary, 1 = second monitor, 2 = third...
```

The repository's `config.toml` holds the defaults. Copy it next to the built
`.exe` to start from it:

```powershell
Copy-Item config.toml build\Release\
```

If a value is invalid, an error dialog names the property and the line, and the
program exits without drawing anything.

### Sizes and DPI

The process is per-monitor DPI-aware (`PerMonitorV2`), so Windows never rescales
it. Every size in the file is in **physical pixels**: `size = 12` is 12 pixels on
screen at 100 % and at 200 % scaling alike. This matches how games render, and it
keeps the crosshair centered at any scale.

### Outline

The outline grows every arm by `outline_thickness` pixels on all sides. It is
drawn before the arms, so it never covers a neighbouring arm. The window grows
with it, so the outline is never clipped. The outline uses the same `opacity` as
the crosshair.

### Monitors

`0` is always the primary monitor. `1`, `2`... are the other monitors in the
order Windows enumerates them, which is not necessarily left-to-right. If the
chosen monitor does not exist, a warning dialog is shown and the primary monitor
is used.

### Supported syntax

This is **not a full TOML parser**. It accepts only:

- the sections `[crosshair]` and `[display]`, in any order;
- `key = value` lines: integers, `true`/`false`, and colors as quoted strings;
- `#` comments, on their own line or after a value;
- blank lines, CRLF or LF line endings, and a UTF-8 byte order mark.

Anything else is rejected, including unknown properties and sections, properties
placed in the wrong section, and `#RRGGBBAA` colors (use `#RRGGBB` plus
`opacity`). The file must be 4096 bytes or smaller. `thickness` cannot exceed
`2 * (gap + size)`, or the cross would be clipped.

Odd thicknesses cannot be centered exactly on the pixel grid, so the arms sit
half a pixel off-center.

### Desktop changes

When the resolution changes (for example when a game switches modes) or a monitor
is plugged in or removed, Windows sends `WM_DISPLAYCHANGE` and the crosshair
recenters on the configured monitor, or on the primary one if that monitor is gone.
It reacts to the event; nothing polls.

## Compatibility

What has actually been checked, so nothing here is a guess:

| Area | Checked | Not checked yet |
|---|---|---|
| Windows | Windows 11 | Windows 10 |
| Resolution | 1920x1080 | 1280x720, 2560x1440, 3840x2160 |
| DPI scaling | 125 % (window size and centering exact in physical pixels) | 100 %, 150 %, 175 %, 200 % |
| Monitors | one monitor; a missing monitor falls back to the primary | two or three monitors |
| Game display mode | a game in windowed / borderless mode shows the crosshair | - |
| Fullscreen exclusive | does not show it (documented limitation) | - |
| Non-ANSI folder names | works (tested with a Cyrillic folder) | - |
| Recentering | simulated by sending `WM_DISPLAYCHANGE` after moving the window | a real resolution change |

## Performance

Measured with `scripts/measure.ps1` (median of 5 runs, 10 s idle window, default
`config.toml`) on Windows 11, Intel Core i7-1165G7, 12 GB RAM, 1920x1080 at 125 %:

| Metric | v0.5.0 | v0.6.0 |
|---|---|---|
| Executable size | 19,968 bytes | 171,520 bytes |
| RAM, private working set | 740 KB | 80 KB |
| RAM, committed | 1,112 KB | 1,108 KB |
| CPU while idle (10 s) | 0 ms | 0 ms |
| GPU | 0 % | 0 % |
| Startup until the window is visible | 23 ms | 25 ms |
| Runtime DLLs to install | `VCRUNTIME140.dll` | none |

How to read it:

- **RAM.** The private working set is what Task Manager shows as "Memory". It
  dropped because, once the crosshair is drawn, the program asks Windows to take
  its pages out of the working set. They stay cached and return on demand, so the
  process stops counting as resident memory while it idles. The **committed**
  memory did not change: the program does not need less memory, it just stops
  holding it resident. Task Manager may show a slightly higher number than the
  script, because the working set grows a little each time the crosshair repaints.
- **Size.** It grew by about 150 KB because the C runtime is now inside the
  executable. That is what removes the `VCRUNTIME140.dll` dependency. Compiler
  flags such as `/O1` or link-time optimization changed nothing measurable.
- **Startup** includes creating the process from PowerShell, and the first launch
  of a new binary can be slower because of the antivirus scan. Read it as an upper
  bound.

To measure a build yourself:

```powershell
.\scripts\measure.ps1 -Exe build\Release\crosshair.exe
```

## Tests

The config parser is plain C and has its own tests:

```powershell
cmake -S . -B build -DCROSSHAIR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Known limitations

- Requires Windows 10 version 1703 or later (per-monitor DPI awareness v2).
- Topmost is requested once at creation. Another topmost window, or a fullscreen
  app, can still cover the crosshair.
- **Fullscreen exclusive games will not show the overlay.** In that mode the game
  owns the display and Windows does not composite other windows over it. Use
  "windowed", "borderless" or "fullscreen windowed" in the game instead. The
  project never injects into or hooks games, so there is no workaround from here.
- `cross` shape only.
- Running `crosshair.exe` twice draws two crosshairs on top of each other. Stop the
  extra one from Task Manager.
- Selecting a monitor other than the primary has only been tested on a
  single-monitor machine (the fallback path).
