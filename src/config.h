#ifndef CROSSHAIR_CONFIG_H
#define CROSSHAIR_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    unsigned char r, g, b;
    int size;      /* length of each arm, in pixels */
    int thickness; /* arm thickness, in pixels */
    int gap;       /* distance from the center to the start of each arm */
    int opacity;   /* 0 (invisible) to 255 (opaque) */
} Config;

void config_defaults(Config *cfg);

/* Parses the supported subset of TOML (see README) from a mutable,
 * NUL-terminated buffer. Overrides the fields present in `cfg`.
 * On failure returns false and writes a readable message to `err`. */
bool config_parse(char *text, Config *cfg, char *err, size_t err_size);

/* Loads `path` into `cfg`. A missing file is not an error: defaults are used. */
bool config_load(const char *path, Config *cfg, char *err, size_t err_size);

#endif
