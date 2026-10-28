/* SPDX-License-Identifier: GPL-3.0-or-later */

/*
 * Stub backend. It exists so the seam can be built and tested on a host
 * without any hardware. Every entry point that would touch a device returns
 * AL_ERR_UNSUPPORTED; nothing here fabricates success.
 */

#include <stdlib.h>

#include "adrenoline/backend.h"

struct al_device {
    struct al_caps caps;
};

static const struct al_caps stub_caps = {
    .max_texture_2d = 0,
    .max_render_targets = 0,
    .max_samples = 0,
    .integer_textures = false,
    .depth32_float = false,
    .compute = false,
    .astc = false,
    .owns_tiling = false,
};

static enum al_status stub_create(const struct al_device_desc *desc, struct al_device **out)
{
    if (desc == NULL || out == NULL)
        return AL_ERR_INVALID;

    struct al_device *dev = calloc(1, sizeof(*dev));
    if (dev == NULL)
        return AL_ERR_NOMEM;

    dev->caps = stub_caps;
    *out = dev;
    return AL_OK;
}

static void stub_destroy(struct al_device *dev)
{
    free(dev);
}

static const struct al_caps *stub_caps_get(const struct al_device *dev)
{
    return dev != NULL ? &dev->caps : NULL;
}

static enum al_status stub_pipeline_create(struct al_device *dev,
                                           const struct al_nir_module *module,
                                           struct al_pipeline **out)
{
    (void)dev;
    (void)module;
    (void)out;
    return AL_ERR_UNSUPPORTED;
}

static void stub_pipeline_destroy(struct al_device *dev, struct al_pipeline *pipeline)
{
    (void)dev;
    (void)pipeline;
}

static enum al_status stub_cmdbuf_begin(struct al_device *dev, struct al_cmdbuf **out)
{
    (void)dev;
    (void)out;
    return AL_ERR_UNSUPPORTED;
}

static enum al_status stub_cmdbuf_draw(struct al_cmdbuf *cb, struct al_pipeline *pipeline,
                                       uint32_t vertex_count, uint32_t instance_count)
{
    (void)cb;
    (void)pipeline;
    (void)vertex_count;
    (void)instance_count;
    return AL_ERR_UNSUPPORTED;
}

static enum al_status stub_cmdbuf_end(struct al_cmdbuf *cb, uint64_t *fence_out)
{
    (void)cb;
    (void)fence_out;
    return AL_ERR_UNSUPPORTED;
}

static enum al_status stub_fence_wait(struct al_device *dev, uint64_t fence,
                                      uint64_t timeout_ns)
{
    (void)dev;
    (void)fence;
    (void)timeout_ns;
    return AL_ERR_UNSUPPORTED;
}

static const struct al_backend_ops stub_ops = {
    .name = "stub",
    .kind = AL_BACKEND_STUB,
    .create = stub_create,
    .destroy = stub_destroy,
    .caps = stub_caps_get,
    .pipeline_create = stub_pipeline_create,
    .pipeline_destroy = stub_pipeline_destroy,
    .cmdbuf_begin = stub_cmdbuf_begin,
    .cmdbuf_draw = stub_cmdbuf_draw,
    .cmdbuf_end = stub_cmdbuf_end,
    .fence_wait = stub_fence_wait,
};

const struct al_backend_ops *al_backend_stub(void)
{
    return &stub_ops;
}
