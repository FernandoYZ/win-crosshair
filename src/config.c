#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_SIZE 4096

typedef enum { FIELD_INT, FIELD_BOOL, FIELD_COLOR } FieldType;

/* Every supported property. `lo`/`hi` only apply to integers. */
static const struct {
    const char *section;
    const char *key;
    FieldType type;
    size_t offset;
    int lo, hi;
} FIELDS[] = {
    { "crosshair", "color",             FIELD_COLOR, offsetof(Config, color),             0,   0 },
    { "crosshair", "size",              FIELD_INT,   offsetof(Config, size),              1, 500 },
    { "crosshair", "thickness",         FIELD_INT,   offsetof(Config, thickness),         1, 100 },
    { "crosshair", "gap",               FIELD_INT,   offsetof(Config, gap),               0, 500 },
    { "crosshair", "opacity",           FIELD_INT,   offsetof(Config, opacity),           0, 255 },
    { "crosshair", "outline",           FIELD_BOOL,  offsetof(Config, outline),           0,   0 },
    { "crosshair", "outline_color",     FIELD_COLOR, offsetof(Config, outline_color),     0,   0 },
    { "crosshair", "outline_thickness", FIELD_INT,   offsetof(Config, outline_thickness), 0,  50 },
    { "display",   "monitor",           FIELD_INT,   offsetof(Config, monitor),           0, 255 },
};

#define FIELD_COUNT (sizeof FIELDS / sizeof FIELDS[0])

void config_defaults(Config *cfg)
{
    cfg->color = (Rgb){ 0, 255, 0 };
    cfg->size = 12;
    cfg->thickness = 2;
    cfg->gap = 4;
    cfg->opacity = 255;
    cfg->outline = true;
    cfg->outline_color = (Rgb){ 0, 0, 0 };
    cfg->outline_thickness = 1;
    cfg->monitor = 0;
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

static bool parse_color(char *value, const char *key, Rgb *out, char *err, size_t err_size, int line)
{
    char *s = unquote(value);
    if (!s) {
        return fail(err, err_size, line,
                    "invalid value for '%s': expected a quoted string like \"#RRGGBB\".", key);
    }
    if (s[0] == '#' && strlen(s) == 9) {
        return fail(err, err_size, line,
                    "invalid value for '%s': #RRGGBBAA is not supported, "
                    "use \"#RRGGBB\" and the 'opacity' property.", key);
    }

    int d[6];
    bool ok = (s[0] == '#' && strlen(s) == 7);
    for (int i = 0; ok && i < 6; i++) {
        d[i] = hex_value(s[i + 1]);
        ok = d[i] >= 0;
    }
    if (!ok) {
        return fail(err, err_size, line,
                    "invalid value for '%s': expected the format \"#RRGGBB\".", key);
    }

    out->r = (unsigned char)(d[0] * 16 + d[1]);
    out->g = (unsigned char)(d[2] * 16 + d[3]);
    out->b = (unsigned char)(d[4] * 16 + d[5]);
    return true;
}

/* Returns the canonical section name from FIELDS, or NULL if unknown. */
static const char *find_section(const char *name)
{
    for (size_t i = 0; i < FIELD_COUNT; i++) {
        if (strcmp(FIELDS[i].section, name) == 0) {
            return FIELDS[i].section;
        }
    }
    return NULL;
}

static bool apply(Config *cfg, const char *section, const char *key, char *value,
                  char *err, size_t err_size, int line)
{
    for (size_t i = 0; i < FIELD_COUNT; i++) {
        if (strcmp(FIELDS[i].section, section) != 0 || strcmp(FIELDS[i].key, key) != 0) {
            continue;
        }

        char *dest = (char *)cfg + FIELDS[i].offset;
        switch (FIELDS[i].type) {
        case FIELD_COLOR:
            return parse_color(value, key, (Rgb *)dest, err, err_size, line);
        case FIELD_BOOL:
            if (strcmp(value, "true") == 0) {
                *(bool *)dest = true;
            } else if (strcmp(value, "false") == 0) {
                *(bool *)dest = false;
            } else {
                return fail(err, err_size, line,
                            "invalid value for '%s': expected true or false.", key);
            }
            return true;
        case FIELD_INT: {
            int v;
            if (!parse_int(value, FIELDS[i].lo, FIELDS[i].hi, &v)) {
                return fail(err, err_size, line,
                            "invalid value for '%s': expected an integer between %d and %d.",
                            key, FIELDS[i].lo, FIELDS[i].hi);
            }
            *(int *)dest = v;
            return true;
        }
        }
    }

    return fail(err, err_size, line, "unknown property '%s' in [%s].", key, section);
}

bool config_parse(char *text, Config *cfg, char *err, size_t err_size)
{
    const char *section = NULL;
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
            section = find_section(name);
            if (!section) {
                return fail(err, err_size, line_no, "unknown section '[%s]'.", name);
            }
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
        if (!section) {
            return fail(err, err_size, line_no,
                        "property '%s' must be inside a section such as [crosshair].", key);
        }
        if (!apply(cfg, section, key, value, err, err_size, line_no)) {
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
