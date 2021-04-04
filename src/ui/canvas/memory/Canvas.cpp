/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Util.hpp"
#include "Optimised.hpp"
#include "RasterCanvas.hpp"
#include "ui/canvas/custom/Cache.hpp"
#include "Math/Angle.hpp"

#ifdef __ARM_NEON__
#include "NEON.hpp"
#endif

#ifndef NDEBUG
#include "util/UTF8.hpp"
#endif

#ifdef UNICODE
#include "util/ConvertString.hpp"
#endif

#include <algorithm>
#include <cassert>
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
Canvas::DrawOutlineRectangle(PixelRect r, Color color) noexcept
{
  SDLRasterCanvas canvas(buffer);
  canvas.DrawRectangle(r.left, r.top, r.right, r.bottom,
                       canvas.Import(color));
}

void
Canvas::DrawFilledRectangle(PixelRect r, Color color) noexcept
{
  if (r.IsEmpty())
    return;

  SDLRasterCanvas canvas(buffer);
  canvas.FillRectangle(r.left, r.top, r.right, r.bottom,
                       canvas.Import(color));
}

void
Canvas::InvertRectangle(PixelRect r)
{
  if (r.IsEmpty())
    return;

  CopyNot(r.GetTopLeft(), r.GetSize(), buffer, r.GetTopLeft());
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
Canvas::DrawLine(PixelPoint a, PixelPoint b) noexcept
{
  const unsigned thickness = pen.GetWidth();
  const unsigned mask = pen.GetMask();

  SDLRasterCanvas canvas(buffer);
  const auto color = canvas.Import(pen.GetColor());
  unsigned mask_position = 0;
  if (thickness > 1)
    canvas.DrawThickLine(a.x, a.y, b.x, b.y, thickness, color,
                         mask, mask_position);
  else
    canvas.DrawLine(a.x, a.y, b.x, b.y, color, mask);
}

void
Canvas::DrawCircle(PixelPoint center, unsigned radius) noexcept
{
  SDLRasterCanvas canvas(buffer);

  if (!brush.IsHollow()) {
    const auto color = canvas.Import(brush.GetColor());

    if (brush.GetColor().IsOpaque())
      canvas.FillCircle(center.x, center.y, radius, color);
    else
      canvas.FillCircle(center.x, center.y, radius, color,
                        AlphaPixelOperations<ActivePixelTraits>(brush.GetColor().Alpha()));
  }

  if (IsPenOverBrush()) {
    if (pen.GetWidth() < 2) {
      canvas.DrawCircle(center.x, center.y, radius,
                        canvas.Import(pen.GetColor()));
      return;
    }

    // no thickCircleColor in SDL_gfx, so need to emulate it with multiple draws (slow!)
    for (int i= (pen.GetWidth()/2); i>= -(int)(pen.GetWidth()-1)/2; --i) {
      canvas.DrawCircle(center.x, center.y, radius + i,
                        canvas.Import(pen.GetColor()));
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
Canvas::CalcTextSize(BasicStringView<TCHAR> text) const noexcept
{
  assert(text != nullptr);
#ifdef UNICODE
  const WideToUTF8Converter text2(text);
#else
  const StringView text2 = text;
  assert(ValidateUTF8(text));
#endif

  PixelSize size = { 0, 0 };

  if (font == nullptr)
    return size;

  /* see if the TextCache can handle this request */
  size = TextCache::LookupSize(*font, text2);
  if (size.height > 0)
    return size;

  return TextCache::GetSize(*font, text2);
}

static TextCache::Result
RenderText(const Font *font, BasicStringView<TCHAR> text) noexcept
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
     typename SourcePixelTraits::const_pointer(s.data),
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
Canvas::DrawText(PixelPoint p, BasicStringView<TCHAR> text) noexcept
{
  assert(text != nullptr);
#ifndef UNICODE
  assert(ValidateUTF8(text));
#endif

  auto s = RenderText(font, text);
  if (s.data == nullptr)
    return;

  SDLRasterCanvas canvas(buffer);
  CopyTextRectangle(canvas, p.x, p.y, s.width, s.height, s,
                    text_color, background_color,
                    background_mode == OPAQUE);
}

void
Canvas::DrawTransparentText(PixelPoint p, BasicStringView<TCHAR> text) noexcept
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
  CopyTextRectangle(canvas, p.x, p.y, s.width, s.height, transparent, s);
}

void
Canvas::DrawClippedText(PixelPoint p, const PixelRect &rc,
                        BasicStringView<TCHAR> text) noexcept
{
  // TODO: implement full clipping
  if (rc.right > p.x)
    DrawClippedText(p, rc.right - p.x, text);
}

void
Canvas::DrawClippedText(PixelPoint p, unsigned width,
                        BasicStringView<TCHAR> text) noexcept
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
  CopyTextRectangle(canvas, p.x, p.y, width, s.height, s,
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
Canvas::Copy(PixelPoint dest_position, PixelSize dest_size,
             ConstImageBuffer src, PixelPoint src_position) noexcept
{
  if (!Clip(dest_position.x, dest_size.width, GetWidth(), src_position.x) ||
      !Clip(dest_position.y, dest_size.height, GetHeight(), src_position.y))
    return;

  SDLRasterCanvas canvas(buffer);
  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch);
}

void
Canvas::Copy(const Canvas &src, PixelPoint src_position) noexcept
{
  Copy({0, 0}, src.GetSize(), src, src_position);
}

void
Canvas::Copy(const Canvas &src)
{
  Copy(src, {0, 0});
}

void
Canvas::Copy(PixelPoint dest_position, PixelSize dest_size,
             const Bitmap &src, PixelPoint src_position) noexcept
{
  Copy(dest_position, dest_size, src.GetNative(), src_position);
}

void
Canvas::Copy(const Bitmap &_src)
{
  ConstImageBuffer src = _src.GetNative();

  Copy({0, 0}, {src.width, src.height}, src, {0, 0});
}

void
Canvas::CopyTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                             const Canvas &src,
                             PixelPoint src_position) noexcept
{
  if (!Clip(dest_position.x, dest_size.width, GetWidth(), src_position.x) ||
      !Clip(dest_position.y, dest_size.height, GetHeight(), src_position.y))
    return;

  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<ActivePixelTraits> operations(canvas.Import(COLOR_WHITE));
  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.buffer.At(src_position.x, src_position.y),
                       src.buffer.pitch,
                       operations);
}

void
Canvas::StretchTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                                ConstImageBuffer src, PixelPoint src_position,
                                PixelSize src_size) noexcept
{
  if (!Clip(dest_position.x, dest_size.width, GetWidth(), src_position.x) ||
      !Clip(dest_position.y, dest_size.height, GetHeight(), src_position.y))
    return;

  SDLRasterCanvas canvas(buffer);
  TransparentPixelOperations<ActivePixelTraits> operations(canvas.Import(COLOR_WHITE));
  canvas.ScaleRectangle(dest_position, dest_size,
                        src.At(src_position.x, src_position.y),
                        src.pitch, src_size,
                        operations);
}

