#include "overlay.h"

/* Extended styles that make this a real overlay:
 *   LAYERED     per-pixel transparency (color key) and window opacity
 *   TRANSPARENT mouse input passes through to the window underneath
 *   NOACTIVATE  never takes focus or becomes the active window
 *   TOOLWINDOW  hidden from Alt+Tab and the taskbar
 *   TOPMOST     stays above normal windows; set once, no SetWindowPos polling */
#define OVERLAY_EX_STYLE \
    (WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)

static const char CLASS_NAME[] = "crosshair_overlay";

/* State of the single overlay window, set once by overlay_create(). */
static Config g_cfg;
static COLORREF g_cross_color;
static COLORREF g_key_color; /* pixels of this color become transparent */
static int g_side;           /* window width and height, in pixels */

/* The key color must differ from the crosshair color, or the crosshair
 * itself would turn transparent. */
static COLORREF pick_key_color(COLORREF cross)
{
    return cross == RGB(255, 0, 255) ? RGB(0, 255, 255) : RGB(255, 0, 255);
}

static void fill_rect(HDC dc, HBRUSH brush, int left, int top, int right, int bottom)
{
    RECT rc = { left, top, right, bottom };
    FillRect(dc, &rc, brush);
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    HBRUSH key = CreateSolidBrush(g_key_color);
    HBRUSH cross = CreateSolidBrush(g_cross_color);

    const int c = g_side / 2;
    const int t0 = c - g_cfg.thickness / 2;
    const int t1 = t0 + g_cfg.thickness;
    const int gap_lo = c - g_cfg.gap; /* inner end of the top/left arms */
    const int gap_hi = c + g_cfg.gap; /* inner end of the bottom/right arms */

    fill_rect(dc, key, 0, 0, g_side, g_side);

    /* horizontal arms */
    fill_rect(dc, cross, gap_lo - g_cfg.size, t0, gap_lo, t1);
    fill_rect(dc, cross, gap_hi, t0, gap_hi + g_cfg.size, t1);
    /* vertical arms */
    fill_rect(dc, cross, t0, gap_lo - g_cfg.size, t1, gap_lo);
    fill_rect(dc, cross, t0, gap_hi, t1, gap_hi + g_cfg.size);

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

BOOL overlay_create(HINSTANCE instance, const Config *cfg)
{
    g_cfg = *cfg;
    g_cross_color = RGB(cfg->r, cfg->g, cfg->b);
    g_key_color = pick_key_color(g_cross_color);
    /* The window is only as big as the crosshair, not the whole screen. */
    g_side = 2 * (cfg->gap + cfg->size);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    int x = (GetSystemMetrics(SM_CXSCREEN) - g_side) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - g_side) / 2;

    HWND hwnd = CreateWindowEx(OVERLAY_EX_STYLE, CLASS_NAME, "crosshair", WS_POPUP,
                               x, y, g_side, g_side,
                               NULL, NULL, instance, NULL);
    if (hwnd == NULL) {
        return FALSE;
    }

    if (!SetLayeredWindowAttributes(hwnd, g_key_color, (BYTE)cfg->opacity,
                                    LWA_COLORKEY | LWA_ALPHA)) {
        DestroyWindow(hwnd);
        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);
    return TRUE;
}
