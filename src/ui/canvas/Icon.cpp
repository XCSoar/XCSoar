// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Icon.hpp"
#include "Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "Math/Angle.hpp"
#include "Math/FastRotation.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "opengl/Texture.hpp"
#include "opengl/Scope.hpp"

#include "opengl/Shaders.hpp"
#include "opengl/Program.hpp"
#include "opengl/VertexPointer.hpp"
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
  /* On OpenGL the GPU scales textures efficiently, so always load
     the highest-resolution variant for maximum quality (especially
     beneficial when icons are scaled up in list views). */
  unsigned source_dpi = 96;
  if (ultra_id.IsDefined()) {
    id = ultra_id;
    source_dpi = 300;
  } else if (big_id.IsDefined()) {
    id = big_id;
    source_dpi = 192;
  }

  const unsigned stretch = IconStretchFixed10(source_dpi);
  bitmap.Load(id);
#else
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
                 unsigned target_height, const Angle angle) const noexcept
{
  Draw(canvas, p, target_height, angle, COLOR_WHITE);
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

  const PixelSize scaled_size = {
    size.width * target_height / size.height,
    target_height,
  };

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

#ifdef ENABLE_OPENGL
static void
DrawTrafficIconQuad(const PixelSize &icon_size, GLTexture &texture,
                    PixelPoint p, unsigned icon_height, Angle angle,
                    Color body_color, Color outline_color) noexcept
{
  if (icon_height == 0 || icon_size.height == 0)
    return;

  const PixelSize scaled_size = {
    icon_size.width * icon_height / icon_size.height,
    icon_height,
  };

  OpenGL::traffic_icon_shader->Use();
  body_color.Uniform(OpenGL::traffic_icon_body_color);
  outline_color.Uniform(OpenGL::traffic_icon_outline_color);

  texture.Bind();

  BulkPixelPoint vertices[4];
  if (angle != Angle::Zero()) {
    const int half_width = int(scaled_size.width / 2);
    const int half_height = int(scaled_size.height / 2);
    const FastRotation rotation(angle);
    const auto corner = [&](int x, int y) {
      const auto rotated =
        rotation.Rotate(DoublePoint2D{double(x), double(y)});
      return PixelPoint{
        p.x + int(rotated.x),
        p.y + int(rotated.y),
      };
    };

    vertices[0] = corner(-half_width, -half_height);
    vertices[1] = corner(half_width, -half_height);
    vertices[2] = corner(-half_width, half_height);
    vertices[3] = corner(half_width, half_height);
  } else {
    const PixelPoint dest = {
      p.x - int(icon_size.width * icon_height / icon_size.height / 2),
      p.y - int(icon_height / 2),
    };
    const PixelRect rect(dest, scaled_size);
    vertices[0] = rect.GetTopLeft();
    vertices[1] = rect.GetTopRight();
    vertices[2] = rect.GetBottomLeft();
    vertices[3] = rect.GetBottomRight();
  }

  const ScopeVertexPointer vp(vertices);

  const PixelRect src = texture.GetRect();
  const PixelSize allocated = texture.GetAllocatedSize();
  const GLfloat x0 = GLfloat(src.left) / allocated.width;
  const GLfloat y0 = GLfloat(src.top) / allocated.height;
  const GLfloat x1 = GLfloat(src.right) / allocated.width;
  const GLfloat y1 = GLfloat(src.bottom) / allocated.height;
  const bool flipped = texture.IsFlipped();

  const GLfloat coord[] = {
    x0, flipped ? y1 : y0,
    x1, flipped ? y1 : y0,
    x0, flipped ? y0 : y1,
    x1, flipped ? y0 : y1,
  };

  glEnableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  glVertexAttribPointer(OpenGL::Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, coord);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
}
#endif

void
MaskedIcon::Draw(Canvas &canvas, PixelPoint p,
                 unsigned target_height, const Angle angle,
                 const Color body_color) const noexcept
{
  assert(IsDefined());

  if (target_height == 0 || target_height == size.height) {
    Draw(canvas, p);
    return;
  }

  if (size.height == 0)
    return;

  const Color outline_color = IsDarkBackground(canvas.GetTextColor())
    ? COLOR_WHITE
    : COLOR_BLACK;

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
  GLTexture &texture = *bitmap.GetNative();
  DrawTrafficIconQuad(size, texture, p, target_height, angle,
                      body_color, outline_color);
#else
  const PixelSize scaled_size = {
    size.width * target_height / size.height,
    target_height,
  };
  const PixelPoint dest = {
    p.x - int(origin.x * target_height / size.height),
    p.y - int(origin.y * target_height / size.height),
  };

  DrawMonochromeIcon(bitmap, size, canvas, dest, scaled_size, false);
#endif
}

#if !defined(ENABLE_OPENGL)
static void
DrawMonochromeIcon(const Bitmap &bitmap, const PixelSize &icon_size,
                   Canvas &canvas, const PixelPoint dest,
                   const PixelSize &scaled_size, bool inverse) noexcept
{
#ifdef USE_GDI
  const Color old_text_color = canvas.GetTextColor();
  const Color old_bg_color = canvas.GetBackgroundColor();
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);

  if (inverse) {
    canvas.Stretch(dest, scaled_size,
                   bitmap, {(int)icon_size.width, 0}, icon_size, MERGEPAINT);
  } else {
    canvas.Stretch(dest, scaled_size,
                   bitmap, {0, 0}, icon_size, SRCPAINT);
    canvas.Stretch(dest, scaled_size,
                   bitmap, {(int)icon_size.width, 0}, icon_size, SRCAND);
  }

  canvas.SetTextColor(old_text_color);
  canvas.SetBackgroundColor(old_bg_color);
#else
  VirtualCanvas temp{scaled_size};

  if (inverse) {
    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {(int)icon_size.width, 0}, icon_size);
    canvas.CopyNotOr(dest, scaled_size, temp, {0, 0});
  } else {
    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {0, 0}, icon_size);
    canvas.CopyOr(dest, scaled_size, temp, {0, 0});

    temp.Stretch({0, 0}, scaled_size,
                 bitmap, {(int)icon_size.width, 0}, icon_size);
    canvas.CopyAnd(dest, scaled_size, temp, {0, 0});
  }
#endif
}
#endif

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