void
Canvas::StretchNot(const Bitmap &_src)
{
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();
  const unsigned src_x = 0, src_y = 0;
  const unsigned dest_x = 0, dest_y = 0;
  const auto dest_size = GetSize();

  SDLRasterCanvas canvas(buffer);
  BitNotPixelOperations<ActivePixelTraits> operations;

  canvas.ScaleRectangle({dest_x, dest_y}, dest_size,
                        src.At(src_x, src_y),
                        src.pitch, {src.width, src.height},
                        operations);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                ConstImageBuffer src,
                PixelPoint src_position, PixelSize src_size) noexcept
{
  if (dest_size == src_size) {
    /* fast path: no zooming needed */
    Copy(dest_position, dest_size, src, src_position);
    return;
  }

  SDLRasterCanvas canvas(buffer);

  canvas.ScaleRectangle(dest_position, dest_size,
                        src.At(src_position.x, src_position.y),
                        src.pitch, src_size);
}

void
Canvas::Stretch(const Canvas &src,
                PixelPoint src_position, PixelSize src_size) noexcept
{
  // XXX
  Stretch({0, 0}, GetSize(),
          src, src_position, src_size);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src,
                PixelPoint src_position, PixelSize src_size) noexcept
{
  assert(IsDefined());
  assert(src.IsDefined());

  Stretch(dest_position, dest_size,
          src.GetNative(),
          src_position, src_size);
}

void
Canvas::Stretch(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &_src)
{
  assert(IsDefined());
  assert(_src.IsDefined());

  ConstImageBuffer src = _src.GetNative();
  Stretch(dest_position, dest_size,
          src, {0, 0}, {src.width, src.height});
}

