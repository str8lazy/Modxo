/*
 * Host-side unit tests for the OOB control protocol parser.
 * Build: cc -DOOB_HOST_TEST -I modxo_modules/oob_debug/include \
 *          host/oob_protocol_test.c modxo_modules/oob_debug/oob_protocol.c -o oob_protocol_test
 */
#include <oob_debug.h>

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void expect_true(const char *name, bool cond)
{
    if (!cond) {
        printf("FAIL: %s\n", name);
        failures++;
    } else {
        printf("PASS: %s\n", name);
    }
}

static void expect_contains(const char *name, const char *hay, const char *needle)
{
    if (!hay || !strstr(hay, needle)) {
        printf("FAIL: %s (missing '%s' in '%s')\n", name, needle, hay ? hay : "(null)");
        failures++;
    } else {
        printf("PASS: %s\n", name);
    }
}

int main(void)
{
    oob_cmd_result_t r;

    expect_true("help recognized", oob_parse_and_dispatch("help", &r, false));
    expect_true("help ok", r.ok);

    expect_true("status recognized", oob_parse_and_dispatch("status", &r, false));
    expect_true("reset without confirm fails", oob_parse_and_dispatch("reset", &r, false));
    expect_true("reset without confirm not ok", !r.ok);
    expect_contains("reset err text", r.reply, "CONFIRM");

    expect_true("reset with CONFIRM", oob_parse_and_dispatch("reset CONFIRM", &r, false));
    expect_true("reset confirm ok", r.ok);

    expect_true("cycle with --confirm", oob_parse_and_dispatch("cycle --confirm", &r, false));
    expect_true("cycle confirm ok", r.ok);

    expect_true("off without token fails", oob_parse_and_dispatch("off", &r, false));
    expect_true("off fails", !r.ok);

    expect_true("smbus read parse", oob_parse_and_dispatch("smbus read 0x10 0x01", &r, false));
    expect_true("smbus read ok", r.ok);

    expect_true("smbus write parse", oob_parse_and_dispatch("smbus write 0x10 0x02 0x01", &r, false));
    expect_true("smbus write ok", r.ok);

    expect_true("unknown cmd", oob_parse_and_dispatch("reboot", &r, false));
    expect_true("unknown not ok", !r.ok);

    expect_true("hb-stale parse", oob_parse_and_dispatch("hb-stale 3000", &r, false));
    expect_true("hb-stale ok", r.ok);

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
