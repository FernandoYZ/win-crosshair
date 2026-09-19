#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "config.h"
#include "overlay.h"

static void show_message(const char *msg, UINT icon)
{
    MessageBox(NULL, msg, "crosshair", MB_OK | icon);
}

/* config.toml is looked up next to crosshair.exe, not in the working
 * directory, so the program stays portable. */
static BOOL config_path(char *out, size_t size)
{
    char exe[MAX_PATH];
    DWORD len = GetModuleFileName(NULL, exe, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return FALSE;
    }

    char *slash = strrchr(exe, '\\');
    if (slash == NULL) {
        return FALSE;
    }

    int n = snprintf(out, size, "%.*sconfig.toml", (int)(slash - exe + 1), exe);
    return n > 0 && (size_t)n < size;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show)
{
    (void)prev;
    (void)cmdline;
    (void)show;

    /* Must run before any window exists. With per-monitor awareness Windows
     * never scales our coordinates: sizes and positions are physical pixels. */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    char path[MAX_PATH];
    char err[256];
    char msg[MAX_PATH + 300];
    Config cfg;

    if (!config_path(path, sizeof path)) {
        show_message("Failed to locate config.toml.", MB_ICONERROR);
        return 1;
    }
    if (!config_load(path, &cfg, err, sizeof err)) {
        snprintf(msg, sizeof msg, "%s\n\nFile: %s", err, path);
        show_message(msg, MB_ICONERROR);
        return 1;
    }

    RECT monitor;
    if (!overlay_monitor_rect(cfg.monitor, &monitor)) {
        snprintf(msg, sizeof msg, "Monitor %d not found. Using the primary monitor.", cfg.monitor);
        show_message(msg, MB_ICONWARNING);
        if (!overlay_monitor_rect(0, &monitor)) {
            show_message("Failed to find the primary monitor.", MB_ICONERROR);
            return 1;
        }
    }

    if (!overlay_create(instance, &cfg, &monitor)) {
        show_message("Failed to create overlay window.", MB_ICONERROR);
        return 1;
    }

    MSG m;
    while (GetMessage(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return (int)m.wParam;
}
