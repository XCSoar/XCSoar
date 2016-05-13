/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Optimised.hpp"
#include "RasterCanvas.hpp"
#include "Screen/Custom/Cache.hpp"
#include "Math/Angle.hpp"

#ifdef __ARM_NEON__
#include "NEON.hpp"
#endif

#ifndef NDEBUG
#include "Util/UTF8.hpp"
#endif

#ifdef UNICODE
#include "Util/ConvertString.hpp"
#endif

#include <algorithm>
#include <assert.h>
#include <string.h>

class SDLRasterCanvas : public RasterCanvas<ActivePixelTraits> {
public:
  SDLRasterCanvas(WritableImageBuffer<ActivePixelTraits> buffer)
    :RasterCanvas<ActivePixelTraits>(buffer) {}

  static constexpr ActivePixelTraits::color_type Import(Color color) {
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
Canvas::InvertRectangle(PixelRect r)
{
  if (r.IsEmpty())
    return;

  CopyNot(r.left, r.top, r.GetWidth(), r.GetHeight(),
          buffer, r.left, r.top);
}

template<typename Canvas, typename PixelOperations>
static void
DrawPolyline(Canvas &canvas, PixelOperations operations, const Pen &pen,
             const BulkPixelPoint *lppt, unsigned n_points,
             bool loop)
{
  const unsigned thickness = pen.GetWidth();
  const unsigned mask = pen.GetMask();
  const auto color = canvas.Import(pen.GetColor());

  canvas.DrawPolyline(lppt, n_points, loop, color, thickness, mask);
}

void
Canvas::DrawPolyline(const BulkPixelPoint *p, unsigned cPoints)
{
  SDLRasterCanvas canvas(buffer);
  ::DrawPolyline(canvas, ActivePixelTraits(), pen,
                 p, cPoints, false);
}

void
Canvas::DrawPolygon(const BulkPixelPoint *lppt, unsigned cPoints)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  SDLRasterCanvas canvas(buffer);

  if (!brush.IsHollow()) {
    const auto color = canvas.Import(brush.GetColor());
    if (brush.GetColor().IsOpaque())
      canvas.FillPolygon(lppt, cPoints, color);
    else
      canvas.FillPolygon(lppt, cPoints, color,
                         AlphaPixelOperations<ActivePixelTraits>(brush.GetColor().Alpha()));
  }

  if (IsPenOverBrush())
    ::DrawPolyline(canvas, ActivePixelTraits(), pen,
                   lppt, cPoints, true);
}

void
Canvas::DrawHLine(int x1, int x2, int y, Color color)
{
  SDLRasterCanvas canvas(buffer);
  canvas.DrawHLine(x1, x2, y, canvas.Import(color));
}

void
Canvas::DrawLine(int ax, int ay, int bx, int by)
{
  const unsigned thickness = pen.GetWidth();
  const unsigned mask = pen.GetMask();

  SDLRasterCanvas canvas(buffer);
  const auto color = canvas.Import(pen.GetColor());
  unsigned mask_position = 0;
  if (thickness > 1)
    canvas.DrawThickLine(ax, ay, bx, by, thickness, color,
                         mask, mask_position);
  else
    canvas.DrawLine(ax, ay, bx, by, color, mask);
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
                        AlphaPixelOperations<ActivePixelTraits>(brush.GetColor().Alpha()));
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
Canvas::DrawSegment(PixelPoint center, unsigned radius,
                    Angle start, Angle end, bool horizon)
{
  ::Segment(*this, center, radius, start, end, horizon);
}

void
Canvas::DrawAnnulus(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::Annulus(*this, center, big_radius, start, end, small_radius);
}

void
Canvas::DrawKeyhole(PixelPoint center,
                    unsigned small_radius, unsigned big_radius,
                    Angle start, Angle end)
{
  assert(IsDefined());

  ::KeyHole(*this, center, big_radius, start, end, small_radius);
}

void
Canvas::DrawArc(PixelPoint center, unsigned radius,
                Angle start, Angle end)
{
  assert(IsDefined());

  ::Arc(*this, center, radius, start, end);
}

const PixelSize
Canvas::CalcTextSize(const TCHAR *text) const
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const char* text2 = text;
  assert(ValidateUTF8(text));
#endif

  PixelSize size = { 0, 0 };

  if (font == nullptr)
    return size;

  /* see if the TextCache can handle this request */
  size = TextCache::LookupSize(*font, text2);
  if (size.cy > 0)
    return size;

  return TextCache::GetSize(*font, text2);
}

static TextCache::Result
RenderText(const Font *font, const TCHAR *text)
{
  if (font == nullptr)
    return TextCache::Result::Null();

  assert(font->IsDefined());

#ifdef USE_FREETYPE
#ifdef UNICODE
  return TextCache::Get(*font, WideToUTF8Converter(text));
#else
  return TextCache::Get(*font, text);
#endif
#endif
}

template<typename Operations>
static void
CopyTextRectangle(SDLRasterCanvas &canvas, int x, int y,
                  unsigned width, unsigned height,
                  Operations o, TextCache::Result s)
{
  typedef typename Operations::SourcePixelTraits SourcePixelTraits;
  canvas.CopyRectangle<decltype(o), SourcePixelTraits>
    (x, y, width, height,
     typename SourcePixelTraits::const_pointer_type(s.data),
     s.pitch, o);
}

