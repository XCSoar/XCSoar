// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/egl/System.hpp"

namespace EGL {

EGLConfig
ChooseConfig(EGLDisplay display, unsigned antialiasing_samples = 0);

/**
 * Determine which MSAA sample counts this display can provide, by
 * asking for each of them in turn. See
 * OpenGL::available_antialiasing_samples for the bit mask layout.
 */
unsigned
ProbeAntialiasingSamples(EGLDisplay display) noexcept;

} // namespace EGL
