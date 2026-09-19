#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "config.h"
#include "overlay.h"

static void show_error(const char *msg)
{
    MessageBox(NULL, msg, "crosshair", MB_OK | MB_ICONERROR);
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

    char path[MAX_PATH];
    char err[256];
    char msg[MAX_PATH + 300];
    Config cfg;

    if (!config_path(path, sizeof path)) {
        show_error("Failed to locate config.toml.");
        return 1;
    }
    if (!config_load(path, &cfg, err, sizeof err)) {
        snprintf(msg, sizeof msg, "%s\n\nFile: %s", err, path);
        show_error(msg);
        return 1;
    }

    if (!overlay_create(instance, &cfg)) {
        show_error("Failed to create overlay window.");
        return 1;
    }

    MSG m;
    while (GetMessage(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return (int)m.wParam;
}
