# crosshair

A minimal crosshair overlay for Windows, written in C with the Win32 API and GDI.

**Status: v0.2.0 (real overlay).** It draws a fixed green cross in the center of
the primary monitor. The window is topmost, click-through, never takes focus and
is hidden from Alt+Tab. Nothing is configurable yet; see `docs/ROADMAP.md`.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
.\crosshair.exe
```

There is no window to close. Stop it with Task Manager or:

```powershell
taskkill /IM crosshair.exe
```

## Known limitations in v0.2.0

- Topmost is requested once at creation. Another topmost window (or a
  fullscreen app) can still cover the crosshair.
- Not DPI-aware; position may be off on scaled displays (planned for v0.4.0).
- Primary monitor only.
- Fullscreen exclusive games may not show the overlay. This is a limitation of
  external Win32 overlays; the project never injects into or hooks games.
