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
    CHECK(cfg.color.r == 0 && cfg.color.g == 255 && cfg.color.b == 0);
    CHECK(cfg.size == 12 && cfg.thickness == 2 && cfg.gap == 4 && cfg.opacity == 255);
    CHECK(cfg.outline);
    CHECK(cfg.outline_color.r == 0 && cfg.outline_color.g == 0 && cfg.outline_color.b == 0);
    CHECK(cfg.outline_thickness == 1);
    CHECK(cfg.monitor == 0);
}

static void test_missing_file_uses_defaults(void)
{
    Config cfg;
    char err[256];
    CHECK(config_load("this-file-does-not-exist.toml", &cfg, err, sizeof err));
    CHECK(cfg.size == 12 && cfg.color.g == 255 && cfg.outline && cfg.monitor == 0);
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
        "opacity = 128\r\n"
        "outline = false\r\n"
        "outline_color = \"#0000FF\"\r\n"
        "outline_thickness = 0\r\n"
        "\r\n"
        "[display]\r\n"
        "monitor = 2\r\n";
    CHECK(parse(src, &cfg, err, sizeof err));
    CHECK(cfg.color.r == 255 && cfg.color.g == 128 && cfg.color.b == 0);
    CHECK(cfg.size == 20 && cfg.thickness == 3 && cfg.gap == 0 && cfg.opacity == 128);
    CHECK(!cfg.outline);
    CHECK(cfg.outline_color.r == 0 && cfg.outline_color.g == 0 && cfg.outline_color.b == 255);
    CHECK(cfg.outline_thickness == 0);
    CHECK(cfg.monitor == 2);
}

static void test_partial_keeps_defaults(void)
{
    Config cfg;
    char err[256];
    CHECK(parse("[crosshair]\nsize = 30\n", &cfg, err, sizeof err));
    CHECK(cfg.size == 30 && cfg.thickness == 2 && cfg.opacity == 255);
    CHECK(cfg.outline && cfg.outline_thickness == 1 && cfg.monitor == 0);
}

