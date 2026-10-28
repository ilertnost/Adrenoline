/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>

#include "adrenoline/backend.h"

#define AL_MAX_BACKENDS 8

static const struct al_backend_ops *registry[AL_MAX_BACKENDS];
static uint32_t registry_count;

enum al_status al_backend_register(const struct al_backend_ops *ops)
{
    if (ops == NULL || ops->name == NULL)
        return AL_ERR_INVALID;

    for (uint32_t i = 0; i < registry_count; i++) {
        if (strcmp(registry[i]->name, ops->name) == 0)
            return AL_ERR_INVALID;
    }
    if (registry_count == AL_MAX_BACKENDS)
        return AL_ERR_NOMEM;

    registry[registry_count++] = ops;
    return AL_OK;
}

const struct al_backend_ops *al_backend_by_kind(enum al_backend_kind kind)
{
    for (uint32_t i = 0; i < registry_count; i++) {
        if (registry[i]->kind == kind)
            return registry[i];
    }
    return NULL;
}

const struct al_backend_ops *al_backend_by_name(const char *name)
{
    if (name == NULL)
        return NULL;

    for (uint32_t i = 0; i < registry_count; i++) {
        if (strcmp(registry[i]->name, name) == 0)
            return registry[i];
    }
    return NULL;
}

uint32_t al_backend_count(void)
{
    return registry_count;
}

const struct al_backend_ops *al_backend_at(uint32_t index)
{
    return index < registry_count ? registry[index] : NULL;
}
