/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A compiled shader module handed from the frontend to a backend.
 *
 * `data` is a serialized NIR payload. The encoding is private to the seam and
 * is versioned by `format_version`; the frontend and the backend are always
 * built from the same tree, so no cross-version stability is promised here.
 *
 * `key` is a pipeline cache key derived from the stage set, the entry points,
 * the function-constant values and the target device profile. Two modules with
 * the same key must produce identical machine code.
 */
struct al_nir_module {
    const void *data;
    size_t size;
    uint32_t format_version;
    uint32_t stage_mask; /* bit 0 vertex, bit 1 fragment, bit 2 compute */
    uint64_t key;
};

enum al_stage_bits {
    AL_STAGE_VERTEX = 1u << 0,
    AL_STAGE_FRAGMENT = 1u << 1,
    AL_STAGE_COMPUTE = 1u << 2,
};

#ifdef __cplusplus
}
#endif
