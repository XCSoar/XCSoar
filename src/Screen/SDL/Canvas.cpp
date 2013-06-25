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
#include "Screen/Custom/PixelOperations.hpp"
#include "Screen/Custom/PixelTraits.hpp"
#include "Screen/Custom/RasterCanvas.hpp"

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <winuser.h>

#ifdef GREYSCALE

using SDLPixelTraits = GreyscalePixelTraits;

#else

using SDLPixelTraits = BGRAPixelTraits;

#endif

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
#ifdef GREYSCALE
    return Luminosity8(color.GetLuminosity());
#else
    return BGRA8Color(color.Red(), color.Green(), color.Blue(), color.Alpha());
#endif
  }
};

void
Canvas::Destroy()
{
  if (surface != NULL) {
    SDL_FreeSurface(surface);
    surface = NULL;
  }
}

void
Canvas::DrawOutlineRectangle(PixelScalar left, PixelScalar top,
                             PixelScalar right, PixelScalar bottom,
                             Color color)
{
  SDLRasterCanvas canvas(surface, offset, size);
  canvas.DrawRectangle(left, top, right, bottom,
                       canvas.Import(color));
}

void
Canvas::DrawPolyline(const RasterPoint *p, unsigned cPoints)
{
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
}

void
Canvas::DrawPolygon(const RasterPoint *lppt, unsigned cPoints)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

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
}

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

void
Canvas::DrawCircle(PixelScalar x, PixelScalar y, UPixelScalar radius)
{
  SDLRasterCanvas canvas(surface, offset, size);

  if (!brush.IsHollow()) {
    const auto color = canvas.Import(brush.GetColor());

    if (brush.GetColor().IsOpaque())
      canvas.FillCircle(x, y, radius, color);
    else
      canvas.FillCircle(x, y, radius, color,
                        AlphaPixelOperations<SDLPixelTraits>(brush.GetColor().Alpha()));
  }

  if (IsPenOverBrush()) {
    if (pen.GetWidth() < 2) {
      canvas.DrawCircle(x, y, radius, canvas.Import(pen.GetColor()));
      return;
    }

    // no thickCircleColor in SDL_gfx, so need to emulate it with multiple draws (slow!)
    for (int i= (pen.GetWidth()/2); i>= -(int)(pen.GetWidth()-1)/2; --i) {
      canvas.DrawCircle(x, y, radius + i, canvas.Import(pen.GetColor()));
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

  SDLRasterCanvas canvas(surface, offset, size);

  if (background_mode == OPAQUE) {
    OpaqueAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      opaque(canvas.Import(background_color), canvas.Import(text_color));
    canvas.CopyRectangle<decltype(opaque), GreyscalePixelTraits>
      (x, y, s->w, s->h,
       GreyscalePixelTraits::const_pointer_type(s->pixels),
       s->pitch, opaque);
  } else {
    ColoredAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      transparent(canvas.Import(text_color));
    canvas.CopyRectangle<decltype(transparent), GreyscalePixelTraits>
      (x, y, s->w, s->h,
       GreyscalePixelTraits::const_pointer_type(s->pixels),
       s->pitch, transparent);
  }

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

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src_surface->pixels),
                                           src_surface->pitch, src_x, src_y),
                        src_surface->pitch, src_width, src_height,
                        TransparentInvertPixelOperations<SDLPixelTraits>(canvas.Import(key)));
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

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                           src->pitch, src_x, src_y),
                        src->pitch, src_width, src_height);
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
  assert(src_surface->format->BytesPerPixel == 1);

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
}

void
Canvas::CopyNot(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitNotPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyOr(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitOrPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyNotOr(PixelScalar dest_x, PixelScalar dest_y,
                  UPixelScalar dest_width, UPixelScalar dest_height,
                  SDL_Surface *src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src != NULL);

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitNotOrPixelOperations<SDLPixelTraits>());
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

  SDLRasterCanvas canvas(surface, offset, size);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::At(SDLPixelTraits::const_pointer_type(src->pixels),
                                          src->pitch, src_x, src_y),
                       src->pitch,
                       BitAndPixelOperations<SDLPixelTraits>());
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
  // TODO: support scaling

  SDLRasterCanvas canvas(surface, offset, size);
  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       SDLPixelTraits::const_pointer_type(src->pixels), src->pitch,
                       AlphaPixelOperations<SDLPixelTraits>(alpha));
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
