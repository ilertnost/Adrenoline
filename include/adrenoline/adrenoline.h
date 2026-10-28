/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Adrenoline — Metal shader frontend and GPU runtime for Adreno a5xx */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define AL_VERSION_MAJOR 0
#define AL_VERSION_MINOR 1
#define AL_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

enum al_status {
    AL_OK = 0,
    AL_ERR_INVALID = -1,
    AL_ERR_NOMEM = -2,
    AL_ERR_UNSUPPORTED = -3,
    AL_ERR_IO = -4,
    AL_ERR_TIMEOUT = -5,
    AL_ERR_DEVICE_LOST = -6,
    AL_ERR_STALE = -7,
};

const char *al_status_string(enum al_status status);

#ifdef __cplusplus
}
#endif
