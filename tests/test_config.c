#include <stdio.h>
#include <string.h>

#include "config.h"

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
            failures++;                                                    \
        }                                                                  \
    } while (0)

static bool parse(const char *src, Config *cfg, char *err, size_t err_size)
{
    char text[1024];
    snprintf(text, sizeof text, "%s", src);
    config_defaults(cfg);
    err[0] = '\0';
    return config_parse(text, cfg, err, err_size);
}

/* The config must be rejected and the message must name `needle`. */
static void expect_error(const char *src, const char *needle)
{
    Config cfg;
    char err[256];
    bool ok = parse(src, &cfg, err, sizeof err);
    if (ok || strstr(err, needle) == NULL) {
        printf("FAIL: expected error containing \"%s\" for:\n%s\n  got: %s\n",
               needle, src, ok ? "(accepted)" : err);
        failures++;
    }
}

static void test_defaults(void)
{
    Config cfg;
    config_defaults(&cfg);
    CHECK(cfg.r == 0 && cfg.g == 255 && cfg.b == 0);
    CHECK(cfg.size == 12 && cfg.thickness == 2 && cfg.gap == 4 && cfg.opacity == 255);
}

static void test_missing_file_uses_defaults(void)
{
    Config cfg;
    char err[256];
    CHECK(config_load("this-file-does-not-exist.toml", &cfg, err, sizeof err));
    CHECK(cfg.size == 12 && cfg.g == 255);
}

static void test_valid(void)
{
    Config cfg;
    char err[256];
    const char *src =
        "# comment\r\n"
        "[crosshair]\r\n"
        "color = \"#ff8000\"  # orange\r\n"
        "size = 20\r\n"
        "  thickness=3\r\n"
        "gap = 0\r\n"
        "opacity = 128\r\n";
    CHECK(parse(src, &cfg, err, sizeof err));
    CHECK(cfg.r == 255 && cfg.g == 128 && cfg.b == 0);
    CHECK(cfg.size == 20 && cfg.thickness == 3 && cfg.gap == 0 && cfg.opacity == 128);
}

static void test_partial_keeps_defaults(void)
{
    Config cfg;
    char err[256];
    CHECK(parse("[crosshair]\nsize = 30\n", &cfg, err, sizeof err));
    CHECK(cfg.size == 30 && cfg.thickness == 2 && cfg.opacity == 255);
}

static void test_bom_file(void)
{
    const char *path = "test_config_bom.tmp";
    Config cfg;
    char err[256];
    FILE *f = fopen(path, "wb");
    CHECK(f != NULL);
    if (!f) return;
    fputs("\xEF\xBB\xBF[crosshair]\nsize = 7\n", f);
    fclose(f);
    CHECK(config_load(path, &cfg, err, sizeof err));
    CHECK(cfg.size == 7);
    remove(path);
}

static void test_invalid(void)
{
    expect_error("[crosshair]\ncolor = \"green\"\n", "'color'");
    expect_error("[crosshair]\ncolor = #00FF00\n", "'color'");
    expect_error("[crosshair]\ncolor = \"#00FF0\"\n", "'color'");
    expect_error("[crosshair]\ncolor = \"#00GG00\"\n", "'color'");
    expect_error("[crosshair]\ncolor = \"#00FF00CC\"\n", "#RRGGBBAA");
    expect_error("[crosshair]\nsize = -10\n", "'size'");
    expect_error("[crosshair]\nsize = 0\n", "'size'");
    expect_error("[crosshair]\nsize = 501\n", "'size'");
    expect_error("[crosshair]\nsize = 12.5\n", "'size'");
    expect_error("[crosshair]\nsize = \"12\"\n", "'size'");
    expect_error("[crosshair]\nsize = 99999999999\n", "'size'");
    expect_error("[crosshair]\nthickness = 0\n", "'thickness'");
    expect_error("[crosshair]\ngap = -1\n", "'gap'");
    expect_error("[crosshair]\nopacity = 999\n", "'opacity'");
    expect_error("[crosshair]\nopacity = -1\n", "'opacity'");
}

static void test_structure_errors(void)
{
    expect_error("[crosshair]\nshape = \"cross\"\n", "unknown property 'shape'");
    expect_error("[display]\nmonitor = 0\n", "unknown section");
    expect_error("size = 12\n", "[crosshair] section");
    expect_error("[crosshair\nsize = 12\n", "malformed section");
    expect_error("[crosshair]\nsize\n", "key = value");
    expect_error("[crosshair]\nsize =\n", "missing value for 'size'");
    expect_error("[crosshair]\n= 12\n", "key = value");
    expect_error("[crosshair]\nsize = 12\nthickness = 1000\n", "'thickness'");
    expect_error("[crosshair]\nsize = 1\ngap = 0\nthickness = 5\n", "clipped");
}

static void test_error_reports_line(void)
{
    expect_error("[crosshair]\n\n# note\nsize = 0\n", "line 4");
}

int main(void)
{
    test_defaults();
    test_missing_file_uses_defaults();
    test_valid();
    test_partial_keeps_defaults();
    test_bom_file();
    test_invalid();
    test_structure_errors();
    test_error_reports_line();

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all config tests passed\n");
    return 0;
}
