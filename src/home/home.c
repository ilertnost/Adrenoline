/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include <string.h>

#include "adrenoline/home.h"

struct al_home {
    struct al_home_config cfg;
    al_home_emit_fn emit;
    void *user;

    bool pressed;
    uint64_t pressed_at;

    bool have_released;
    uint64_t released_at;

    bool touch_down;
    uint32_t touch_x;
    uint32_t touch_y;
    uint64_t touch_at;
};

static const uint8_t report_descriptor[] = {
    0x05, 0x0C, /* Usage Page (Consumer) */
    0x09, 0x01, /* Usage (Consumer Control) */
    0xA1, 0x01, /* Collection (Application) */
    0x09, 0x40, /*   Usage (Menu) */
    0x15, 0x00, /*   Logical Minimum (0) */
    0x25, 0x01, /*   Logical Maximum (1) */
    0x75, 0x01, /*   Report Size (1) */
    0x95, 0x01, /*   Report Count (1) */
    0x81, 0x02, /*   Input (Data, Variable, Absolute) */
    0x75, 0x07, /*   Report Size (7) */
    0x95, 0x01, /*   Report Count (1) */
    0x81, 0x03, /*   Input (Constant, Variable, Absolute) */
    0xC0,       /* End Collection */
};

static void emit_state(struct al_home *home)
{
    struct al_home_report report;

    if (home->emit == NULL) {
        return;
    }

    memset(&report, 0, sizeof(report));
    report.data[0] = home->pressed ? 0x01u : 0x00u;
    home->emit(home->user, &report);
}

static void press(struct al_home *home, uint64_t now_ms)
{
    if (home->pressed) {
        return; /* duplicate notification, not a second click */
    }

    if (home->have_released && now_ms - home->released_at < home->cfg.repeat_ms) {
        return; /* contact bounce */
    }

    home->pressed = true;
    home->pressed_at = now_ms;
    emit_state(home);
}

static void release(struct al_home *home, uint64_t now_ms)
{
    if (!home->pressed) {
        return;
    }

    home->pressed = false;
    home->have_released = true;
    home->released_at = now_ms;
    emit_state(home);
}

void al_home_config_default(struct al_home_config *cfg, uint16_t screen_w,
                            uint16_t screen_h)
{
    if (cfg == NULL) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->repeat_ms = 150;
    cfg->stuck_ms = 10000;
    cfg->band_top = (uint16_t)(screen_h > 120 ? screen_h - 120 : 0);
    cfg->band_x_min = (uint16_t)(screen_w / 3);
    cfg->band_x_max = (uint16_t)((screen_w / 3) * 2);
    cfg->tap_slop_px = 24;
    cfg->tap_max_ms = 400;
}

enum al_status al_home_create(const struct al_home_config *cfg,
                              al_home_emit_fn emit, void *user,
                              struct al_home **out)
{
    struct al_home *home;

    if (cfg == NULL || out == NULL) {
        return AL_ERR_INVALID;
    }

    if (cfg->band_x_max < cfg->band_x_min) {
        return AL_ERR_INVALID;
    }

    home = calloc(1, sizeof(*home));
    if (home == NULL) {
        return AL_ERR_NOMEM;
    }

    home->cfg = *cfg;
    home->emit = emit;
    home->user = user;
    *out = home;
    return AL_OK;
}

void al_home_destroy(struct al_home *home)
{
    free(home);
}

enum al_status al_home_key(struct al_home *home, bool down, uint64_t now_ms)
{
    if (home == NULL) {
        return AL_ERR_INVALID;
    }

    if (down) {
        press(home, now_ms);
    } else {
        release(home, now_ms);
    }

    return AL_OK;
}

enum al_status al_home_touch(struct al_home *home, uint32_t x, uint32_t y,
                             bool down, uint64_t now_ms)
{
    bool in_band;

    if (home == NULL) {
        return AL_ERR_INVALID;
    }

    in_band = y >= home->cfg.band_top && x >= home->cfg.band_x_min &&
              x <= home->cfg.band_x_max;

    if (down) {
        home->touch_down = in_band;
        home->touch_x = x;
        home->touch_y = y;
        home->touch_at = now_ms;
        return AL_OK;
    }

    /* A release outside the band, after too long, or after a drag is not a tap. */
    if (home->touch_down && in_band &&
        now_ms - home->touch_at <= home->cfg.tap_max_ms &&
        x >= home->touch_x && x - home->touch_x <= home->cfg.tap_slop_px &&
        y >= home->touch_y && y - home->touch_y <= home->cfg.tap_slop_px) {
        press(home, now_ms);
        release(home, now_ms);
    }

    home->touch_down = false;
    return AL_OK;
}

enum al_status al_home_tick(struct al_home *home, uint64_t now_ms)
{
    if (home == NULL) {
        return AL_ERR_INVALID;
    }

    if (home->pressed && now_ms - home->pressed_at > home->cfg.stuck_ms) {
        release(home, now_ms);
    }

    return AL_OK;
}

enum al_status al_home_reset(struct al_home *home)
{
    if (home == NULL) {
        return AL_ERR_INVALID;
    }

    home->touch_down = false;
    home->pressed = false;
    emit_state(home);
    return AL_OK;
}

bool al_home_is_pressed(const struct al_home *home)
{
    return home != NULL && home->pressed;
}

const uint8_t *al_home_report_descriptor(size_t *len)
{
    if (len != NULL) {
        *len = sizeof(report_descriptor);
    }
    return report_descriptor;
}
