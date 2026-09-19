#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include "config.h"
#include "overlay.h"

#define CONFIG_NAME L"config.toml"

static void show_message(const wchar_t *msg, UINT icon)
{
    MessageBoxW(NULL, msg, L"crosshair", MB_OK | icon);
}

/* config.toml is looked up next to crosshair.exe, not in the working
 * directory, so the program stays portable. Paths are UTF-16 end to end:
 * a folder name outside the ANSI code page would not survive a narrow path. */
static BOOL config_path(wchar_t *out, size_t count)
{
    wchar_t exe[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return FALSE;
    }

    wchar_t *slash = wcsrchr(exe, L'\\');
    if (slash == NULL) {
        return FALSE;
    }
    slash[1] = L'\0';

    if (wcslen(exe) + wcslen(CONFIG_NAME) >= count) {
        return FALSE;
    }
    wcscpy(out, exe);
    wcscat(out, CONFIG_NAME);
    return TRUE;
}

static void show_config_error(const char *err, const wchar_t *path)
{
    wchar_t text[256] = L"";
    wchar_t msg[256 + MAX_PATH + 16];

    /* The parser reports UTF-8: it can echo back property names from the file. */
    MultiByteToWideChar(CP_UTF8, 0, err, -1, text, (int)(sizeof text / sizeof text[0]));
    _snwprintf(msg, sizeof msg / sizeof msg[0], L"%ls\n\nFile: %ls", text, path);
    msg[sizeof msg / sizeof msg[0] - 1] = L'\0';
    show_message(msg, MB_ICONERROR);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show)
{
    (void)prev;
    (void)cmdline;
    (void)show;

    /* Must run before any window exists. With per-monitor awareness Windows
     * never scales our coordinates: sizes and positions are physical pixels. */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    wchar_t path[MAX_PATH];
    char err[256];
    Config cfg;

    if (!config_path(path, sizeof path / sizeof path[0])) {
        show_message(L"Failed to locate config.toml.", MB_ICONERROR);
        return 1;
    }
    if (!config_load_w(path, &cfg, err, sizeof err)) {
        show_config_error(err, path);
        return 1;
    }

    RECT monitor;
    if (!overlay_monitor_rect(cfg.monitor, &monitor)) {
        wchar_t msg[96];
        _snwprintf(msg, sizeof msg / sizeof msg[0],
                   L"Monitor %d not found. Using the primary monitor.", cfg.monitor);
        msg[sizeof msg / sizeof msg[0] - 1] = L'\0';
        show_message(msg, MB_ICONWARNING);
        if (!overlay_monitor_rect(0, &monitor)) {
            show_message(L"Failed to find the primary monitor.", MB_ICONERROR);
            return 1;
        }
    }

    if (!overlay_create(instance, &cfg, &monitor)) {
        show_message(L"Failed to create overlay window.", MB_ICONERROR);
        return 1;
    }

    /* The crosshair is drawn and the process is about to sit idle. Ask Windows to
     * take its pages out of the working set: they stay cached and come back on
     * demand, so the process stops counting as resident memory while it waits.
     * The committed memory does not change. */
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    MSG m;
    while (GetMessage(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return (int)m.wParam;
}
