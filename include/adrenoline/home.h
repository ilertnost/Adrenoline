/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Adrenoline — Home button contract and event state machine */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adrenoline.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The system consumes Home as a HID Consumer-page event, not as a keyboard
 * key and not as "AC Home". This is the same usage an accessibility switch
 * sends. See docs/HOME-BUTTON.md.
 */
#define AL_HID_USAGE_PAGE_CONSUMER 0x0Cu
#define AL_HID_USAGE_CONSUMER_MENU 0x40u
#define AL_HOME_REPORT_LEN 1u

struct al_home_report {
    uint8_t data[AL_HOME_REPORT_LEN];
};

/*
 * Called synchronously from the state machine for every state change. On a
 * real device this is where the IOKit binding hands the byte to
 * IOHIDDevice. There is no queue: if the sink blocks, the caller blocks.
 */
typedef void (*al_home_emit_fn)(void *user, const struct al_home_report *report);

struct al_home_config {
    /* A new press is ignored if the previous release is younger than this. */
    uint32_t repeat_ms;
    /* A press held longer than this without a release is force-released. */
    uint32_t stuck_ms;
    /* Bottom band that turns a tap into Home, in panel pixels. */
    uint16_t band_top;
    uint16_t band_x_min;
    uint16_t band_x_max;
    /* A touch that moves further than this is a drag, not a tap. */
    uint16_t tap_slop_px;
    /* A touch held longer than this is not a tap. */
    uint32_t tap_max_ms;
};

struct al_home;

/* 1080x1920 defaults: bottom 120 px, centre third, 24 px slop, 400 ms tap. */
void al_home_config_default(struct al_home_config *cfg, uint16_t screen_w,
                            uint16_t screen_h);

enum al_status al_home_create(const struct al_home_config *cfg,
                              al_home_emit_fn emit, void *user,
                              struct al_home **out);
void al_home_destroy(struct al_home *home);

/*
 * Source events. `now_ms` is a monotonic millisecond clock owned by the
 * caller; the state machine never reads a clock of its own so it stays
 * testable and so a suspend cannot reorder events.
 */
enum al_status al_home_key(struct al_home *home, bool down, uint64_t now_ms);
enum al_status al_home_touch(struct al_home *home, uint32_t x, uint32_t y,
                             bool down, uint64_t now_ms);

/* Drains the stuck-press watchdog. Call periodically from the source loop. */
enum al_status al_home_tick(struct al_home *home, uint64_t now_ms);

/*
 * Pause, stop or a source error. Guarantees the neutral report is emitted so
 * the system is never left believing the button is held.
 */
enum al_status al_home_reset(struct al_home *home);

bool al_home_is_pressed(const struct al_home *home);

/*
 * Report descriptor for the binding: Consumer page, Consumer Control
 * collection, one bit for Menu and seven constant bits of padding.
 */
const uint8_t *al_home_report_descriptor(size_t *len);

#ifdef __cplusplus
}
#endif
