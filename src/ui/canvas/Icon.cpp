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
#elif !defined(USE_GDI)
#include "VirtualCanvas.hpp"
#endif

#include <algorithm>

/**
 * Heuristic: if the caller's text colour is light, the background
 * is probably dark.  Threshold: average channel > 128.
 */
[[gnu::const]]
static bool
IsDarkBackground(Color text_color) noexcept
{
#ifdef GREYSCALE
  return text_color.GetLuminosity() > 128;
#else
  return (text_color.Red() + text_color.Green() +
          text_color.Blue()) > 384;
#endif
}

[[gnu::const]]
static unsigned
IconStretchFixed10(unsigned source_dpi) noexcept
{
  /* the icons were designed for PDAs at short eye distance; the 3/2
     factor reverses the 2/3 factor applied by Layout::Initialise()
     for small screens */
  return Layout::VptScale(72 * 1024 * 3 / 2) / source_dpi;
}

#ifdef ENABLE_OPENGL
/**
 * The icons are rendered at this multiple of the resolution their
 * density bucket needs, so the GPU has enough texels where they are
 * magnified (list rows).  Keep in sync with build/resource.mk.
 */
static constexpr unsigned ICON_SUPERSAMPLE = 3;
#endif

/* nominal densities of the icon variants (Android density buckets);
   ldpi is the 96 dpi desktop baseline rather than Android's 120 */
static constexpr unsigned ICON_LDPI = 96;
static constexpr unsigned ICON_MDPI = 160;
static constexpr unsigned ICON_XHDPI = 320;
static constexpr unsigned ICON_XXHDPI = 480;

/**
 * The physical display density: Layout::vdpi with the small-screen
 * viewing distance adjustment undone.
 */
[[gnu::pure]]
static unsigned
DisplayDensity() noexcept
{
  return Layout::small_screen ? Layout::vdpi * 3 / 2 : Layout::vdpi;
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
MaskedIcon::LoadResource(ResourceId id, ResourceId mdpi_id,
                         ResourceId xhdpi_id, ResourceId xxhdpi_id,
                         bool center)
{
  /* pick the variant whose density bucket is nearest to the display;
     boundaries are midway between the buckets */
  const unsigned density = DisplayDensity();

#ifdef ENABLE_OPENGL
  unsigned source_dpi = ICON_LDPI;
  if (density >= 400 && xxhdpi_id.IsDefined()) {
    id = xxhdpi_id;
    source_dpi = ICON_XXHDPI;
  } else if (density >= 240 && xhdpi_id.IsDefined()) {
    id = xhdpi_id;
    source_dpi = ICON_XHDPI;
  } else if (density >= 128 && mdpi_id.IsDefined()) {
    id = mdpi_id;
    source_dpi = ICON_MDPI;
  }

  const unsigned stretch =
    IconStretchFixed10(source_dpi * ICON_SUPERSAMPLE);
  bitmap.Load(id);
#else
  if (Layout::vdpi >= 120) {
    /* switch to the larger variants at 120dpi */

    unsigned source_dpi = ICON_LDPI;
    if (density >= 400 && xxhdpi_id.IsDefined()) {
      id = xxhdpi_id;
      source_dpi = ICON_XXHDPI;
    } else if (density >= 240 && xhdpi_id.IsDefined()) {
      id = xhdpi_id;
      source_dpi = ICON_XHDPI;
    } else if (mdpi_id.IsDefined()) {
      id = mdpi_id;
      source_dpi = ICON_MDPI;
    }

    bitmap.LoadStretch(id, IconStretchInteger(source_dpi));
  } else
    bitmap.Load(id);
#endif

  assert(IsDefined());

  has_colors = bitmap.HasColors();

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
  /* GDI uses current HDC colors when blitting from a monochrome
     bitmap; ensure black foreground / white background */
  const Color old_text_color = canvas.GetTextColor();
  const Color old_bg_color = canvas.GetBackgroundColor();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);
#endif

  canvas.CopyOr(p, size, bitmap, {0, 0});
  canvas.CopyAnd(p, size, bitmap, {(int)size.width, 0});

#ifdef USE_GDI
  canvas.SetTextColor(old_text_color);
  canvas.SetBackgroundColor(old_bg_color);
#endif
#endif
}

