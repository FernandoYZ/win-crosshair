#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_SIZE 4096

/* Integer properties of [crosshair] and their accepted range. */
static const struct {
    const char *key;
    size_t offset;
    int lo, hi;
} INT_FIELDS[] = {
    { "size",      offsetof(Config, size),      1, 500 },
    { "thickness", offsetof(Config, thickness), 1, 100 },
    { "gap",       offsetof(Config, gap),       0, 500 },
    { "opacity",   offsetof(Config, opacity),   0, 255 },
};

void config_defaults(Config *cfg)
{
    cfg->r = 0;
    cfg->g = 255;
    cfg->b = 0;
    cfg->size = 12;
    cfg->thickness = 2;
    cfg->gap = 4;
    cfg->opacity = 255;
}

/* Always returns false so callers can write `return fail(...)`.
 * `line` is 1-based; 0 means the error is not tied to a line. */
static bool fail(char *err, size_t err_size, int line, const char *fmt, ...)
{
    char msg[192];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    if (line > 0) {
        snprintf(err, err_size, "config.toml, line %d: %s", line, msg);
    } else {
        snprintf(err, err_size, "config.toml: %s", msg);
    }
    return false;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
    return s;
}

/* Cuts the line at the first '#' that is not inside a string. */
static void strip_comment(char *s)
{
    bool in_string = false;
    for (; *s; s++) {
        if (*s == '"') {
            in_string = !in_string;
        } else if (*s == '#' && !in_string) {
            *s = '\0';
            return;
        }
    }
}

/* Returns the text between the surrounding quotes, or NULL if `s` is not a
 * simple quoted string. Modifies `s` in place. */
static char *unquote(char *s)
{
    size_t len = strlen(s);
    if (len < 2 || s[0] != '"' || s[len - 1] != '"') {
        return NULL;
    }
    s[len - 1] = '\0';
    return strchr(s + 1, '"') ? NULL : s + 1;
}

static bool parse_int(const char *s, int lo, int hi, int *out)
{
    const char *digits = (*s == '-') ? s + 1 : s;
    if (*digits == '\0') {
        return false;
    }
    for (const char *p = digits; *p; p++) {
        if (!isdigit((unsigned char)*p)) {
            return false;
        }
    }

    errno = 0;
    long v = strtol(s, NULL, 10);
    if (errno == ERANGE || v < lo || v > hi) {
        return false;
    }
    *out = (int)v;
    return true;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool parse_color(char *value, Config *cfg, char *err, size_t err_size, int line)
{
    char *s = unquote(value);
    if (!s) {
        return fail(err, err_size, line,
                    "invalid value for 'color': expected a quoted string like \"#RRGGBB\".");
    }
    if (s[0] == '#' && strlen(s) == 9) {
        return fail(err, err_size, line,
                    "invalid value for 'color': #RRGGBBAA is not supported, "
                    "use \"#RRGGBB\" and the 'opacity' property.");
    }

    int d[6];
    bool ok = (s[0] == '#' && strlen(s) == 7);
    for (int i = 0; ok && i < 6; i++) {
        d[i] = hex_value(s[i + 1]);
        ok = d[i] >= 0;
    }
    if (!ok) {
        return fail(err, err_size, line,
                    "invalid value for 'color': expected the format \"#RRGGBB\".");
    }

    cfg->r = (unsigned char)(d[0] * 16 + d[1]);
    cfg->g = (unsigned char)(d[2] * 16 + d[3]);
    cfg->b = (unsigned char)(d[4] * 16 + d[5]);
    return true;
}

static bool apply(Config *cfg, const char *key, char *value, char *err, size_t err_size, int line)
{
    if (strcmp(key, "color") == 0) {
        return parse_color(value, cfg, err, err_size, line);
    }

    for (size_t i = 0; i < sizeof INT_FIELDS / sizeof INT_FIELDS[0]; i++) {
        if (strcmp(key, INT_FIELDS[i].key) != 0) {
            continue;
        }
        int v;
        if (!parse_int(value, INT_FIELDS[i].lo, INT_FIELDS[i].hi, &v)) {
            return fail(err, err_size, line,
                        "invalid value for '%s': expected an integer between %d and %d.",
                        key, INT_FIELDS[i].lo, INT_FIELDS[i].hi);
        }
        *(int *)((char *)cfg + INT_FIELDS[i].offset) = v;
        return true;
    }

    return fail(err, err_size, line, "unknown property '%s'.", key);
}

bool config_parse(char *text, Config *cfg, char *err, size_t err_size)
{
    bool in_crosshair = false;
    int line_no = 0;
    char *next = text;

    while (*next) {
        char *line = next;
        char *nl = strchr(next, '\n');
        if (nl) {
            *nl = '\0';
            next = nl + 1;
        } else {
            next += strlen(next);
        }
        line_no++;

        strip_comment(line);
        line = trim(line);
        if (*line == '\0') {
            continue;
        }

        if (*line == '[') {
            size_t len = strlen(line);
            if (line[len - 1] != ']') {
                return fail(err, err_size, line_no, "malformed section header.");
            }
            line[len - 1] = '\0';
            char *name = trim(line + 1);
            if (strcmp(name, "crosshair") != 0) {
                return fail(err, err_size, line_no, "unknown section '[%s]'.", name);
            }
            in_crosshair = true;
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq) {
            return fail(err, err_size, line_no, "expected 'key = value'.");
        }
        *eq = '\0';
        char *key = trim(line);
        char *value = trim(eq + 1);
        if (*key == '\0') {
            return fail(err, err_size, line_no, "expected 'key = value'.");
        }
        if (*value == '\0') {
            return fail(err, err_size, line_no, "missing value for '%s'.", key);
        }
        if (!in_crosshair) {
            return fail(err, err_size, line_no,
                        "property '%s' must be inside a [crosshair] section.", key);
        }
        if (!apply(cfg, key, value, err, err_size, line_no)) {
            return false;
        }
    }

    if (cfg->thickness > 2 * (cfg->gap + cfg->size)) {
        return fail(err, err_size, 0,
                    "'thickness' (%d) is larger than 2 * ('gap' + 'size'); "
                    "the crosshair would be clipped.", cfg->thickness);
    }
    return true;
}

bool config_load(const char *path, Config *cfg, char *err, size_t err_size)
{
    config_defaults(cfg);

    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) {
            return true;
        }
        return fail(err, err_size, 0, "cannot open file: %s.", strerror(errno));
    }

    char buf[MAX_FILE_SIZE + 1];
    size_t n = fread(buf, 1, sizeof buf, f);
    bool read_failed = ferror(f) != 0;
    fclose(f);

    if (read_failed) {
        return fail(err, err_size, 0, "cannot read file.");
    }
    if (n > MAX_FILE_SIZE) {
        return fail(err, err_size, 0, "file is larger than %d bytes.", MAX_FILE_SIZE);
    }
    buf[n] = '\0';

    char *text = buf;
    if (n >= 3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0) {
        text += 3; /* UTF-8 byte order mark added by some editors */
    }
    return config_parse(text, cfg, err, err_size);
}
