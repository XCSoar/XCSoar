// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PortableColor.hpp"

/**
 * Perceived luminosity helpers shared by all canvas backends.
 * Weights match Luminosity8::FromRGB() in PortableColor.hpp and the
 * luminosity() GLSL function in Shaders.cpp.
 */
namespace Luminosity {

constexpr float RED = 2126.f / 10000.f;
constexpr float GREEN = 7152.f / 10000.f;
constexpr float BLUE = 722.f / 10000.f;

[[gnu::const]] constexpr uint8_t
FromRGB(uint8_t r, uint8_t g, uint8_t b) noexcept
{
  return Luminosity8(r, g, b).GetLuminosity();
}

[[gnu::const]] constexpr float
FromNormalized(float r, float g, float b) noexcept
{
  return r * RED + g * GREEN + b * BLUE;
}

/**
 * GLSL fragment shader preamble.  Keep weights in sync with FromRGB().
 */
#define LUMINOSITY_GLSL_PREAMBLE \
  "const vec3 LUMINOSITY_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);\n" \
  "float luminosity(vec3 rgb) {\n" \
  "  return dot(rgb, LUMINOSITY_WEIGHTS);\n" \
  "}\n"

} // namespace Luminosity