void
Canvas::StretchMono(PixelPoint dest_position, PixelSize dest_size,
                    ::ConstImageBuffer<GreyscalePixelTraits> src,
                    PixelPoint src_position, PixelSize src_size,
                    Color fg_color, Color bg_color)
{
  assert(IsDefined());
  assert(dest_size.width < 0x4000);
  assert(dest_size.height < 0x4000);

  if (dest_size.width >= 0x4000 || dest_size.height >= 0x4000)
    /* paranoid sanity check; shouldn't ever happen */
    return;

  SDLRasterCanvas canvas(buffer);

  OpaqueTextPixelOperations<ActivePixelTraits, GreyscalePixelTraits>
    opaque(canvas.Import(fg_color), canvas.Import(bg_color));

  canvas.ScaleRectangle<decltype(opaque), GreyscalePixelTraits>
    (dest_position, dest_size,
     src.At(src_position.x, src_position.y), src.pitch, src_size,
     opaque);
}

void
Canvas::CopyNot(PixelPoint dest_position, PixelSize dest_size,
                ConstImageBuffer src, PixelPoint src_position) noexcept
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       BitNotPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyOr(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src, PixelPoint src_position) noexcept
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       BitOrPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                  ConstImageBuffer src, PixelPoint src_position) noexcept
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       BitNotOrPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                  const Bitmap &src, PixelPoint src_position) noexcept
{
  assert(src.IsDefined());

  CopyNotOr(dest_position, dest_size, src.GetNative(), src_position);
}

void
Canvas::CopyAnd(PixelPoint dest_position, PixelSize dest_size,
                ConstImageBuffer src, PixelPoint src_position) noexcept
{
  SDLRasterCanvas canvas(buffer);

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       BitAndPixelOperations<ActivePixelTraits>());
}

void
Canvas::CopyNot(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src, PixelPoint src_position) noexcept
{
  assert(src.IsDefined());

  CopyNot(dest_position, dest_size, src.GetNative(), src_position);
}

void
Canvas::CopyOr(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept
{
  assert(src.IsDefined());

  CopyOr(dest_position, dest_size, src.GetNative(), src_position);
}

void
Canvas::CopyAnd(PixelPoint dest_position, PixelSize dest_size,
                const Bitmap &src, PixelPoint src_position) noexcept
{
  assert(src.IsDefined());

  CopyAnd(dest_position, dest_size, src.GetNative(), src_position);
}

void
Canvas::CopyAnd(const Bitmap &src)
{
  CopyAnd({0, 0}, GetSize(), src.GetNative(), {0, 0});
}

void
Canvas::DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept
{
  unsigned radius = std::min(ellipse_size.width, ellipse_size.height) / 2u;
  ::RoundRect(*this, r, radius);
}

void
Canvas::AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                   ConstImageBuffer src,
                   PixelPoint src_position, PixelSize src_size,
                   uint8_t alpha)
{
  // TODO: support scaling

  SDLRasterCanvas canvas(buffer);

  AlphaPixelOperations<ActivePixelTraits> operations(alpha);

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       operations);
}

void
Canvas::AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                   const Canvas &src,
                   PixelPoint src_position, PixelSize src_size,
                   uint8_t alpha)
{
  AlphaBlend(dest_position, dest_size,
             src.buffer, src_position, src_size,
             alpha);
}

void
Canvas::AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                           ConstImageBuffer src,
                           PixelPoint src_position, PixelSize src_size,
                           uint8_t alpha)
{
  // TODO: support scaling

  SDLRasterCanvas canvas(buffer);

  NotWhiteCondition<ActivePixelTraits> c;
  NotWhiteAlphaPixelOperations<ActivePixelTraits> operations(c,
                                                             PortableAlphaPixelOperations<ActivePixelTraits>(alpha));

  canvas.CopyRectangle(dest_position.x, dest_position.y,
                       dest_size.width, dest_size.height,
                       src.At(src_position.x, src_position.y), src.pitch,
                       operations);
}

void
Canvas::AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                           const Canvas src,
                           PixelPoint src_position, PixelSize src_size,
                           uint8_t alpha)
{
  AlphaBlendNotWhite(dest_position, dest_size,
                     src.buffer, src_position, src_size,
                     alpha);
}
