# crosshair

A minimal crosshair overlay for Windows, written in C with the Win32 API and GDI.

**Status: v0.3.0 (configuration).** It draws a configurable cross in the center of
the primary monitor. The window is topmost, click-through, never takes focus and
is hidden from Alt+Tab. See `docs/ROADMAP.md` for what comes next.

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

## Configuration

`crosshair.exe` reads `config.toml` from **the same folder as the executable**
(not the working directory). If the file does not exist, defaults are used, so
the file is optional. Edit it, then restart `crosshair.exe`.

```toml
[crosshair]
color = "#00FF00"  # "#RRGGBB"
size = 12          # length of each arm in pixels (1-500)
thickness = 2      # arm thickness in pixels (1-100)
gap = 4            # distance from the center to each arm (0-500)
opacity = 255      # 0 (invisible) to 255 (opaque)
```

The repository's `config.toml` holds the defaults. Copy it next to the built
`.exe` to start from it:

```powershell
Copy-Item config.toml build\Release\
```

If a value is invalid, an error dialog names the property and the line, and the
program exits without drawing anything.

### Supported syntax

This is **not a full TOML parser**. It accepts only:

- a single `[crosshair]` section, with properties inside it;
- `key = value` lines; integers, and the color as a quoted string;
- `#` comments, on their own line or after a value;
- blank lines, CRLF or LF line endings, and a UTF-8 byte order mark.

Anything else is rejected, including unknown properties and sections, and
`#RRGGBBAA` colors (use `#RRGGBB` plus `opacity`). The file must be 4096 bytes
or smaller. `thickness` cannot exceed `2 * (gap + size)`, or the cross would be
clipped.

Odd thicknesses cannot be centered exactly on the pixel grid, so the arms sit
half a pixel off-center.

## Tests

The config parser is plain C and has its own tests:

```powershell
cmake -S . -B build -DCROSSHAIR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Known limitations

- Topmost is requested once at creation. Another topmost window, or a fullscreen
  app, can still cover the crosshair.
- Not DPI-aware; the position may be off on scaled displays (planned for v0.4.0).
- Primary monitor only, `cross` shape only, no outline.
- Depends on `VCRUNTIME140.dll` (dynamic MSVC runtime); it may fail to start on a
  machine without the Microsoft Visual C++ Redistributable (x64).
- Fullscreen exclusive games may not show the overlay. This is a limitation of
  external Win32 overlays; the project never injects into or hooks games.
