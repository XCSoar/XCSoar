/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Util.hpp"
#include "Format.hpp"

#ifdef GREYSCALE
#include "Screen/Custom/PixelOperations.hpp"
#include "Screen/Custom/PixelTraits.hpp"
#include "Screen/Custom/RasterCanvas.hpp"
#endif

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <winuser.h>

#ifndef GREYSCALE
#include <SDL_rotozoom.h>
#include <SDL_imageFilter.h>
#endif

#include <SDL_gfxBlitFunc.h>

#ifdef GREYSCALE

using SDLPixelTraits = GreyscalePixelTraits;

static inline unsigned
ClipMax(unsigned limit, int offset, unsigned size)
{
  return std::min(unsigned(size),
                  unsigned(std::max(int(limit - offset), 0)));
}

class SDLRasterCanvas : public RasterCanvas<SDLPixelTraits> {
public:
  SDLRasterCanvas(SDL_Surface *surface, RasterPoint offset, PixelSize size)
    :RasterCanvas<SDLPixelTraits>(SDLPixelTraits::At(SDLPixelTraits::pointer_type(surface->pixels),
                                                     surface->pitch,
                                                     offset.x, offset.y),
                                  surface->pitch,
                                  ClipMax(surface->w, offset.x, size.cx),
                                  ClipMax(surface->h, offset.y, size.cy)) {}

  static constexpr SDLPixelTraits::color_type Import(Color color) {
    return color.GetLuminosity();
  }
};

#endif

void
Canvas::Destroy()
{
  if (surface != NULL) {
    SDL_FreeSurface(surface);
    surface = NULL;
  }
}

#ifdef GREYSCALE

void
Canvas::DrawOutlineRectangle(PixelScalar left, PixelScalar top,
                             PixelScalar right, PixelScalar bottom,
                             Color color)
{
  SDLRasterCanvas canvas(surface, offset, size);
  canvas.DrawRectangle(left, top, right, bottom,
                       canvas.Import(color));
}

#endif

void
Canvas::DrawPolyline(const RasterPoint *p, unsigned cPoints)
{
#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  const unsigned thickness = pen.GetWidth();
  const auto color = canvas.Import(pen.GetColor());

  if (thickness > 1)
    for (unsigned i = 1; i < cPoints; ++i)
      canvas.DrawThickLine(p[i - 1].x, p[i - 1].y, p[i].x, p[i].y,
                           thickness, color);
  else
    for (unsigned i = 1; i < cPoints; ++i)
      canvas.DrawLine(p[i - 1].x, p[i - 1].y, p[i].x, p[i].y,
                      color);

#else
  const Uint32 color = pen.GetColor().GFXColor();

#if SDL_GFXPRIMITIVES_MAJOR > 2 || \
  (SDL_GFXPRIMITIVES_MAJOR == 2 && (SDL_GFXPRIMITIVES_MINOR > 0 || \
                                    SDL_GFXPRIMITIVES_MICRO >= 22))
  /* thickLineColor() was added in SDL_gfx 2.0.22 */
  const unsigned width = pen.GetWidth();
  if (width > 1)
    for (unsigned i = 1; i < cPoints; ++i)
      ::thickLineColor(surface, p[i - 1].x, p[i - 1].y, p[i].x, p[i].y,
                       width, color);
  else
#endif
    for (unsigned i = 1; i < cPoints; ++i)
      ::lineColor(surface, p[i - 1].x, p[i - 1].y, p[i].x, p[i].y, color);
#endif
}

