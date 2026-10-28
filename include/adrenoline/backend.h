/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "adrenoline.h"
#include "nir.h"

#ifdef __cplusplus
extern "C" {
#endif

struct al_device;
struct al_pipeline;
struct al_cmdbuf;

enum al_backend_kind {
    AL_BACKEND_STUB = 0,
    AL_BACKEND_GALLIUM = 1,
    AL_BACKEND_PM4_A5XX = 2,
};

/*
 * What a backend can actually do, as opposed to what the Metal frontend would
 * like to promise. The frontend degrades against this table; it must never
 * advertise a capability that no selected backend reports.
 */
struct al_caps {
    uint32_t max_texture_2d;
    uint32_t max_render_targets;
    uint32_t max_samples;
    bool integer_textures;
    bool depth32_float;
    bool compute;
    bool astc;
    bool owns_tiling; /* backend runs its own GMEM/binning pass */
};

/*
 * Hardware facts a backend needs. Values for MSM8998 / Adreno 540 are the
 * documented defaults; a port supplies its own from the device tree.
 */
struct al_device_desc {
    uint64_t register_base;
    uint64_t smmu_base;
    uint32_t smmu_page_size;
    uint32_t address_bits;
    void *platform; /* opaque, backend specific */
};

struct al_backend_ops {
    const char *name;
    enum al_backend_kind kind;

    enum al_status (*create)(const struct al_device_desc *desc, struct al_device **out);
    void (*destroy)(struct al_device *dev);
    const struct al_caps *(*caps)(const struct al_device *dev);

    enum al_status (*pipeline_create)(struct al_device *dev,
                                      const struct al_nir_module *module,
                                      struct al_pipeline **out);
    void (*pipeline_destroy)(struct al_device *dev, struct al_pipeline *pipeline);

    enum al_status (*cmdbuf_begin)(struct al_device *dev, struct al_cmdbuf **out);
    enum al_status (*cmdbuf_draw)(struct al_cmdbuf *cb, struct al_pipeline *pipeline,
                                  uint32_t vertex_count, uint32_t instance_count);
    enum al_status (*cmdbuf_end)(struct al_cmdbuf *cb, uint64_t *fence_out);

    enum al_status (*fence_wait)(struct al_device *dev, uint64_t fence, uint64_t timeout_ns);
};

/* Registry. Backends self-register; the seam stays link-time explicit. */
enum al_status al_backend_register(const struct al_backend_ops *ops);
const struct al_backend_ops *al_backend_by_kind(enum al_backend_kind kind);
const struct al_backend_ops *al_backend_by_name(const char *name);
uint32_t al_backend_count(void);
const struct al_backend_ops *al_backend_at(uint32_t index);

/* Provided by backends/stub. */
const struct al_backend_ops *al_backend_stub(void);

#ifdef __cplusplus
}
#endif
