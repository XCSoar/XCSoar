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

unsigned available_antialiasing_samples;

unsigned requested_antialiasing_samples;

unsigned antialiasing_samples;

FboAntialiasingMode fbo_antialiasing_mode;

unsigned fbo_antialiasing_samples;

unsigned available_fbo_antialiasing_samples;

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

/**
 * Format the sample counts set in #mask as e.g. " 2x 4x 8x", for use
 * right after a fixed label.  " unknown" if the mask has never been
 * probed (see #available_antialiasing_samples), " none" if it has
 * but found nothing.
 */
[[gnu::pure]]
static StaticString<32>
FormatSampleList(unsigned mask) noexcept
{
  StaticString<32> buffer;

  if (mask == 0) {
    buffer = " unknown";
    return buffer;
  }

  buffer.clear();
  for (const unsigned n : ANTIALIASING_SAMPLE_COUNTS)
    if (mask & (1u << n))
      buffer.AppendFormat(" %ux", n);

  if (buffer.empty())
    buffer = " none";

  return buffer;
}

/**
 * Format a channel's chosen sample count as e.g. "4x", or "off" for
 * zero.
 */
[[gnu::pure]]
static StaticString<16>
FormatChosenSamples(unsigned samples) noexcept
{
  StaticString<16> buffer;
  if (samples > 0)
    buffer.Format("%ux", samples);
  else
    buffer = "off";

  return buffer;
}

void
SetAntialiasingSamples(unsigned samples) noexcept
{
  antialiasing_samples = samples;

  LogFmt("Anti-aliasing (window): available{}, active {}",
         FormatSampleList(available_antialiasing_samples).c_str(),
         FormatChosenSamples(samples).c_str());
}

void
SetAvailableAntialiasingSamples(unsigned mask) noexcept
{
  available_antialiasing_samples = mask;
}

void
SetFboAntialiasing(FboAntialiasingMode mode, unsigned samples) noexcept
{
  fbo_antialiasing_mode = mode;
  fbo_antialiasing_samples = samples;

  /* unlike the window surface, any level up to the hardware limit
     works, so bit 0 ("known") is always set, and every level at or
     below the limit joins it */
  available_fbo_antialiasing_samples = 1u;
  for (const unsigned n : ANTIALIASING_SAMPLE_COUNTS)
    if (n <= samples)
      available_fbo_antialiasing_samples |= 1u << n;

  switch (mode) {
  case FboAntialiasingMode::NONE:
    LogFmt("Anti-aliasing (FBO): none");
    break;

  case FboAntialiasingMode::IMPLICIT:
    LogFmt("Anti-aliasing (FBO): up to {}x, implicit resolve", samples);
    break;

  case FboAntialiasingMode::BLIT:
    LogFmt("Anti-aliasing (FBO): up to {}x, blit resolve", samples);
    break;
  }
}

} // namespace OpenGL