void
Canvas::DrawPolygon(const RasterPoint *lppt, unsigned cPoints)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  SDLRasterCanvas::Point points[cPoints];
  for (unsigned i = 0; i < cPoints; ++i) {
    points[i].x = lppt[i].x;
    points[i].y = lppt[i].y;
  }

  if (!brush.IsHollow()) {
    const auto color = canvas.Import(brush.GetColor());
    if (brush.GetColor().IsOpaque())
      canvas.FillPolygon(points, cPoints, color);
    else
      canvas.FillPolygon(points, cPoints, color,
                         AlphaPixelOperations<SDLPixelTraits>(brush.GetColor().Alpha()));
  }

  if (IsPenOverBrush()) {
    const unsigned thickness = pen.GetWidth();
    const auto color = canvas.Import(pen.GetColor());

    if (thickness > 1) {
      for (unsigned i = 1; i < cPoints; ++i)
        canvas.DrawThickLine(points[i - 1].x, points[i - 1].y,
                             points[i].x, points[i].y,
                             thickness, color);
      canvas.DrawThickLine(points[cPoints - 1].x, points[cPoints - 1].y,
                           points[0].x, points[0].y,
                           thickness, color);
    } else {
      for (unsigned i = 1; i < cPoints; ++i)
        canvas.DrawLine(points[i - 1].x, points[i - 1].y,
                        points[i].x, points[i].y,
                        color);
      canvas.DrawLine(points[cPoints - 1].x, points[cPoints - 1].y,
                      points[0].x, points[0].y,
                      color);
    }
  }

#else
  Sint16 vx[cPoints], vy[cPoints];

  for (unsigned i = 0; i < cPoints; ++i) {
    vx[i] = offset.x + lppt[i].x;
    vy[i] = offset.y + lppt[i].y;
  }

  if (!brush.IsHollow())
    ::filledPolygonColor(surface, vx, vy, cPoints,
                         brush.GetColor().GFXColor());

  if (IsPenOverBrush()) {
    const Uint32 color = pen.GetColor().GFXColor();

#if SDL_GFXPRIMITIVES_MAJOR > 2 || \
  (SDL_GFXPRIMITIVES_MAJOR == 2 && (SDL_GFXPRIMITIVES_MINOR > 0 || \
                                    SDL_GFXPRIMITIVES_MICRO >= 22))
    /* thickLineColor() was added in SDL_gfx 2.0.22 */
    const unsigned width = pen.GetWidth();
    if (width > 1) {
      for (unsigned i = 1; i < cPoints; ++i)
        ::thickLineColor(surface, vx[i - 1], vy[i - 1], vx[i], vy[i],
                         width, color);
      ::thickLineColor(surface, vx[cPoints - 1], vy[cPoints - 1], vx[0], vy[0],
                       width, color);
    } else
#endif
      ::polygonColor(surface, vx, vy, cPoints, color);
  }
#endif
}

#ifdef GREYSCALE

void
Canvas::DrawLine(int ax, int ay, int bx, int by)
{
  const unsigned thickness = pen.GetWidth();

  SDLRasterCanvas canvas(surface, offset, size);
  const auto color = canvas.Import(pen.GetColor());
  if (thickness > 1)
    canvas.DrawThickLine(ax, ay, bx, by, thickness, color);
  else
    canvas.DrawLine(ax, ay, bx, by, color);
}

#endif

void
Canvas::DrawCircle(PixelScalar x, PixelScalar y, UPixelScalar radius)
{
#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);
#endif

  if (!brush.IsHollow()) {
#ifdef GREYSCALE
    const auto color = canvas.Import(brush.GetColor());

    if (brush.GetColor().IsOpaque())
      canvas.FillCircle(x, y, radius, color);
    else
      canvas.FillCircle(x, y, radius, color,
                        AlphaPixelOperations<SDLPixelTraits>(brush.GetColor().Alpha()));
#else
    ::filledCircleColor(surface, x, y, radius,
                        brush.GetColor().GFXColor());
#endif
  }

  if (IsPenOverBrush()) {
    if (pen.GetWidth() < 2) {
#ifdef GREYSCALE
      canvas.DrawCircle(x, y, radius, canvas.Import(pen.GetColor()));
#else
      ::circleColor(surface, x, y, radius, pen.GetColor().GFXColor());
#endif
      return;
    }

    // no thickCircleColor in SDL_gfx, so need to emulate it with multiple draws (slow!)
    for (int i= (pen.GetWidth()/2); i>= -(int)(pen.GetWidth()-1)/2; --i) {
#ifdef GREYSCALE
      canvas.DrawCircle(x, y, radius + i, canvas.Import(pen.GetColor()));
#else
      ::circleColor(surface, x, y, radius+i, pen.GetColor().GFXColor());
#endif
    }
  }
}