static void
CopyTextRectangle(SDLRasterCanvas &canvas, int x, int y,
                  unsigned width, unsigned height,
                  TextCache::Result s,
                  Color text_color, Color background_color, bool opaque)
{
  if (opaque) {
    OpaqueAlphaPixelOperations<ActivePixelTraits, GreyscalePixelTraits>
      opaque(canvas.Import(background_color), canvas.Import(text_color));
    CopyTextRectangle(canvas, x, y, width, height, opaque, s);
  } else {
    ColoredAlphaPixelOperations<ActivePixelTraits, GreyscalePixelTraits>
      transparent(canvas.Import(text_color));
    CopyTextRectangle(canvas, x, y, width, height, transparent, s);
  }
}

void
Canvas::DrawText(int x, int y, const TCHAR *text)
{
  assert(text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  SDLRasterCanvas canvas(buffer);
  CopyTextRectangle(canvas, x, y, s.width, s.height, s,
                    text_color, background_color,
                    background_mode == OPAQUE);
}

void
Canvas::DrawTransparentText(int x, int y, const TCHAR *text)
{
  assert(text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  SDLRasterCanvas canvas(buffer);
  ColoredAlphaPixelOperations<ActivePixelTraits, GreyscalePixelTraits>
    transparent(canvas.Import(text_color));
  CopyTextRectangle(canvas, x, y, s.width, s.height, transparent, s);
}

void
Canvas::DrawClippedText(int x, int y, const PixelRect &rc, const TCHAR *text)
{
  // TODO: implement full clipping
  if (rc.right > x)
    DrawClippedText(x, y, rc.right - x, text);
}

void
Canvas::DrawClippedText(int x, int y, unsigned width, const TCHAR *text)
{
  assert(text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  if (width > s.width)
    width = s.width;

  SDLRasterCanvas canvas(buffer);
  CopyTextRectangle(canvas, x, y, width, s.height, s,
                    text_color, background_color,
                    background_mode == OPAQUE);

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
Canvas::CopyTransparentWhite(int dest_x, int dest_y,
                             unsigned dest_width, unsigned dest_height,
                             const Canvas &src, int src_x, int src_y)
{
  if (!Clip(dest_x, dest_width, GetWidth(), src_x) ||
      !Clip(dest_y, dest_height, GetHeight(), src_y))
    return;

  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<ActivePixelTraits> operations(canvas.Import(COLOR_WHITE));
  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.buffer.At(src_x, src_y), src.buffer.pitch,
                       operations);
}

void
Canvas::StretchTransparentWhite(int dest_x, int dest_y,
                                unsigned dest_width, unsigned dest_height,
                                ConstImageBuffer src, int src_x, int src_y,
                                unsigned src_width, unsigned src_height)
{
  if (!Clip(dest_x, dest_width, GetWidth(), src_x) ||
      !Clip(dest_y, dest_height, GetHeight(), src_y))
    return;

  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<ActivePixelTraits> operations(canvas.Import(COLOR_WHITE));
  canvas.ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                        src.At(src_x, src_y), src.pitch, src.width, src.height,
                        operations);
}

void
Canvas::StretchNot(const Bitmap &_src)
{
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();
  const unsigned src_x = 0, src_y = 0;
  const unsigned dest_x = 0, dest_y = 0;
  const unsigned dest_width = GetWidth();
  const unsigned dest_height = GetHeight();

  SDLRasterCanvas canvas(buffer);
  BitNotPixelOperations<ActivePixelTraits> operations;

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

  OpaqueTextPixelOperations<ActivePixelTraits, GreyscalePixelTraits>
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
                       BitNotPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyOr(int dest_x, int dest_y,
               unsigned dest_width, unsigned dest_height,
               ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitOrPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyNotOr(int dest_x, int dest_y,
                  unsigned dest_width, unsigned dest_height,
                  ConstImageBuffer src, int src_x, int src_y)
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       BitNotOrPixelOperations<ActivePixelTraits>());
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
                       BitAndPixelOperations<ActivePixelTraits>());
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

  AlphaPixelOperations<ActivePixelTraits> operations(alpha);

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       operations);
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

void
Canvas::AlphaBlendNotWhite(int dest_x, int dest_y,
                           unsigned dest_width, unsigned dest_height,
                           ConstImageBuffer src,
                           int src_x, int src_y,
                           unsigned src_width, unsigned src_height,
                           uint8_t alpha)
{
  // TODO: support scaling

  SDLRasterCanvas canvas(buffer);

  NotWhiteCondition<ActivePixelTraits> c;
  NotWhiteAlphaPixelOperations<ActivePixelTraits> operations(c,
                                                             PortableAlphaPixelOperations<ActivePixelTraits>(alpha));

  canvas.CopyRectangle(dest_x, dest_y, dest_width, dest_height,
                       src.At(src_x, src_y), src.pitch,
                       operations);
}

void
Canvas::AlphaBlendNotWhite(int dest_x, int dest_y,
                           unsigned dest_width, unsigned dest_height,
                           const Canvas src,
                           int src_x, int src_y,
                           unsigned src_width, unsigned src_height,
                           uint8_t alpha)
{
  AlphaBlendNotWhite(dest_x, dest_y, dest_width, dest_height,
                     src.buffer,
                     src_x, src_y, src_width, src_height,
                     alpha);
}
