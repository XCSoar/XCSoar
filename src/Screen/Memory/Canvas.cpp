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
#include "PixelOperations.hpp"
#include "RasterCanvas.hpp"
#include "Screen/Custom/Cache.hpp"

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <winuser.h>

class SDLRasterCanvas : public RasterCanvas<SDLPixelTraits> {
public:
  SDLRasterCanvas(WritableImageBuffer<SDLPixelTraits> buffer)
    :RasterCanvas<SDLPixelTraits>(buffer) {}

  static constexpr SDLPixelTraits::color_type Import(Color color) {
#ifdef GREYSCALE
    return Luminosity8(color.GetLuminosity());
#else
    return BGRA8Color(color.Red(), color.Green(), color.Blue(), color.Alpha());
#endif
  }
};

void
Canvas::DrawOutlineRectangle(int left, int top, int right, int bottom,
                             Color color)
{
  SDLRasterCanvas canvas(buffer);
  canvas.DrawRectangle(left, top, right, bottom,
                       canvas.Import(color));
}

void
Canvas::DrawFilledRectangle(int left, int top, int right, int bottom,
                            Color color)
{
  if (left >= right || top >= bottom)
    return;

  SDLRasterCanvas canvas(buffer);
  canvas.FillRectangle(left, top, right, bottom,
                       canvas.Import(color));
}

void
Canvas::DrawPolyline(const RasterPoint *p, unsigned cPoints)
{
  SDLRasterCanvas canvas(buffer);

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

  SDLRasterCanvas canvas(buffer);

  static_assert(sizeof(RasterPoint) == sizeof(SDLRasterCanvas::Point),
                "Incompatible point types");
  const SDLRasterCanvas::Point *points =
    reinterpret_cast<const SDLRasterCanvas::Point *>(lppt);

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

  SDLRasterCanvas canvas(buffer);
  const auto color = canvas.Import(pen.GetColor());
  if (thickness > 1)
    canvas.DrawThickLine(ax, ay, bx, by, thickness, color);
  else
    canvas.DrawLine(ax, ay, bx, by, color);
}

void
Canvas::DrawCircle(int x, int y, unsigned radius)
{
  SDLRasterCanvas canvas(buffer);

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
Canvas::DrawSegment(int x, int y, unsigned radius,
                    Angle start, Angle end, bool horizon)
{
  Segment(*this, x, y, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(int x, int y,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(int x, int y,
                    unsigned small_radius, unsigned big_radius,
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

  /* see if the TextCache can handle this request */
  size = TextCache::LookupSize(*font, text);
  if (size.cy > 0)
    return size;

  return TextCache::GetSize(*font, text);
}

static TextCache::Result
RenderText(const Font *font, const TCHAR *text)
{
  if (font == nullptr)
    return TextCache::Result::Null();

  assert(font->IsDefined());

#ifdef USE_FREETYPE
  return TextCache::Get(*font, text);
#endif
}

void
Canvas::DrawText(int x, int y, const TCHAR *text)
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  SDLRasterCanvas canvas(buffer);

  if (background_mode == OPAQUE) {
    OpaqueAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      opaque(canvas.Import(background_color), canvas.Import(text_color));
    canvas.CopyRectangle<decltype(opaque), GreyscalePixelTraits>
      (x, y, s.width, s.height,
       GreyscalePixelTraits::const_pointer_type(s.data),
       s.pitch, opaque);
  } else {
    ColoredAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
      transparent(canvas.Import(text_color));
    canvas.CopyRectangle<decltype(transparent), GreyscalePixelTraits>
      (x, y, s.width, s.height,
       GreyscalePixelTraits::const_pointer_type(s.data),
       s.pitch, transparent);
  }
}

void
Canvas::DrawTransparentText(int x, int y, const TCHAR *text)
{
  assert(text != NULL);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  SDLRasterCanvas canvas(buffer);
  ColoredAlphaPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
    transparent(canvas.Import(text_color));
  canvas.CopyRectangle<decltype(transparent), GreyscalePixelTraits>
    (x, y, s.width, s.height,
     GreyscalePixelTraits::const_pointer_type(s.data),
     s.pitch, transparent);
}

static bool
Clip(int &position, unsigned &length, unsigned max,
     int &src_position)
{
  if (position < 0) {
    if (length <= unsigned(-position))
      return false;

    length -= -position;
    src_position -= position;
    position = 0;
  }

  if (unsigned(position) >= max)
    return false;

  if (position + length >= max)
    length = max - position;

  return true;
}

void
Canvas::Copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             ConstImageBuffer src, int src_x, int src_y)
{
  if (!Clip(dest_x, dest_width, GetWidth(), src_x) ||
      !Clip(dest_y, dest_height, GetHeight(), src_y))
    return;

  SDLRasterCanvas canvas(buffer);
  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch);
}

void
Canvas::Copy(const Canvas &src, int src_x, int src_y)
{
  Copy(0, 0, src.GetWidth(), src.GetHeight(), src, src_x, src_y);
}

void
Canvas::Copy(const Canvas &src)
{
  Copy(src, 0, 0);
}

void
Canvas::Copy(int dest_x, int dest_y,
             unsigned dest_width, unsigned dest_height,
             const Bitmap &src, int src_x, int src_y)
{
  Copy(dest_x, dest_y, dest_width, dest_height,
       src.GetNative(), src_x, src_y);
}

void
Canvas::Copy(const Bitmap &_src)
{
  ConstImageBuffer src = _src.GetNative();

  Copy(0, 0, src.width, src.height, src, 0, 0);
}

void
Canvas::CopyTransparentWhite(const Canvas &src)
{
  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<SDLPixelTraits> operations(canvas.Import(COLOR_WHITE));
  canvas.CopyRectangle(0, 0, GetWidth(), GetHeight(),
                       src.buffer.data, src.buffer.pitch,
                       operations);
}

void
Canvas::CopyTransparentBlack(const Canvas &src)
{
  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<SDLPixelTraits> operations(canvas.Import(COLOR_BLACK));
  canvas.CopyRectangle(0, 0, GetWidth(), GetHeight(),
                       src.buffer.data, src.buffer.pitch,
                       operations);
}

void
Canvas::StretchTransparent(const Bitmap &_src, Color key)
{
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();

  SDLRasterCanvas canvas(buffer);
  canvas.ScaleRectangle(0, 0, GetWidth(), GetHeight(),
                        src.data, src.pitch, src.width, src.height,
                        TransparentPixelOperations<SDLPixelTraits>(canvas.Import(key)));
}

void
Canvas::InvertStretchTransparent(const Bitmap &_src, Color key)
{
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();
  const unsigned src_x = 0, src_y = 0;
  const unsigned dest_x = 0, dest_y = 0;
  const unsigned dest_width = GetWidth();
  const unsigned dest_height = GetHeight();

  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<SDLPixelTraits> operations(canvas.Import(key));

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        src.At(src_x, src_y), src.pitch, src.width, src.height,
                        operations);
}

