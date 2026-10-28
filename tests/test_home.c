/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <string.h>

#include "adrenoline/home.h"

struct sink {
    uint8_t data[16];
    size_t n;
};

static void collect(void *user, const struct al_home_report *report)
{
    struct sink *sink = user;

    if (sink->n < sizeof(sink->data)) {
        sink->data[sink->n++] = report->data[0];
    }
}

static void sink_reset(struct sink *sink)
{
    sink->n = 0;
    memset(sink->data, 0, sizeof(sink->data));
}

static int failures;

static void check(int cond, const char *what)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

static int expect(const struct sink *sink, const uint8_t *want, size_t n,
                  const char *what)
{
    if (sink->n != n || (n != 0 && memcmp(sink->data, want, n) != 0)) {
        fprintf(stderr, "FAIL: %s (got %zu reports:", what, sink->n);
        for (size_t i = 0; i < sink->n; i++) {
            fprintf(stderr, " %u", sink->data[i]);
        }
        fprintf(stderr, ")\n");
        failures++;
        return 0;
    }
    return 1;
}

int main(void)
{
    struct al_home_config cfg;
    struct al_home *home = NULL;
    struct sink sink;

    memset(&sink, 0, sizeof(sink));
    al_home_config_default(&cfg, 1080, 1920);

    if (al_home_create(&cfg, collect, &sink, &home) != AL_OK || home == NULL) {
        fprintf(stderr, "create failed\n");
        return 1;
    }

    /* A press produces one press, duplicates are not clicks. */
    sink_reset(&sink);
    al_home_key(home, true, 1000);
    al_home_key(home, true, 1010);
    {
        const uint8_t want[] = { 1 };
        expect(&sink, want, 1, "single press from duplicate downs");
    }

    al_home_key(home, false, 1100);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "release after press");
    }
    check(!al_home_is_pressed(home), "not pressed after release");

    /* Contact bounce inside the repeat window is dropped. */
    sink_reset(&sink);
    al_home_key(home, true, 1200); /* 100 ms after release, under 150 */
    al_home_key(home, false, 1210);
    expect(&sink, NULL, 0, "bounce suppressed");

    /* Once the window passes it is a real click again. */
    al_home_key(home, true, 1400);
    al_home_key(home, false, 1450);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "click after repeat window");
    }

    /* A source that dies mid-press must not leave the button held. */
    sink_reset(&sink);
    al_home_key(home, true, 2000);
    al_home_tick(home, 2000 + cfg.stuck_ms + 1);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "stuck press force-released");
    }

    /* Reset always leaves a neutral state. */
    sink_reset(&sink);
    al_home_key(home, true, 3000);
    al_home_reset(home);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "reset emits neutral");
    }

    /* Tap in the Home band is a click. */
    sink_reset(&sink);
    al_home_touch(home, 540, 1850, true, 4000);
    al_home_touch(home, 540, 1852, false, 4050);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "tap in band");
    }

    /* Outside the band nothing happens. */
    sink_reset(&sink);
    al_home_touch(home, 540, 900, true, 5000);
    al_home_touch(home, 540, 900, false, 5050);
    expect(&sink, NULL, 0, "tap outside band ignored");

    /* A drag is not a tap. */
    sink_reset(&sink);
    al_home_touch(home, 540, 1850, true, 6000);
    al_home_touch(home, 700, 1850, false, 6050);
    expect(&sink, NULL, 0, "drag ignored");

    /* A hold past the tap limit is not a tap. */
    sink_reset(&sink);
    al_home_touch(home, 540, 1850, true, 7000);
    al_home_touch(home, 540, 1850, false, 7000 + cfg.tap_max_ms + 1);
    expect(&sink, NULL, 0, "slow touch ignored");

    /* Touch taps obey the same repeat window as the key source. */
    sink_reset(&sink);
    al_home_touch(home, 540, 1850, true, 8100);
    al_home_touch(home, 540, 1850, false, 8150);
    {
        const uint8_t want[] = { 1, 0 };
        expect(&sink, want, 2, "tap after idle");
    }

    sink_reset(&sink);
    al_home_touch(home, 540, 1850, true, 8200); /* 50 ms after release */
    al_home_touch(home, 540, 1850, false, 8250);
    expect(&sink, NULL, 0, "tap inside repeat window suppressed");

    /* Descriptor must actually declare Consumer/Menu. */
    {
        size_t len = 0;
        const uint8_t *desc = al_home_report_descriptor(&len);

        check(desc != NULL && len >= 6, "descriptor present");
        check(desc[0] == 0x05 && desc[1] == 0x0C, "usage page consumer");
        check(memchr(desc, 0x09, len) != NULL, "usage item present");
        check(desc[6] == 0x09 && desc[7] == 0x40, "usage Menu");
    }

    /* Bad configuration is rejected rather than clamped. */
    {
        struct al_home_config bad = cfg;
        struct al_home *junk = NULL;

        bad.band_x_max = 0;
        bad.band_x_min = 100;
        check(al_home_create(&bad, NULL, NULL, &junk) == AL_ERR_INVALID,
              "inverted band rejected");
        check(junk == NULL, "nothing allocated on rejection");
        check(al_home_create(NULL, NULL, NULL, &junk) == AL_ERR_INVALID,
              "null config rejected");
    }

    al_home_destroy(home);

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }

    printf("home: ok\n");
    return 0;
}
