#ifndef CROSSHAIR_OVERLAY_H
#define CROSSHAIR_OVERLAY_H

#include <windows.h>

#include "config.h"

/* Creates and shows the crosshair window centered on the primary monitor.
 * Returns FALSE if the window class or the window could not be created. */
BOOL overlay_create(HINSTANCE instance, const Config *cfg);

#endif