void
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                ConstImageBuffer src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
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

  SDLRasterCanvas canvas(buffer);

  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        src.At(src_x, src_y), src.pitch, src_width, src_height);
}

void
Canvas::Stretch(const Canvas &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  // XXX
  Stretch(0, 0, GetWidth(), GetHeight(),
          src, src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src,
                int src_x, int src_y,
                unsigned src_width, unsigned src_height)
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(),
          src_x, src_y, src_width, src_height);
}

void
Canvas::Stretch(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &_src)
{
  assert(IsDefined());
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();
  Stretch(dest_x, dest_y, dest_width, dest_height,
          src, 0, 0, src.width, src.height);
}

void
Canvas::StretchMono(int dest_x, int dest_y,
                    unsigned dest_width, unsigned dest_height,
                    ::ConstImageBuffer<GreyscalePixelTraits> src,
                    int src_x, int src_y,
                    unsigned src_width, unsigned src_height,
                    Color fg_color, Color bg_color)
{
  assert(IsDefined());
  assert(dest_width < 0x4000);
  assert(dest_height < 0x4000);

  if (dest_width >= 0x4000 || dest_height >= 0x4000)
    /* paranoid sanity check; shouldn't ever happen */
    return;

  SDLRasterCanvas canvas(buffer);

  OpaqueTextPixelOperations<SDLPixelTraits, GreyscalePixelTraits>
    opaque(canvas.Import(fg_color), canvas.Import(bg_color));

  canvas.ScaleRectangle<decltype(opaque), GreyscalePixelTraits>
    (dest_x, dest_y,
     dest_width, dest_height,
     src.At(src_x, src_y), src.pitch, src_width, src_height,
     opaque);
}

void
Canvas::CopyNot(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitNotPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyOr(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitOrPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyNotOr(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitNotOrPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyNotOr(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  const Bitmap &src, int src_x, int src_y)
{
  assert(src.IsDefined());

  CopyNotOr(dest_x, dest_y, dest_width, dest_height,
            src.GetNative(), src_x, src_y);
}

void
Canvas::CopyAnd(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitAndPixelOperations<SDLPixelTraits>());
}

void
Canvas::CopyNot(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y)
{
  assert(src.IsDefined());

  CopyNot(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(), src_x, src_y);
}

void
Canvas::CopyOr(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               const Bitmap &src, int src_x, int src_y)
{
  assert(src.IsDefined());

  CopyOr(dest_x, dest_y, dest_width, dest_height,
         src.GetNative(), src_x, src_y);
}

void
Canvas::CopyAnd(int dest_x, int dest_y,
                unsigned dest_width, unsigned dest_height,
                const Bitmap &src, int src_x, int src_y)
{
  assert(src.IsDefined());

  CopyAnd(dest_x, dest_y, dest_width, dest_height,
          src.GetNative(), src_x, src_y);
}

void
Canvas::CopyAnd(const Bitmap &src)
{
  CopyAnd(0, 0, GetWidth(), GetHeight(),
          src.GetNative(), 0, 0);
}

void
Canvas::DrawRoundRectangle(int left, int top,
                           int right, int bottom,
                           unsigned ellipse_width,
                           unsigned ellipse_height)
{
  unsigned radius = std::min(ellipse_width, ellipse_height) / 2u;
  ::RoundRect(*this, left, top, right, bottom, radius);
}

void
Canvas::AlphaBlend(int dest_x, int dest_y,
                   unsigned dest_width, unsigned dest_height,
                   ConstImageBuffer src,
                   int src_x, int src_y,
                   unsigned src_width, unsigned src_height,
                   uint8_t alpha)
{
  // TODO: support scaling

  SDLRasterCanvas canvas(buffer);
  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
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
             src.buffer,
             src_x, src_y, src_width, src_height,
             alpha);
}
