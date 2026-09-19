#include <windows.h>

#include "overlay.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdline, int show)
{
    (void)prev;
    (void)cmdline;
    (void)show;

    if (!overlay_create(instance)) {
        MessageBox(NULL, "Failed to create overlay window.", "crosshair", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