void
Canvas::DrawSegment(PixelScalar x, PixelScalar y, UPixelScalar radius,
                    Angle start, Angle end, bool horizon)
{
  Segment(*this, x, y, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(PixelScalar x, PixelScalar y,
                    UPixelScalar small_radius, UPixelScalar big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(PixelScalar x, PixelScalar y,
                    UPixelScalar small_radius, UPixelScalar big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

const PixelSize
Canvas::CalcTextSize(const TCHAR *text) const
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  PixelSize size = { 0, 0 };

  if (font == NULL)
    return size;

  return font->TextSize(text);
}

static SDL_Surface *
RenderText(const Font *font, const TCHAR *text)
{
  if (font == nullptr)
    return nullptr;

  assert(font->IsDefined());

#ifdef USE_FREETYPE
  PixelSize size = font->TextSize(text);
  if (size.cx == 0 && size.cy == 0)
    return nullptr;

  SDL_Surface *s = ::SDL_CreateRGBSurface(SDL_SWSURFACE, size.cx, size.cy,
                                          8, 0xff, 0xff, 0xff, 0x00);
  if (s == nullptr)
    return nullptr;

  size.cx = s->pitch;

  font->Render(text, size, s->pixels);
  return s;
#endif
}

void
Canvas::DrawText(PixelScalar x, PixelScalar y, const TCHAR *text)
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  SDL_Surface *s = RenderText(font, text);
  if (s == NULL)
    return;

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  if (background_mode == OPAQUE) {
    OpaqueAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      opaque(canvas.Import(background_color), canvas.Import(text_color));
    canvas.CopyRectangle(x, y, s->w, s->h,
                         GreyscalePixelTraits::const_pointer_type(s->pixels),
                         s->pitch, opaque);
  } else {
    ColoredAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      transparent(canvas.Import(text_color));
    canvas.CopyRectangle(x, y, s->w, s->h,
                         GreyscalePixelTraits::const_pointer_type(s->pixels),
                         s->pitch, transparent);
  }

#else
  if (s->format->palette != NULL && s->format->palette->ncolors >= 2) {
    s->format->palette->colors[1] = text_color;

    if (background_mode == OPAQUE) {
      s->flags &= ~SDL_SRCCOLORKEY;
      s->format->palette->colors[0] = background_color;
    }
  }

  Copy(x, y, s);
#endif

  ::SDL_FreeSurface(s);
}

void
Canvas::DrawTransparentText(PixelScalar x, PixelScalar y, const TCHAR *text)
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  SDL_Surface *s = RenderText(font, text);
  if (s == NULL)
    return;

  if (s->format->palette != NULL && s->format->palette->ncolors >= 2)
    s->format->palette->colors[1] = text_color;

  Copy(x, y, s);
  ::SDL_FreeSurface(s);
}

static bool
Clip(PixelScalar &position, UPixelScalar &length, UPixelScalar max,
     PixelScalar &src_position)
{
  if (position < 0) {
    if (length <= (UPixelScalar)-position)
      return false;

    length -= -position;
    src_position -= position;
    position = 0;
  }

  if ((UPixelScalar)position >= max)
    return false;

  if (position + length >= max)
    length = max - position;

  return true;
}

void
Canvas::Copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             SDL_Surface *src_surface, PixelScalar src_x, PixelScalar src_y)
{
  assert(src_surface != NULL);

  if (!Clip(dest_x, dest_width, GetWidth(), src_x) ||
      !Clip(dest_y, dest_height, GetHeight(), src_y))
    return;

#ifdef GREYSCALE
  assert(surface->format->BytesPerPixel == 1);
  assert(src_surface->format->BytesPerPixel == 1);

  if ((src_surface->flags & SDL_SRCCOLORKEY) == 0 &&
      (src_surface->format->palette == nullptr ||
       src_surface->format->palette->ncolors == 0x100)) {
    /* optimised fast path for greyscale -> greyscale blitting */

    SDLRasterCanvas canvas(surface, offset, size);
    canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                         SDLPixelTraits::const_pointer_type(src_surface->pixels),
                         src_surface->pitch);

    return;
  }
