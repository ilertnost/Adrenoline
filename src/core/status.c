/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "adrenoline/adrenoline.h"

const char *al_status_string(enum al_status status)
{
    switch (status) {
    case AL_OK:
        return "ok";
    case AL_ERR_INVALID:
        return "invalid argument";
    case AL_ERR_NOMEM:
        return "out of memory";
    case AL_ERR_UNSUPPORTED:
        return "unsupported";
    case AL_ERR_IO:
        return "i/o error";
    case AL_ERR_TIMEOUT:
        return "timeout";
    case AL_ERR_DEVICE_LOST:
        return "device lost";
    case AL_ERR_STALE:
        return "stale object";
    }
    return "unknown";
}
