// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Globals.hpp"
#include "Debug.hpp"
#include "ui/dim/Point.hpp"
#include "LogFile.hpp"
#include "util/StaticString.hxx"

#include <glm/mat4x4.hpp>

namespace OpenGL {

bool texture_non_power_of_two;

unsigned max_texture_size;

bool mapbuffer;

GLenum render_buffer_depth_stencil, render_buffer_stencil;

unsigned max_antialiasing_samples;

unsigned available_antialiasing_samples;

unsigned antialiasing_samples;

UnsignedPoint2D window_size, viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
DisplayOrientation display_orientation;
#endif

PixelPoint translate;

glm::mat4 projection_matrix;

unsigned max_map_scale;

#ifndef NDEBUG
#ifdef _WIN32
DWORD thread;
#else
pthread_t thread;
#endif
#endif

void
SetAntialiasingSamples(unsigned samples) noexcept
{
  antialiasing_samples = samples;
  LogFmt("Anti-aliasing: {} samples", samples);
}

void
SetAvailableAntialiasingSamples(unsigned mask) noexcept
{
  available_antialiasing_samples = mask;

  StaticString<64> buffer;
  buffer.clear();
  for (const unsigned n : ANTIALIASING_SAMPLE_COUNTS)
    if (mask & (1u << n))
      buffer.AppendFormat(" %ux", n);

  LogFmt("Anti-aliasing available:{}",
         buffer.empty() ? " none" : buffer.c_str());
}

} // namespace OpenGL