#endif

  dest_x += offset.x;
  dest_y += offset.y;

  SDL_Rect src_rect = { src_x, src_y, dest_width, dest_height };
  SDL_Rect dest_rect = { dest_x, dest_y };

  ::SDL_BlitSurface(src_surface, &src_rect, surface, &dest_rect);
}

void
Canvas::Copy(const Canvas &src, PixelScalar src_x, PixelScalar src_y)
{
  Copy(0, 0, src.GetWidth(), src.GetHeight(), src, src_x, src_y);
}

void
Canvas::Copy(const Canvas &src)
{
  Copy(src, 0, 0);
}

void
Canvas::Copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  Copy(dest_x, dest_y, dest_width, dest_height,
       src.GetNative(), src_x, src_y);
}

void
Canvas::Copy(const Bitmap &src)
{
  SDL_Surface *surface = src.GetNative();
  Copy(0, 0, surface->w, surface->h, surface, 0, 0);
}

void
Canvas::CopyTransparentWhite(const Canvas &src)
{
  assert(src.surface != NULL);

  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY, src.map(COLOR_WHITE));
  Copy(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::CopyTransparentBlack(const Canvas &src)
{
  assert(src.surface != NULL);

  ::SDL_SetColorKey(src.surface, SDL_SRCCOLORKEY, src.map(COLOR_BLACK));
  Copy(src);
  ::SDL_SetColorKey(src.surface, 0, 0);
}

void
Canvas::StretchTransparent(const Bitmap &src, Color key)
{
  assert(src.IsDefined());

  SDL_Surface *surface = src.GetNative();

  ::SDL_SetColorKey(surface, SDL_SRCCOLORKEY, key.Map(surface->format));
  Stretch(surface);
  ::SDL_SetColorKey(surface, 0, 0);
}

void
Canvas::InvertStretchTransparent(const Bitmap &src, Color key)
{
  assert(src.IsDefined());

  SDL_Surface *src_surface = src.GetNative();
  const UPixelScalar src_x = 0, src_y = 0;
  const UPixelScalar src_width = src_surface->w;
  const UPixelScalar src_height = src_surface->h;
  const UPixelScalar dest_x = 0, dest_y = 0;
  const UPixelScalar dest_width = GetWidth();
  const UPixelScalar dest_height = GetHeight();

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src_surface->pixels),
                                           src_surface->pitch, src_x, src_y),
                        src_surface->pitch, src_width, src_height,
                        TransparentInvertPixelOperations<SDLPixelTraits>(canvas.Import(key)));

#else

  SDL_Surface *zoomed =
    ::zoomSurface(src_surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, SDL_SRCCOLORKEY, key.Map(zoomed->format));

  CopyNot(dest_x, dest_y, dest_width, dest_height,
           zoomed, (src_x * dest_width) / src_width,
           (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
#endif
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                SDL_Surface *src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
  assert(src != NULL);
  assert(dest_width < 0x4000);
  assert(dest_height < 0x4000);

  if (dest_width == src_width && dest_height == src_height) {
    /* fast path: no zooming needed */
    Copy(dest_x, dest_y, dest_width, dest_height, src, src_x, src_y);
    return;
  }

  if (dest_width >= 0x4000 || dest_height >= 0x4000)
    /* paranoid sanity check; shouldn't ever happen */
    return;

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                           src->pitch, src_x, src_y),
                        src->pitch, src_width, src_height);

