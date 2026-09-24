// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/System.hpp"

/**
 * GLSL attributes.
 */
namespace OpenGL::Attribute {

static constexpr GLuint POSITION = 1;
static constexpr GLuint TEXCOORD = 2;
static constexpr GLuint COLOR = 3;

/** the radius of a round line, see #round_line_shader */
static constexpr GLuint RADIUS = 4;

} // namespace OpenGL::Attribute
