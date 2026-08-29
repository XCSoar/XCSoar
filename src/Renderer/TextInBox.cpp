// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextInBox.hpp"
#include "LabelBlock.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "Math/Angle.hpp"
#include "Screen/Layout.hpp"
#include "util/UTF8.hpp"

#include <algorithm>

#include <math.h>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Triangulate.hpp"
#endif

static PixelPoint
TextInBoxMoveInView(PixelRect &rc, const PixelRect &map_rc) noexcept
{
  PixelPoint offset(0, 0);

  // If label is above maprect
  if (map_rc.top > rc.top) {
    // Move label down into maprect
    unsigned d = map_rc.top - rc.top;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is right of maprect
  if (map_rc.right < rc.right) {
    unsigned d = map_rc.right - rc.right;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  // If label is below maprect
  if (map_rc.bottom < rc.bottom) {
    unsigned d = map_rc.bottom - rc.bottom;
    rc.top += d;
    rc.bottom += d;
    offset.y += d;
  }

  // If label is left of maprect
  if (map_rc.left > rc.left) {
    unsigned d = map_rc.left - rc.left;
    rc.right += d;
    rc.left += d;
    offset.x += d;
  }

  return offset;
}

/**
 * Stamp the text along a circle of the given radius, one stamp per
 * ~1.5px of circumference so consecutive stamps always overlap.  The
 * four diagonal copies this used to be only close up while the offset
 * is 1px; beyond that they leave gaps along every stroke, which is
 * what high-DPI screens hit (offset 5 on a 3x iPhone).
 */
static void
DrawTextHalo(Canvas &canvas, const char *text, const PixelPoint p,
             const unsigned offset) noexcept
{
  /* 8 is the full neighbourhood of a 1px halo, 16 caps the cost */
  const unsigned n = std::clamp(4 * offset, 8u, 16u);

  for (unsigned i = 0; i < n; ++i) {
    const auto [sin, cos] =
      (Angle::FullCircle() * ((double)i / n)).SinCos();
    canvas.DrawText({p.x + (int)lround(cos * offset),
                     p.y + (int)lround(sin * offset)},
                    text);
  }
}

void
RenderShadowedText(Canvas &canvas, const char *text,
                   PixelPoint p,
                   Color text_color, Color outline_color) noexcept
{
  if (text == nullptr || text[0] == '\0')
    return;

  canvas.SetBackgroundTransparent();

  canvas.SetTextColor(outline_color);

  /* at least 1px, or tiny fonts get no halo at all */
  DrawTextHalo(canvas, text, p, std::max(1u, canvas.GetFontHeight() / 12u));

  canvas.SetTextColor(text_color);
  canvas.DrawText(p, text);
}

void
RenderShadowedText(Canvas &canvas, const char *text,
                   PixelPoint p,
                   bool inverted) noexcept
{
  RenderShadowedText(canvas, text, p,
                     inverted ? COLOR_WHITE : COLOR_BLACK,
                     inverted ? COLOR_BLACK : COLOR_WHITE);
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const char *text, PixelPoint p,
          TextInBoxMode mode, const PixelRect &map_rc,
          LabelBlock *label_block) noexcept
{
  // landable waypoint label inside white box

  if (text == nullptr || text[0] == '\0' || !ValidateUTF8(text))
    text = "?";

  PixelSize tsize = canvas.CalcTextSize(text);

  if (mode.align == TextInBoxMode::Alignment::RIGHT)
    p.x -= tsize.width;
  else if (mode.align == TextInBoxMode::Alignment::CENTER)
    p.x -= tsize.width / 2;

  if (mode.vertical_position == TextInBoxMode::VerticalPosition::ABOVE)
    p.y -= tsize.height;
  else if (mode.vertical_position == TextInBoxMode::VerticalPosition::CENTERED)
    p.y -= tsize.height / 2;

  const unsigned padding = Layout::GetTextPadding();
  PixelRect rc;
  rc.left = p.x - padding - 1;
  rc.right = p.x + tsize.width + padding;
  rc.top = p.y - (int)padding;
  rc.bottom = p.y + tsize.height + padding;

  if (mode.move_in_view) {
    auto offset = TextInBoxMoveInView(rc, map_rc);
    p.x += offset.x;
    p.y += offset.y;
  }

  if (label_block != nullptr && !label_block->check(rc))
    return false;

  if (mode.shape == LabelShape::ROUNDED_BLACK ||
      mode.shape == LabelShape::ROUNDED_WHITE) {
    /* A hairline pen breaks up along the rounded corners where the
       outline is emitted as a triangle strip: its half width (0.5px)
       rounds to zero on most of the arc segments, leaving a dotted
       edge.  Widen the pen only there.  Where GL_LINE_LOOP draws the
       outline (and on the non-OpenGL canvases), a DPI-scaled pen would
       merely make the box fat and - because LineToTriangles() rounds
       the segment offsets to whole pixels - ragged around the corners;
       on a 3x iPhone it turns the 1px hairline into 3px. */
    unsigned outline_width = 1;
#ifdef ENABLE_OPENGL
    if (!UseOpenGLLineLoopOutline(outline_width))
      outline_width = std::max(2u, Layout::ScaleFinePenWidth(1));
#endif

    const Pen outline_pen{outline_width,
                          mode.shape == LabelShape::ROUNDED_BLACK
                          ? COLOR_BLACK : COLOR_WHITE};
    canvas.Select(outline_pen);

    {
#ifdef ENABLE_OPENGL
      const ScopeAlphaBlend alpha_blend;
      canvas.Select(Brush(COLOR_WHITE.WithAlpha(0xa0)));
#else
      canvas.SelectWhiteBrush();
#endif

      /* DrawRoundRectangle takes an ellipse diameter (radius =
         diameter/2). Cap it so short labels stay rounded rectangles
         instead of pills. */
      const unsigned ellipse =
        std::min(Layout::VptScale(8),
                 std::max(2u, (unsigned)rc.GetHeight() / 2));
      canvas.DrawRoundRectangle(rc, PixelSize{ellipse});
    }

    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(p, text);
  } else if (mode.shape == LabelShape::FILLED) {
    canvas.SetBackgroundColor(COLOR_WHITE);
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawOpaqueText(p, rc, text);
  } else if (mode.shape == LabelShape::OUTLINED) {
    RenderShadowedText(canvas, text, p, false);
  } else if (mode.shape == LabelShape::OUTLINED_INVERTED) {
    RenderShadowedText(canvas, text, p, true);
  } else {
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(p, text);
  }

  return true;
}

bool
TextInBox(Canvas &canvas, const char *text, PixelPoint p,
          TextInBoxMode mode,
          PixelSize screen_size,
          LabelBlock *label_block) noexcept
{
  return TextInBox(canvas, text, p, mode, PixelRect{screen_size}, label_block);
}
