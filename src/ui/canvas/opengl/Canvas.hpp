// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Color.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "ui/opengl/System.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"

#include <string_view>

#include <tchar.h>

/* Workaround: Some Win32 headers define OPAQUE and TRANSPARENT as preprocessor
 * defines. Undefine them to avoid name conflict. */
#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif


class Angle;
class Bitmap;
class GLTexture;
template<class T> class AllocatedArray;

/**
 * Base drawable canvas class
 * 
 */
class Canvas {
  friend class SubCanvas;
  friend class BufferCanvas;

protected:
  PixelPoint offset = {0, 0};
  PixelSize size = {0, 0};

  Pen pen;
  Brush brush;
  const Font *font = nullptr;
  Color text_color, background_color;
  enum {
    OPAQUE, TRANSPARENT
  } background_mode = OPAQUE;

  /**
   * static buffer to store vertices of wide lines.
   */
  static AllocatedArray<BulkPixelPoint> vertex_buffer;

public:
  Canvas() noexcept = default;
  constexpr Canvas(PixelSize _size) noexcept:size(_size) {}

  Canvas(const Canvas &other) = delete;
  Canvas &operator=(const Canvas &other) = delete;

  constexpr void Create(PixelSize _size) noexcept {
    size = _size;
  }

protected:
  /**
   * Returns true if the outline should be drawn after the area has
   * been filled.  As an optimization, this function returns false if
   * brush and pen share the same color.
   */
  bool IsPenOverBrush() const noexcept {
    return pen.IsDefined() &&
      (brush.IsHollow() || brush.GetColor() != pen.GetColor());
  }

public:
  bool IsDefined() const noexcept {
    return true;
  }

  PixelSize GetSize() const noexcept {
    return size;
  }

  unsigned GetWidth() const noexcept {
    return size.width;
  }

  unsigned GetHeight() const noexcept {
    return size.height;
  }

  [[gnu::pure]]
  PixelRect GetRect() const noexcept {
    return PixelRect(size);
  }

  void SelectNullPen() noexcept {
    pen = Pen(0, COLOR_BLACK);
  }

  void SelectWhitePen() noexcept {
    pen = Pen(1, COLOR_WHITE);
  }

  void SelectWhitePen(unsigned width) noexcept {
    pen = Pen(width, COLOR_WHITE);
  }

  void SelectBlackPen() noexcept {
    pen = Pen(1, COLOR_BLACK);
  }

  void SelectBlackPen(unsigned width) noexcept {
    pen = Pen(width, COLOR_BLACK);
  }

  void SelectHollowBrush() noexcept {
    brush.Destroy();
  }

  void SelectWhiteBrush() noexcept {
    brush = Brush(COLOR_WHITE);
  }

  void SelectBlackBrush() noexcept {
    brush = Brush(COLOR_BLACK);
  }

  void Select(const Pen &_pen) noexcept {
    pen = _pen;
  }

  void Select(const Brush &_brush) noexcept {
    brush = _brush;
  }

  void Select(const Font &_font) noexcept {
    font = &_font;
  }

  void SetTextColor(const Color c) noexcept {
    text_color = c;
  }

  Color GetTextColor() const noexcept {
    return text_color;
  }

  void SetBackgroundColor(const Color c) noexcept {
    background_color = c;
  }

  Color GetBackgroundColor() const noexcept {
    return background_color;
  }

  void SetBackgroundOpaque() noexcept {
    background_mode = OPAQUE;
  }

  void SetBackgroundTransparent() noexcept {
    background_mode = TRANSPARENT;
  }

  void InvertRectangle(PixelRect r) noexcept;

  void DrawRectangle(PixelRect r) noexcept {
    DrawFilledRectangle(r, brush);

    if (IsPenOverBrush())
      DrawOutlineRectangle(r);
  }

  void DrawFilledRectangle(PixelRect r, const Color color) noexcept;

  void DrawFilledRectangle(PixelRect r, const Brush &brush) noexcept {
    if (!brush.IsHollow())
      DrawFilledRectangle(r, brush.GetColor());
  }

  /**
   * Draw a rectangle outline with the current OpenGL color and
   * settings.
   */
  void DrawOutlineRectangleGL(PixelRect r) noexcept;

  void DrawOutlineRectangle(PixelRect r) noexcept;
  void DrawOutlineRectangle(PixelRect r, Color color) noexcept;

  /**
   * Fade to white.  This enables GL_BLEND and disables it before
   * returning.
   *
   * @param alpha the alpha value, 0=no change, 0xff=fully white.
   */
  void FadeToWhite(PixelRect rc, GLubyte alpha) noexcept;
  void FadeToWhite(GLubyte alpha) noexcept;

  void Clear() noexcept {
    DrawRectangle(PixelRect{GetSize()});
  }

  void Clear(const Color color) noexcept {
    DrawFilledRectangle(PixelRect{GetSize()}, color);
  }

  void Clear(const Brush &brush) noexcept {
    DrawFilledRectangle(PixelRect{GetSize()}, brush);
  }

  void ClearWhite() noexcept {
    Clear(COLOR_WHITE);
  }

  void DrawRoundRectangle(PixelRect r, PixelSize ellipse_size) noexcept;

