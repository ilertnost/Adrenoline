/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>

#include "adrenoline/backend.h"

int main(void)
{
    int failures = 0;

    if (al_backend_register(al_backend_stub()) != AL_OK) {
        fprintf(stderr, "register: unexpected failure\n");
        return 1;
    }

    if (al_backend_count() != 1) {
        fprintf(stderr, "count: expected 1, got %u\n", al_backend_count());
        failures++;
    }

    const struct al_backend_ops *ops = al_backend_by_kind(AL_BACKEND_STUB);
    if (ops == NULL || ops->kind != AL_BACKEND_STUB) {
        fprintf(stderr, "lookup by kind failed\n");
        return 1;
    }

    if (al_backend_by_name("stub") != ops) {
        fprintf(stderr, "lookup by name failed\n");
        failures++;
    }

    if (al_backend_by_name("turnip") != NULL) {
        fprintf(stderr, "lookup of absent backend should be NULL\n");
        failures++;
    }

    struct al_device_desc desc = {
        .register_base = 0x05000000ull,
        .smmu_base = 0x05040000ull,
        .smmu_page_size = 4096,
        .address_bits = 49,
        .platform = NULL,
    };

    struct al_device *dev = NULL;
    if (ops->create(&desc, &dev) != AL_OK || dev == NULL) {
        fprintf(stderr, "create failed\n");
        return 1;
    }

    const struct al_caps *caps = ops->caps(dev);
    if (caps == NULL || caps->owns_tiling) {
        fprintf(stderr, "unexpected caps for stub\n");
        failures++;
    }

    struct al_nir_module unused_module = {0};
    struct al_pipeline *pipe = NULL;
    if (ops->pipeline_create(dev, &unused_module, &pipe) != AL_ERR_UNSUPPORTED) {
        fprintf(stderr, "stub pipeline_create must not fabricate success\n");
        failures++;
    }

    ops->destroy(dev);

    if (failures == 0)
        printf("ok\n");
    return failures == 0 ? 0 : 1;
}
