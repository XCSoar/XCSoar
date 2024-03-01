// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Icon.hpp"
#include "Canvas.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "opengl/Texture.hpp"
#include "opengl/Scope.hpp"

#include "opengl/Shaders.hpp"
#include "opengl/Program.hpp"
#endif

#include <algorithm>

[[gnu::const]]
static unsigned
IconStretchFixed10(unsigned source_dpi) noexcept
{
  /* the icons were designed for PDAs at short eye distance; the 3/2
     factor reverses the 2/3 factor applied by Layout::Initialise()
     for small screens */
  return Layout::VptScale(72 * 1024 * 3 / 2) / source_dpi;
}

#ifndef ENABLE_OPENGL

[[gnu::const]]
static unsigned
IconStretchInteger(unsigned source_dpi) noexcept
{
  return std::max((IconStretchFixed10(source_dpi) + 512) >> 10,
                  1u);
}

#endif

void
MaskedIcon::LoadResource(ResourceId id, ResourceId big_id,
                         ResourceId ultra_id,
                         bool center)
{
#ifdef ENABLE_OPENGL
  unsigned stretch = 1024;
#endif

  if (Layout::vdpi >= 120) {
    /* switch to larger 160dpi icons at 120dpi */

    unsigned source_dpi = 96;
    if (Layout::vdpi >= 220 && ultra_id.IsDefined()) {
      id = ultra_id;
      source_dpi = 300;
    } else if (big_id.IsDefined()) {
      id = big_id;
      source_dpi = 192;
    }

#ifdef ENABLE_OPENGL
    stretch = IconStretchFixed10(source_dpi);
    bitmap.Load(id);
#else
    bitmap.LoadStretch(id, IconStretchInteger(source_dpi));
#endif
  } else
    bitmap.Load(id);

  assert(IsDefined());

  size = bitmap.GetSize();
#ifdef ENABLE_OPENGL
  /* let the GPU stretch on-the-fly */
  size.width = size.width * stretch >> 10;
  size.height = size.height * stretch >> 10;
#else
  /* left half is mask, right half is icon */
  size.width /= 2;
#endif

  if (center) {
    origin.x = size.width / 2;
    origin.y = size.height / 2;
  } else {
    origin.x = 0;
    origin.y = 0;
  }
}

void
MaskedIcon::Draw([[maybe_unused]] Canvas &canvas, PixelPoint p) const noexcept
{
  assert(IsDefined());

  p -= origin;

#ifdef ENABLE_OPENGL
  OpenGL::texture_shader->Use();

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(p, size), texture.GetRect());
#else

#ifdef USE_GDI
  /* our icons are monochrome bitmaps, and GDI uses current colors of
     the destination HDC when blitting from a monochrome HDC */
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);
#endif

  canvas.CopyOr(p, size, bitmap, {0, 0});
  canvas.CopyAnd(p, size, bitmap, {(int)size.width, 0});
#endif
}

void
MaskedIcon::Draw([[maybe_unused]] Canvas &canvas, const PixelRect &rc, bool inverse) const noexcept
{
  const PixelPoint position = rc.CenteredTopLeft(size);

#ifdef ENABLE_OPENGL
  if (inverse)
    OpenGL::invert_shader->Use();
  else
    OpenGL::texture_shader->Use();

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(position, size), texture.GetRect());
#else
  if (inverse) // black background
    canvas.CopyNotOr(position, size, bitmap, {(int)size.width, 0});

  else
    canvas.CopyAnd(position, size, bitmap, {(int)size.width, 0});
#endif

}
