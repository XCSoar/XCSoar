// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Globals.hpp"
#include "Debug.hpp"
#include "ui/dim/Point.hpp"

#include <glm/mat4x4.hpp>

namespace OpenGL {

bool texture_non_power_of_two;

bool mapbuffer;

GLenum render_buffer_depth_stencil, render_buffer_stencil;

UnsignedPoint2D window_size, viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
DisplayOrientation display_orientation;
#endif

PixelPoint translate;

glm::mat4 projection_matrix;

#ifndef NDEBUG
pthread_t thread;
#endif

} // namespace OpenGL
