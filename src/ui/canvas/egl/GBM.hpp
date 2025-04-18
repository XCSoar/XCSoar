// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <gbm.h>

static constexpr uint32_t XCSOAR_GBM_FORMAT = GBM_FORMAT_XRGB8888;

/**
 * A fallback value for EGL_NATIVE_VISUAL_ID; this is needed for the
 * "amdgpu" driver on Linux.
 */
static constexpr uint32_t XCSOAR_GBM_FORMAT_FALLBACK = GBM_FORMAT_ARGB8888;
