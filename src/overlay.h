#ifndef CROSSHAIR_OVERLAY_H
#define CROSSHAIR_OVERLAY_H

#include <windows.h>

#include "config.h"

/* Finds monitor `index` (0 = primary, 1.. = the others in enumeration order)
 * and stores its full bounds, in physical desktop pixels, in `rect`.
 * Returns FALSE if there is no such monitor. */
BOOL overlay_monitor_rect(int index, RECT *rect);

/* Creates and shows the crosshair window centered on `monitor`.
 * Returns FALSE if the window class or the window could not be created. */
BOOL overlay_create(HINSTANCE instance, const Config *cfg, const RECT *monitor);

#endif