void
MaskedIcon::Draw(Canvas &canvas, PixelPoint p,
                 unsigned target_height) const noexcept
{
  assert(IsDefined());

  if (target_height == 0 || target_height == size.height) {
    Draw(canvas, p);
    return;
  }

  if (size.height == 0)
    return;

  /* uniformly scaled size */
  const PixelSize scaled_size = {
    size.width * target_height / size.height,
    target_height,
  };

  /* scale the hotspot (origin) proportionally */
  const PixelPoint dest = {
    p.x - int(origin.x * target_height / size.height),
    p.y - int(origin.y * target_height / size.height),
  };

#ifdef ENABLE_OPENGL
  const bool inverse = !has_colors &&
    IsDarkBackground(canvas.GetTextColor());

  if (inverse)
    OpenGL::invert_shader->Use();
  else
    OpenGL::texture_shader->Use();

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(dest, scaled_size), texture.GetRect());
#elif defined(USE_GDI)
  const Color old_text_color = canvas.GetTextColor();
  const bool inverse = !has_colors &&
    IsDarkBackground(old_text_color);

  const Color old_bg_color = canvas.GetBackgroundColor();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);

  if (inverse) {
    canvas.Stretch(dest, scaled_size,
                   bitmap, {(int)size.width, 0}, size, MERGEPAINT);
  } else {
    canvas.Stretch(dest, scaled_size,
                   bitmap, {0, 0}, size, SRCPAINT);
    canvas.Stretch(dest, scaled_size,
                   bitmap, {(int)size.width, 0}, size, SRCAND);
  }

  canvas.SetTextColor(old_text_color);
  canvas.SetBackgroundColor(old_bg_color);
#else
  /* memory canvas: stretch each half (mask / icon) into a temporary
     surface, then composite with CopyOr + CopyAnd (or CopyNotOr for
     dark-mode inversion). */
  const Color old_text_color = canvas.GetTextColor();
  const bool inverse = !has_colors &&
    IsDarkBackground(old_text_color);

  VirtualCanvas temp{scaled_size};

  if (inverse) {
    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {(int)size.width, 0}, size);
    canvas.CopyNotOr(dest, scaled_size, temp, {0, 0});
  } else {
    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {0, 0}, size);
    canvas.CopyOr(dest, scaled_size, temp, {0, 0});

    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {(int)size.width, 0}, size);
    canvas.CopyAnd(dest, scaled_size, temp, {0, 0});
  }
#endif
}

void
MaskedIcon::Draw(Canvas &canvas, const PixelRect &rc,
                 [[maybe_unused]] bool inverse) const noexcept
{
  const PixelPoint position = rc.CenteredTopLeft(size);

#ifdef ENABLE_OPENGL
  /* detect dark backgrounds from the caller's text color rather than
     relying on the "inverse" parameter, which may not reflect the
     actual background (e.g. TabRenderer passes "selected" as
     inverse, but in dark mode *all* tabs have dark backgrounds).
     Skip inversion for colour icons (has_colors). */
  const bool dark_bg = !has_colors &&
    IsDarkBackground(canvas.GetTextColor());

  if (dark_bg)
    OpenGL::invert_shader->Use();
  else
    OpenGL::texture_shader->Use();

  const ScopeAlphaBlend alpha_blend;

  GLTexture &texture = *bitmap.GetNative();
  texture.Bind();
  texture.Draw(PixelRect(position, size), texture.GetRect());
#else

  /* detect dark backgrounds from the caller's text color rather than
     relying on the "inverse" parameter, which may not reflect the
     actual background (e.g. TabRenderer passes "selected" as
     inverse, but in dark mode *all* tabs have dark backgrounds).
     Skip inversion for colour icons (has_colors). */
  const Color old_text_color = canvas.GetTextColor();
  const bool dark_bg = !has_colors &&
    IsDarkBackground(old_text_color);

#ifdef USE_GDI
  /* GDI uses current HDC colors when blitting from a monochrome
     bitmap; ensure black foreground / white background */
  const Color old_bg_color = canvas.GetBackgroundColor();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);
#endif

  if (dark_bg) {
    canvas.CopyNotOr(position, size, bitmap, {(int)size.width, 0});
  } else {
    canvas.CopyOr(position, size, bitmap, {0, 0});
    canvas.CopyAnd(position, size, bitmap, {(int)size.width, 0});
  }

#ifdef USE_GDI
  canvas.SetTextColor(old_text_color);
  canvas.SetBackgroundColor(old_bg_color);
#endif
#endif
}
