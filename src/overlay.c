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
static COLORREF g_outline_color;
static COLORREF g_key_color; /* pixels of this color become transparent */
static int g_outline;        /* outline thickness in pixels, 0 when disabled */
static int g_side;           /* window width and height, in pixels */

static COLORREF to_colorref(Rgb c)
{
    return RGB(c.r, c.g, c.b);
}

/* The key color must differ from every color that is drawn, or that part of
 * the crosshair would turn transparent. */
static COLORREF pick_key_color(COLORREF a, COLORREF b)
{
    static const COLORREF candidates[] = { RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 0) };
    for (size_t i = 0; i < sizeof candidates / sizeof candidates[0]; i++) {
        if (candidates[i] != a && candidates[i] != b) {
            return candidates[i];
        }
    }
    return RGB(1, 2, 3); /* unreachable: three candidates, at most two excluded */
}

static void fill_rect(HDC dc, HBRUSH brush, int left, int top, int right, int bottom)
{
    RECT rc = { left, top, right, bottom };
    FillRect(dc, &rc, brush);
}

/* Fills the four arms, each grown by `grow` pixels on every side. */
static void draw_arms(HDC dc, HBRUSH brush, int grow)
{
    const int c = g_side / 2;
    const int t0 = c - g_cfg.thickness / 2 - grow;
    const int t1 = c - g_cfg.thickness / 2 + g_cfg.thickness + grow;
    const int gap_lo = c - g_cfg.gap; /* inner end of the top/left arms */
    const int gap_hi = c + g_cfg.gap; /* inner end of the bottom/right arms */
    const int len = g_cfg.size;

    /* horizontal arms */
    fill_rect(dc, brush, gap_lo - len - grow, t0, gap_lo + grow, t1);
    fill_rect(dc, brush, gap_hi - grow, t0, gap_hi + len + grow, t1);
    /* vertical arms */
    fill_rect(dc, brush, t0, gap_lo - len - grow, t1, gap_lo + grow);
    fill_rect(dc, brush, t0, gap_hi - grow, t1, gap_hi + len + grow);
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    HBRUSH key = CreateSolidBrush(g_key_color);
    HBRUSH cross = CreateSolidBrush(g_cross_color);

    fill_rect(dc, key, 0, 0, g_side, g_side);

    /* All outlines go first and all arms after, so an outline never covers
     * a neighboring arm. */
    if (g_outline > 0) {
        HBRUSH outline = CreateSolidBrush(g_outline_color);
        draw_arms(dc, outline, g_outline);
        DeleteObject(outline);
    }
    draw_arms(dc, cross, 0);

    DeleteObject(cross);
    DeleteObject(key);
    EndPaint(hwnd, &ps);
}

/* Top-left corner that centers the window on `monitor`. */
static void centered_origin(const RECT *monitor, int *x, int *y)
{
    *x = monitor->left + ((monitor->right - monitor->left) - g_side) / 2;
    *y = monitor->top + ((monitor->bottom - monitor->top) - g_side) / 2;
}

/* Runs when the desktop layout changes (resolution, monitor plugged in or
 * removed): the old position is no longer the center. Event-driven, so the
 * idle cost stays zero. */
static void recenter(HWND hwnd)
{
    RECT monitor;
    int x, y;

    /* If the chosen monitor is gone, use the primary one. */
    if (!overlay_monitor_rect(g_cfg.monitor, &monitor) && !overlay_monitor_rect(0, &monitor)) {
        return;
    }
    centered_origin(&monitor, &x, &y);
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_PAINT:
        paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1; /* paint() covers the whole client area */
    case WM_DISPLAYCHANGE:
        recenter(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

typedef struct {
    int target;
    int others_seen; /* non-primary monitors counted so far */
    RECT rect;
    BOOL found;
} MonitorSearch;

static BOOL CALLBACK monitor_proc(HMONITOR monitor, HDC dc, LPRECT clip, LPARAM param)
{
    MonitorSearch *search = (MonitorSearch *)param;
    MONITORINFO info = { sizeof info };
    (void)dc;
    (void)clip;

    if (!GetMonitorInfo(monitor, &info)) {
        return TRUE;
    }

    /* Index 0 is always the primary monitor; the others follow in the order
     * Windows enumerates them. */
    int index = (info.dwFlags & MONITORINFOF_PRIMARY) ? 0 : ++search->others_seen;
    if (index == search->target) {
        search->rect = info.rcMonitor;
        search->found = TRUE;
        return FALSE;
    }
    return TRUE;
}

BOOL overlay_monitor_rect(int index, RECT *rect)
{
    MonitorSearch search = { index, 0, { 0, 0, 0, 0 }, FALSE };
    EnumDisplayMonitors(NULL, NULL, monitor_proc, (LPARAM)&search);
    if (search.found) {
        *rect = search.rect;
    }
    return search.found;
}

BOOL overlay_create(HINSTANCE instance, const Config *cfg, const RECT *monitor)
{
    g_cfg = *cfg;
    g_outline = cfg->outline ? cfg->outline_thickness : 0;
    g_cross_color = to_colorref(cfg->color);
    g_outline_color = g_outline > 0 ? to_colorref(cfg->outline_color) : g_cross_color;
    g_key_color = pick_key_color(g_cross_color, g_outline_color);
    /* The window is only as big as the crosshair, not the whole screen. */
    g_side = 2 * (cfg->gap + cfg->size + g_outline);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    int x, y;
    centered_origin(monitor, &x, &y);

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
