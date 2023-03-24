// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * A collection of global variables for the OpenGL backend.  Global
 * variables are not good style, but since there can be only once
 * OpenGL context at a time, this is good enough for XCSoar.
 *
 */

#pragma once

#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"

#include <glm/fwd.hpp>

#ifdef SOFTWARE_ROTATE_DISPLAY
#include <cstdint>
enum class DisplayOrientation : uint8_t;
#endif

struct UnsignedPoint2D;
struct PixelPoint;

namespace OpenGL {

/**
 * Is the extension ARB_texture_non_power_of_two present?  If yes,
 * then textures can have any size, not just power of two.
 */
extern bool texture_non_power_of_two;

/**
 * Is glMapBuffer() available?  May be implemented by the extension
 * GL_OES_mapbuffer.
 */
extern bool mapbuffer;

/**
 * Which depth+stencil internalFormat is supported by the
 * Renderbuffer?
 */
extern GLenum render_buffer_depth_stencil;

/**
 * Which stencil internalFormat is supported by the Renderbuffer?
 */
extern GLenum render_buffer_stencil;

/**
 * The dimensions of the OpenGL window in pixels.
 */
extern UnsignedPoint2D window_size;

/**
 * The dimensions of the OpenGL viewport in pixels.
 */
extern UnsignedPoint2D viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
extern DisplayOrientation display_orientation;
#endif

/**
 * The current SubCanvas translation in pixels.
 */
extern PixelPoint translate;

extern glm::mat4 projection_matrix;

} // namespace OpenGL
