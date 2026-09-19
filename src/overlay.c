#include "overlay.h"

/* v0.1.0: every value is hardcoded. Configuration arrives in v0.3.0. */
#define CROSS_COLOR RGB(0, 255, 0)
#define CROSS_SIZE 12
#define CROSS_THICKNESS 2
#define CROSS_GAP 4

/* Pixels painted with this color become transparent (LWA_COLORKEY). */
#define KEY_COLOR RGB(255, 0, 255)

/* The window is only as big as the crosshair, not the whole screen. */
#define HALF_EXTENT (CROSS_GAP + CROSS_SIZE)
#define WINDOW_SIDE (2 * HALF_EXTENT)

static const char CLASS_NAME[] = "crosshair_overlay";

static void fill_rect(HDC dc, HBRUSH brush, int left, int top, int right, int bottom)
{
    RECT rc = { left, top, right, bottom };
    FillRect(dc, &rc, brush);
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    HBRUSH key = CreateSolidBrush(KEY_COLOR);
    HBRUSH cross = CreateSolidBrush(CROSS_COLOR);

    const int c = HALF_EXTENT;
    const int t0 = c - CROSS_THICKNESS / 2;
    const int t1 = t0 + CROSS_THICKNESS;

    fill_rect(dc, key, 0, 0, WINDOW_SIDE, WINDOW_SIDE);

    /* horizontal arms */
    fill_rect(dc, cross, c - CROSS_GAP - CROSS_SIZE, t0, c - CROSS_GAP, t1);
    fill_rect(dc, cross, c + CROSS_GAP, t0, c + CROSS_GAP + CROSS_SIZE, t1);
    /* vertical arms */
    fill_rect(dc, cross, t0, c - CROSS_GAP - CROSS_SIZE, t1, c - CROSS_GAP);
    fill_rect(dc, cross, t0, c + CROSS_GAP, t1, c + CROSS_GAP + CROSS_SIZE);

    DeleteObject(cross);
    DeleteObject(key);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_PAINT:
        paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1; /* paint() covers the whole client area */
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

BOOL overlay_create(HINSTANCE instance)
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    int x = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_SIDE) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_SIDE) / 2;

    HWND hwnd = CreateWindowEx(WS_EX_LAYERED, CLASS_NAME, "crosshair", WS_POPUP,
                               x, y, WINDOW_SIDE, WINDOW_SIDE,
                               NULL, NULL, instance, NULL);
    if (hwnd == NULL) {
        return FALSE;
    }

    if (!SetLayeredWindowAttributes(hwnd, KEY_COLOR, 0, LWA_COLORKEY)) {
        DestroyWindow(hwnd);
        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return TRUE;
}