  void DrawRaisedEdge(PixelRect &rc) noexcept;

  void DrawPolyline(const BulkPixelPoint *points, unsigned num_points) noexcept;

  void DrawPolygon(const BulkPixelPoint *points, unsigned num_points) noexcept;

  /**
   * Draw a triangle fan (GL_TRIANGLE_FAN).  The first point is the
   * origin of the fan.
   */
  void DrawTriangleFan(const BulkPixelPoint *points, unsigned num_points) noexcept;

  /**
   * Draw a solid thin horizontal line.
   */
  void DrawHLine(int x1, int x2, int y, Color color) noexcept;

  void DrawLine(PixelPoint a, PixelPoint b) noexcept;

  /**
   * Similar to DrawLine(), but force exact pixel coordinates.  This
   * may be more expensive on some platforms, and works only for thin
   * lines.
   */
  void DrawExactLine(PixelPoint a, PixelPoint b) noexcept;

  void DrawLinePiece(const PixelPoint a, const PixelPoint b) noexcept;

  void DrawTwoLines(PixelPoint a, PixelPoint b, PixelPoint c) noexcept;

  /**
   * @see DrawTwoLines(), DrawExactLine()
   */
  void DrawTwoLinesExact(PixelPoint a, PixelPoint b, PixelPoint c) noexcept;

  void DrawCircle(PixelPoint center, unsigned radius) noexcept;

  void DrawSegment(PixelPoint center, unsigned radius,
                   Angle start, Angle end, bool horizon=false) noexcept;

  void DrawAnnulus(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end) noexcept;

  void DrawKeyhole(PixelPoint center, unsigned small_radius,
                   unsigned big_radius,
                   Angle start, Angle end) noexcept;

  void DrawArc(PixelPoint center, unsigned radius,
               Angle start, Angle end) noexcept;

  void DrawFocusRectangle(PixelRect rc) noexcept;

  [[gnu::pure]]
  const PixelSize CalcTextSize(std::string_view text) const noexcept;

  [[gnu::pure]]
  unsigned CalcTextWidth(std::string_view text) const noexcept {
    return CalcTextSize(text).width;
  }

  [[gnu::pure]]
  unsigned GetFontHeight() const noexcept {
    return font != nullptr ? font->GetHeight() : 0;
  }

  void DrawText(PixelPoint p, std::string_view text) noexcept;

  void DrawTransparentText(PixelPoint p, std::string_view text) noexcept;

  void DrawOpaqueText(PixelPoint p, const PixelRect &rc,
                      std::string_view text) noexcept;

  void DrawClippedText(PixelPoint p, const PixelRect &rc,
                       std::string_view text) noexcept {
    // XXX

    if (p.x < rc.right)
      DrawClippedText(p, rc.right - p.x, text);
  }

  void DrawClippedText(PixelPoint p, PixelSize size,
                       std::string_view text) noexcept;

  void DrawClippedText(PixelPoint p, unsigned width,
                       std::string_view text) noexcept {
    DrawClippedText(p, {width, 16384u}, text);
  }

  /**
   * Render text, clip it within the bounds of this Canvas.
   */
  void TextAutoClipped(PixelPoint p, std::string_view t) noexcept {
    if (p.x < (int)GetWidth() && p.y < (int)GetHeight())
      DrawClippedText(p, {GetWidth() - p.x, GetHeight() - p.y}, t);
  }

  /**
   * Render multi-line text.
   *
   * @return the resulting text height
   */
  unsigned DrawFormattedText(PixelRect r, std::string_view text,
                             unsigned format) noexcept;

  /**
   * Draws a texture.  The caller is responsible for binding it and
   * enabling GL_TEXTURE_2D.
   */
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const GLTexture &texture,
               PixelPoint src_position, PixelSize src_size) noexcept;

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const GLTexture &texture) noexcept;

  void Copy(PixelPoint dest_position, PixelSize dest_size,
            const Bitmap &src, PixelPoint src_position) noexcept;
  void Copy(const Bitmap &src) noexcept;

  void StretchNot(const Bitmap &src) noexcept;

  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src,
               PixelPoint src_position, PixelSize src_size) noexcept;
  void Stretch(PixelPoint dest_position, PixelSize dest_size,
               const Bitmap &src) noexcept;

  void Stretch(const Bitmap &src) noexcept {
    Stretch({0,0}, size, src);
  }

  /**
   * Stretches a monochrome bitmap (1 bit per pixel), painting the
   * black pixels in the specified foreground color.  The white pixels
   * will be either transparent or drawn in the specified background
   * color, whichever operation is faster on the Canvas.
   *
   * @param fg_color draw this color instead of "black"
   * @param bg_color draw this color instead of "white"
   */
  void StretchMono(PixelPoint dest_position, PixelSize dest_size,
                   const Bitmap &src,
                   PixelPoint src_position, PixelSize src_size,
                   Color fg_color, Color bg_color) noexcept;

  /**
   * Copy pixels from this object to a texture.  The texture must be
   * initialised already.  Note that the texture will be flipped
   * vertically. So the texture must be created with flipped=true.
   */
  void CopyToTexture(GLTexture &texture, PixelRect src_rc) const noexcept;
};