static void test_sections_any_order(void)
{
    Config cfg;
    char err[256];
    CHECK(parse("[display]\nmonitor = 1\n[crosshair]\nsize = 9\n", &cfg, err, sizeof err));
    CHECK(cfg.monitor == 1 && cfg.size == 9);
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

static void test_invalid_values(void)
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

static void test_invalid_outline_and_display(void)
{
    expect_error("[crosshair]\noutline = yes\n", "'outline'");
    expect_error("[crosshair]\noutline = 1\n", "'outline'");
    expect_error("[crosshair]\noutline = \"true\"\n", "'outline'");
    expect_error("[crosshair]\noutline = True\n", "'outline'");
    expect_error("[crosshair]\noutline_color = \"red\"\n", "'outline_color'");
    expect_error("[crosshair]\noutline_color = \"#00FF00CC\"\n", "#RRGGBBAA");
    expect_error("[crosshair]\noutline_thickness = -1\n", "'outline_thickness'");
    expect_error("[crosshair]\noutline_thickness = 51\n", "'outline_thickness'");
    expect_error("[display]\nmonitor = -1\n", "'monitor'");
    expect_error("[display]\nmonitor = 256\n", "'monitor'");
    expect_error("[display]\nmonitor = \"primary\"\n", "'monitor'");
}

static void test_structure_errors(void)
{
    expect_error("[crosshair]\nshape = \"cross\"\n", "unknown property 'shape'");
    expect_error("[audio]\nvolume = 1\n", "unknown section");
    expect_error("size = 12\n", "inside a section");
    expect_error("[crosshair\nsize = 12\n", "malformed section");
    expect_error("[crosshair]\nsize\n", "key = value");
    expect_error("[crosshair]\nsize =\n", "missing value for 'size'");
    expect_error("[crosshair]\n= 12\n", "key = value");
    expect_error("[crosshair]\nmonitor = 1\n", "unknown property 'monitor' in [crosshair]");
    expect_error("[display]\nsize = 12\n", "unknown property 'size' in [display]");
    expect_error("[crosshair]\nsize = 12\nthickness = 1000\n", "'thickness'");
    expect_error("[crosshair]\nsize = 1\ngap = 0\nthickness = 5\n", "clipped");
}

static void test_error_reports_line(void)
{
    expect_error("[crosshair]\n\n# note\nsize = 0\n", "line 4");
    expect_error("[crosshair]\nsize = 5\n[display]\nmonitor = x\n", "line 4");
}

static void test_degenerate_input(void)
{
    Config cfg;
    char err[256];

    CHECK(parse("", &cfg, err, sizeof err));
    CHECK(cfg.size == 12 && cfg.monitor == 0);
    CHECK(parse("\n\n   \n", &cfg, err, sizeof err));
    CHECK(parse("# only a comment", &cfg, err, sizeof err));
    CHECK(parse("[crosshair]", &cfg, err, sizeof err)); /* no trailing newline */
    CHECK(parse("[crosshair]\nsize = 8", &cfg, err, sizeof err) && cfg.size == 8);
    CHECK(parse("[crosshair]\nsize = 8\nsize = 9\n", &cfg, err, sizeof err) && cfg.size == 9);

    /* A very long line must be rejected cleanly, not overflow anything. */
    char big[1024];
    memcpy(big, "[crosshair]\n", 12);
    memset(big + 12, 'a', 900);
    memcpy(big + 12 + 900, " = 1\n", 6);
    expect_error(big, "unknown property");
}

/* Writes a config file of exactly `total` bytes: a valid header padded with a comment. */
static bool write_sized_file(const char *path, size_t total)
{
    static char buf[8192];
    const char header[] = "[crosshair]\nsize = 5\n#";
    size_t hlen = sizeof header - 1;
    if (total < hlen || total > sizeof buf) return false;
    memcpy(buf, header, hlen);
    memset(buf + hlen, 'x', total - hlen);

    FILE *f = fopen(path, "wb");
    if (!f) return false;
    size_t written = fwrite(buf, 1, total, f);
    fclose(f);
    return written == total;
}

static void test_file_size_limit(void)
{
    const char *path = "test_config_size.tmp";
    Config cfg;
    char err[256];

    CHECK(write_sized_file(path, 4096));
    CHECK(config_load(path, &cfg, err, sizeof err) && cfg.size == 5);

    CHECK(write_sized_file(path, 4097));
    CHECK(!config_load(path, &cfg, err, sizeof err));
    CHECK(strstr(err, "larger than") != NULL);
    remove(path);
}

#ifdef _WIN32
/* Folder and file names outside the ANSI code page must work. */
static void test_unicode_path(void)
{
    const wchar_t *path = L"test_config_Тест_中文.tmp";
    Config cfg;
    char err[256];

    /* A missing file with such a name is not an error either. */
    CHECK(config_load_w(path, &cfg, err, sizeof err));
    CHECK(cfg.size == 12);

    FILE *f = _wfopen(path, L"wb");
    CHECK(f != NULL);
    if (!f) return;
    fputs("[crosshair]\nsize = 11\n", f);
    fclose(f);

    CHECK(config_load_w(path, &cfg, err, sizeof err));
    CHECK(cfg.size == 11);
    _wremove(path);
}
#endif

int main(void)
{
    test_defaults();
    test_missing_file_uses_defaults();
    test_valid();
    test_partial_keeps_defaults();
    test_sections_any_order();
    test_bom_file();
    test_invalid_values();
    test_invalid_outline_and_display();
    test_structure_errors();
    test_error_reports_line();
    test_degenerate_input();
    test_file_size_limit();
#ifdef _WIN32
    test_unicode_path();
#endif

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all config tests passed\n");
    return 0;
}
