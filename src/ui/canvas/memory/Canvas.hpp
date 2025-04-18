// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"
#include "Color.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "PixelTraits.hpp"
#include "Buffer.hpp"
#include "ActivePixelTraits.hpp"
#include "util/tstring_view.hxx"

#include <tchar.h>

#ifdef _WIN32
/* those are WIN32 macros - undefine, or Canvas::background_mode will
   break */
#undef OPAQUE
#undef TRANSPARENT
#endif

class Angle;
class Bitmap;

/**
 * Base drawable canvas class
 */
class Canvas {
  friend class WindowCanvas;
  friend class SubCanvas;

  using ConstImageBuffer = ::ConstImageBuffer<ActivePixelTraits>;

protected:
  WritableImageBuffer<ActivePixelTraits> buffer;

  Pen pen;
  Brush brush;
  const Font *font = nullptr;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode = OPAQUE;

public:
  Canvas()
    :buffer(WritableImageBuffer<ActivePixelTraits>::Empty()) {}

  explicit Canvas(WritableImageBuffer<ActivePixelTraits> _buffer)
    :buffer(_buffer) {}

  void Create(WritableImageBuffer<ActivePixelTraits> _buffer) {
    buffer = _buffer;
  }

protected:
  /**
   * Returns true if the outline should be drawn after the area has
   * been filled.  As an optimization, this function returns false if
   * brush and pen share the same color.
   */
  bool IsPenOverBrush() const {
    return pen.IsDefined() &&
      (brush.IsHollow() || brush.GetColor() != pen.GetColor());
  }

public:
  bool IsDefined() const {
    return buffer.data != nullptr;
  }

  const PixelSize &GetSize() const noexcept {
    return buffer.size;
  }

  unsigned GetWidth() const {
    return buffer.size.width;
  }

  unsigned GetHeight() const {
    return buffer.size.height;
  }

  [[gnu::pure]]
  PixelRect GetRect() const {
    return PixelRect(GetSize());
  }

  void SelectNullPen() {
    pen = Pen(0, COLOR_BLACK);
  }

  void SelectWhitePen() {
    pen = Pen(1, COLOR_WHITE);
  }

  void SelectWhitePen(unsigned width) {
    pen = Pen(width, COLOR_WHITE);
  }

  void SelectBlackPen() {
    pen = Pen(1, COLOR_BLACK);
  }

  void SelectBlackPen(unsigned width) {
    pen = Pen(width, COLOR_BLACK);
  }

  void SelectHollowBrush() {
    brush.Destroy();
  }

  void SelectWhiteBrush() {
    brush = Brush(COLOR_WHITE);
  }

  void SelectBlackBrush() {
    brush = Brush(COLOR_BLACK);
  }

  void Select(const Pen &_pen) {
    pen = _pen;
  }

  void Select(const Brush &_brush) {
    brush = _brush;
  }

  void Select(const Font &_font) {
    font = &_font;
  }

  void SetTextColor(const Color c) {
    text_color = c;
  }

  Color GetTextColor() const {
    return text_color;
  }

  void SetBackgroundColor(const Color c) {
    background_color = c;
  }

  Color GetBackgroundColor() const {
    return background_color;
  }

  void SetBackgroundOpaque() {
    background_mode = OPAQUE;
  }

  void SetBackgroundTransparent() {
    background_mode = TRANSPARENT;
  }

  void DrawOutlineRectangle(PixelRect r, Color color) noexcept;

  void DrawRectangle(PixelRect r) noexcept {
    DrawFilledRectangle(r, brush);

    if (IsPenOverBrush())
      DrawOutlineRectangle(r, pen.GetColor());
  }

  void DrawFilledRectangle(PixelRect r, Color color) noexcept;

  void DrawFilledRectangle(PixelRect r, const Brush &brush) noexcept {
    if (brush.IsHollow())
      return;

    DrawFilledRectangle(r, brush.GetColor());
  }

  void InvertRectangle(PixelRect r);

  void Clear() {
    DrawRectangle(GetRect());
  }

  void Clear(const Color color) {
    DrawFilledRectangle(GetRect(), color);
  }

  void Clear(const Brush &brush) {
    DrawFilledRectangle(GetRect(), brush);
  }

  void ClearWhite() {
    Clear(COLOR_WHITE);
  }

  void DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept;

  void DrawRaisedEdge(PixelRect &rc) noexcept;

  void DrawPolyline(const BulkPixelPoint *points, unsigned num_points);
  void DrawPolygon(const BulkPixelPoint *points, unsigned num_points);