#else

  SDL_Surface *zoomed =
    ::zoomSurface(src, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);

  if (zoomed == NULL)
    return;

  ::SDL_SetColorKey(zoomed, 0, 0);

  Copy(dest_x, dest_y, dest_width, dest_height,
       zoomed, (src_x * dest_width) / src_width,
       (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
#endif
}

void
Canvas::Stretch(const Canvas &src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
  // XXX
  Stretch(0, 0, GetWidth(), GetHeight(),
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(),
          src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src)
{
  assert(IsDefined());
  assert(src.IsDefined());

  SDL_Surface *surface = src.GetNative();
  Stretch(dest_x, dest_y, dest_width, dest_height,
          surface, 0, 0, surface->w, surface->h);
}

void
Canvas::StretchMono(PixelScalar dest_x, PixelScalar dest_y,
                    UPixelScalar dest_width, UPixelScalar dest_height,
                    const Bitmap &src,
                    PixelScalar src_x, PixelScalar src_y,
                    UPixelScalar src_width, UPixelScalar src_height,
                    Color fg_color, Color bg_color)
{
  assert(IsDefined());
  assert(src.IsDefined());
  assert(dest_width < 0x4000);
  assert(dest_height < 0x4000);

  if (dest_width >= 0x4000 || dest_height >= 0x4000)
    /* paranoid sanity check; shouldn't ever happen */
    return;

  SDL_Surface *src_surface = src.GetNative();

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  OpaqueTextPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
    opaque(canvas.Import(fg_color), canvas.Import(bg_color));

  canvas.ScaleRectangle<decltype(opaque), GreyscalePixelTraits>
    (dest_x, dest_y,
     dest_width, dest_height,
     GreyscalePixelTraits::At(GreyscalePixelTraits::const_pointer_type(src_surface->pixels),
                              src_surface->pitch, src_x, src_y),
     src_surface->pitch, src_width, src_height,
     opaque);

#else

  assert(src_surface->format->palette != NULL &&
         src_surface->format->palette->ncolors == 256);

  SDL_Surface *zoomed =
    ::zoomSurface(src_surface, (double)dest_width / (double)src_width,
                  (double)dest_height / (double)src_height,
                  SMOOTHING_OFF);
  if (zoomed == NULL)
    return;

  assert(zoomed->format->palette != NULL &&
         zoomed->format->palette->ncolors == 256);

  ::SDL_SetColorKey(zoomed, 0, 0);
  zoomed->format->palette->colors[0] = text_color;
  zoomed->format->palette->colors[255] = bg_color;

  Copy(dest_x, dest_y, dest_width, dest_height,
       zoomed, (src_x * dest_width) / src_width,
       (src_y * dest_height) / src_height);
  ::SDL_FreeSurface(zoomed);
#endif
}

#ifndef GREYSCALE

static bool
ClipRange(PixelScalar &a, UPixelScalar a_size,
          PixelScalar &b, UPixelScalar b_size,
          UPixelScalar &size)
{
  if (a < 0) {
    b -= a;
    size += a;
    a = 0;
  }

  if (b < 0) {
    a -= b;
    size += b;
    b = 0;
  }

  if ((PixelScalar)size <= 0)
    return false;

  if (a + size > a_size)
    size = a_size - a;

  if ((PixelScalar)size <= 0)
    return false;

  if (b + size > b_size)
    size = b_size - b;

  return (PixelScalar)size > 0;
}

static void
blit_not(SDL_Surface *dest, PixelScalar dest_x, PixelScalar dest_y,
         UPixelScalar dest_width, UPixelScalar dest_height,
         SDL_Surface *_src, PixelScalar src_x, PixelScalar src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!ClipRange(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !ClipRange(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = LazyConvertSurface(_src, dest->format);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    if (src != _src)
      ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  for (UPixelScalar y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitNegation(src_buffer, dest_buffer,
                                 dest_width * dest->format->BytesPerPixel);
    src_buffer += src->pitch;
    dest_buffer += dest->pitch;
  }

  /* cleanup */

  ::SDL_UnlockSurface(src);
  if (src != _src)
    ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

static void
blit_or(SDL_Surface *dest, PixelScalar dest_x, PixelScalar dest_y,
        UPixelScalar dest_width, UPixelScalar dest_height,
        SDL_Surface *_src, PixelScalar src_x, PixelScalar src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!ClipRange(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !ClipRange(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = LazyConvertSurface(_src, dest->format);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    if (src != _src)
      ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  for (UPixelScalar y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitOr(src_buffer, dest_buffer, dest_buffer,
                           dest_width * dest->format->BytesPerPixel);
    src_buffer += src->pitch;
    dest_buffer += dest->pitch;
  }

  /* cleanup */

  ::SDL_UnlockSurface(src);
  if (src != _src)
    ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

static void
BlitNotOr(SDL_Surface *dest, PixelScalar dest_x, PixelScalar dest_y,
          UPixelScalar dest_width, UPixelScalar dest_height,
          SDL_Surface *_src, PixelScalar src_x, PixelScalar src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!ClipRange(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !ClipRange(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = LazyConvertSurface(_src, dest->format);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    if (src != _src)
      ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  const size_t line_size = dest_width * dest->format->BytesPerPixel;
  unsigned char *tmp = new unsigned char[line_size];

  for (UPixelScalar y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitNegation(src_buffer, tmp, line_size);
    src_buffer += src->pitch;
    ::SDL_imageFilterBitOr(tmp, dest_buffer, dest_buffer, line_size);
    dest_buffer += dest->pitch;
  }

  delete[] tmp;

  /* cleanup */

  ::SDL_UnlockSurface(src);
  if (src != _src)
    ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

static void
blit_and(SDL_Surface *dest, PixelScalar dest_x, PixelScalar dest_y,
         UPixelScalar dest_width, UPixelScalar dest_height,
         SDL_Surface *_src, PixelScalar src_x, PixelScalar src_y)
{
  int ret;

  /* obey the dest and src surface borders */

  if (!ClipRange(dest_x, dest->w, src_x, _src->w, dest_width) ||
      !ClipRange(dest_y, dest->h, src_y, _src->h, dest_height))
    return;

  ret = ::SDL_LockSurface(dest);
  if (ret != 0)
    return;

  /* convert src's pixel format */

  SDL_Surface *src = LazyConvertSurface(_src, dest->format);
  if (src == NULL) {
    ::SDL_UnlockSurface(dest);
    return;
  }

  ret = ::SDL_LockSurface(src);
  if (ret != 0) {
    if (src != _src)
      ::SDL_FreeSurface(src);
    ::SDL_UnlockSurface(dest);
    return;
  }

  /* get pointers to the upper left dest/src pixel */

  unsigned char *dest_buffer = (unsigned char *)dest->pixels;
  dest_buffer += dest_y * dest->pitch +
    dest_x * dest->format->BytesPerPixel;

  unsigned char *src_buffer = (unsigned char *)src->pixels;
  src_buffer += src_y * src->pitch +
    src_x * src->format->BytesPerPixel;

  /* copy line by line */

  for (UPixelScalar y = 0; y < dest_height; ++y) {
    ::SDL_imageFilterBitAnd(src_buffer, dest_buffer, dest_buffer,
                            dest_width * dest->format->BytesPerPixel);
    src_buffer += src->pitch;
    dest_buffer += dest->pitch;
  }

  /* cleanup */

  ::SDL_UnlockSurface(src);
  if (src != _src)
    ::SDL_FreeSurface(src);
  ::SDL_UnlockSurface(dest);
}

#endif

void
Canvas::CopyNot(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitNotPixelOperations<SDLPixelTraits>());

#else
  dest_x += offset.x;
  dest_y += offset.y;

  ::blit_not(surface, dest_x, dest_y, dest_width, dest_height,
             src, src_x, src_y);
#endif
}

void
Canvas::CopyOr(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitOrPixelOperations<SDLPixelTraits>());

#else
  dest_x += offset.x;
  dest_y += offset.y;

  ::blit_or(surface, dest_x, dest_y, dest_width, dest_height,
            src, src_x, src_y);
#endif
}

void
Canvas::CopyNotOr(PixelScalar dest_x, PixelScalar dest_y,
                  UPixelScalar dest_width, UPixelScalar dest_height,
                  SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitNotOrPixelOperations<SDLPixelTraits>());

#else
  dest_x += offset.x;
  dest_y += offset.y;

  ::BlitNotOr(surface, dest_x, dest_y, dest_width, dest_height,
              src, src_x, src_y);
#endif
}

void
Canvas::CopyNotOr(PixelScalar dest_x, PixelScalar dest_y,
                  UPixelScalar dest_width, UPixelScalar dest_height,
                  const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  CopyNotOr(dest_x, dest_y, dest_width, dest_height,
            src.GetNative(), src_x, src_y);
}

void
Canvas::CopyAnd(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitAndPixelOperations<SDLPixelTraits>());

#else

  dest_x += offset.x;
  dest_y += offset.y;

  ::blit_and(surface, dest_x, dest_y, dest_width, dest_height,
             src, src_x, src_y);
#endif
}

void
Canvas::CopyNot(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  CopyNot(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(), src_x, src_y);
}

void
Canvas::CopyOr(PixelScalar dest_x, PixelScalar dest_y,
               UPixelScalar dest_width, UPixelScalar dest_height,
               const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  CopyOr(dest_x, dest_y, dest_width, dest_height,
         src.GetNative(), src_x, src_y);
}

void
Canvas::CopyAnd(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.IsDefined());

  CopyAnd(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(), src_x, src_y);
}

void
Canvas::DrawRoundRectangle(PixelScalar left, PixelScalar top,
                           PixelScalar right, PixelScalar bottom,
                           UPixelScalar ellipse_width,
                           UPixelScalar ellipse_height)
{
  UPixelScalar radius = std::min(ellipse_width, ellipse_height) / 2;
  ::RoundRect(*this, left, top, right, bottom, radius);
}

void
Canvas::AlphaBlend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   SDL_Surface *src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha)
{
  // TODO: support scaling; SDL_gfx doesn't implement it
  // TODO: this method assumes 32 bit RGBA; but what about RGB 565?

#ifdef GREYSCALE
  SDLRasterCanvas canvas(surface, offset, size);
  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::const_pointer_type(src->pixels), src->pitch,
                       AlphaPixelOperations<SDLPixelTraits>(alpha));
#else
  ::SDL_gfxSetAlpha(src, alpha);

  SDL_PixelFormat *format = src->format;
  if (format->Rmask == 0xff0000 || format->Rmask == 0xff) {
    src->format->Aloss = 0;
    src->format->Ashift = 24;
    src->format->Amask = 0xff000000;
  } else if (format->Rmask == 0xff000000 || format->Rmask == 0xff00) {
    src->format->Aloss = 0;
    src->format->Ashift = 0;
    src->format->Amask = 0xff;
  }

  SDL_Rect src_rect = {
    Sint16(src_x), Sint16(src_y), Uint16(src_width), Uint16(src_height)
  };
  SDL_Rect dest_rect = {
    Sint16(dest_x), Sint16(dest_y), Uint16(dest_width), Uint16(dest_height)
  };
  ::SDL_gfxBlitRGBA(src, &src_rect, surface, &dest_rect);
#endif
}

void
Canvas::AlphaBlend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   const Canvas &src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha)
{
  AlphaBlend(dest_x, dest_y, dest_width, dest_height,
             src.surface, src_x, src_y, src_width, src_height,
             alpha);
}