  void DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points) {
    DrawPolygon(points, num_points);
  }

  /**
   * Draw a solid thin horizontal line.
   */
  void DrawHLine(int x1, int x2, int y, Color color);

  void DrawLine(PixelPoint a, PixelPoint b) noexcept;

  void DrawExactLine(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawLinePiece(const PixelPoint a, const PixelPoint b) {
    DrawLine(a, b);
  }

  void DrawTwoLines(PixelPoint a, PixelPoint b, PixelPoint c) noexcept {
    DrawLine(a, b);
    DrawLine(b, c);
  }

  void DrawTwoLinesExact(PixelPoint a, PixelPoint b, PixelPoint c) noexcept {
    DrawTwoLines(a, b, c);
  }

  void DrawCircle(PixelPoint center, unsigned radius) noexcept;

  void DrawSegment(PixelPoint center, unsigned radius,
                   Angle start, Angle end, bool horizon=false);

  void DrawAnnulus(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end);

  void DrawKeyhole(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end);

  void DrawArc(PixelPoint center, unsigned radius,
               Angle start, Angle end);

  void DrawFocusRectangle(const PixelRect &rc) {
    DrawOutlineRectangle(rc, COLOR_DARK_GRAY);
  }

  [[gnu::pure]]
  const PixelSize CalcTextSize(tstring_view text) const noexcept;

  [[gnu::pure]]
  unsigned CalcTextWidth(tstring_view text) const noexcept {
    return CalcTextSize(text).width;
  }

  [[gnu::pure]]
  unsigned GetFontHeight() const {
    return font != nullptr ? font->GetHeight() : 0;
  }

  void DrawText(PixelPoint p, tstring_view text) noexcept;

  void DrawTransparentText(PixelPoint p, tstring_view text) noexcept;

  void DrawOpaqueText(PixelPoint p, const PixelRect &rc,
                      tstring_view text) noexcept;

  void DrawClippedText(PixelPoint p, const PixelRect &rc,
                       tstring_view text) noexcept;
  void DrawClippedText(PixelPoint p, unsigned width,
                       tstring_view text) noexcept;

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelPoint p, tstring_view t) noexcept {
    DrawText(p, t);
  }

  /**
   * Render multi-line text.
   *
   * @return the resulting text height
   */
  unsigned DrawFormattedText(PixelRect r, tstring_view text,
                             unsigned format) noexcept;

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            ConstImageBuffer src, PixelPoint src_position) noexcept;

  void Copy(PixelPoint dest_position, ConstImageBuffer src) noexcept {
    Copy(dest_position, src.size, src, {0, 0});
  }

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Canvas &src, PixelPoint src_position) noexcept {
    Copy(dest_position, dest_size, src.buffer, src_position);
  }

  void Copy(const Canvas &src, PixelPoint src_position) noexcept;
  void Copy(const Canvas &src);

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Bitmap &src, PixelPoint src_position) noexcept;
  void Copy(const Bitmap &src);

  void CopyTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                            const Canvas &src,
                            PixelPoint src_position) noexcept;

  void StretchTransparentWhite(PixelPoint dest_position, PixelSize dest_size,
                               ConstImageBuffer src, PixelPoint src_position,
                               PixelSize src_size) noexcept;

  void StretchNot(const Bitmap &src);

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(ConstImageBuffer src) {
    Stretch({0, 0}, GetSize(),
            src, {0, 0}, src.size);
  }

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src,
               PixelPoint src_position, PixelSize src_size) noexcept {
    Stretch(dest_position, dest_size,
            src.buffer, src_position, src_size);
  }

  void Stretch(const Canvas &src,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src,
               PixelPoint src_position, PixelSize src_size) noexcept;
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src);

  void Stretch(const Bitmap &src) {
    Stretch({0, 0}, buffer.size, src);
  }

  void StretchMono(PixelPoint dest_position, PixelSize dest_size,
                   ::ConstImageBuffer<GreyscalePixelTraits> src,
                   PixelPoint src_position, PixelSize src_size,
                   Color fg_color, Color bg_color);

  void CopyNot(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyNot(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Canvas &src, PixelPoint src_position) noexcept {
    CopyOr(dest_position, dest_size, src.buffer, src_position);
  }

  void CopyOr(PixelPoint dest_position, PixelSize dest_size,
              const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyOr(const Bitmap &src) {
    CopyOr({0, 0}, GetSize(), src, {0, 0});
  }

  void CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                 ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyNotOr(PixelPoint dest_position, PixelSize dest_size,
                 const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               ConstImageBuffer src, PixelPoint src_position) noexcept;

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Canvas &src, PixelPoint src_position) noexcept {
    CopyAnd(dest_position, dest_size, src.buffer, src_position);
  }

  void CopyAnd(const Canvas &src) {
    CopyAnd({0, 0}, src.GetSize(), src.buffer, {0, 0});
  }

  void CopyAnd(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src, PixelPoint src_position) noexcept;

  void CopyAnd(const Bitmap &src);

  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  ConstImageBuffer src,
                  PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha);

  void AlphaBlend(PixelPoint dest_position, PixelSize dest_size,
                  const Canvas &src,
                  PixelPoint src_position, PixelSize src_size,
                  uint8_t alpha);

  /**
   * Like AlphaBlend(), but skip source pixels that are white.
   */
  void AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                          ConstImageBuffer src,
                          PixelPoint src_position, PixelSize src_size,
                          uint8_t alpha);

  void AlphaBlendNotWhite(PixelPoint dest_position, PixelSize dest_size,
                          const Canvas src,
                          PixelPoint src_position, PixelSize src_size,
                          uint8_t alpha);
};
