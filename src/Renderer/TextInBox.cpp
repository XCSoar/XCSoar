// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextInBox.hpp"
#include "LabelBlock.hpp"
#include "BoxShadowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "Math/Angle.hpp"
#include "Screen/Layout.hpp"
#include "util/UTF8.hpp"

#include <algorithm>

#include <math.h>

#ifdef ENABLE_OPENGL
#include "Hardware/CPU.hpp"
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
                   bool inverted) noexcept
{
  if (text == nullptr || text[0] == '\0')
    return;

  canvas.SetBackgroundTransparent();

  canvas.SetTextColor(inverted ? COLOR_BLACK : COLOR_WHITE);

  /* at least 1px, or tiny fonts get no halo at all */
  DrawTextHalo(canvas, text, p, std::max(1u, canvas.GetFontHeight() / 12u));

  canvas.SetTextColor(inverted ? COLOR_WHITE : COLOR_BLACK);
  canvas.DrawText(p, text);
}

/**
 * The opacity of a #LabelShape::PILL.
 */
static constexpr uint8_t PILL_ALPHA = 0xf2;

/**
 * Draw the box of a #LabelShape::PILL and its shadow.
 */
static void
DrawPill(Canvas &canvas, const PixelRect &rc) noexcept
{
  /* a pill: the diameter of the corners is the box's height */
  const PixelSize ellipse{unsigned(rc.GetHeight())};

  canvas.SelectNullPen();

#ifdef ENABLE_OPENGL
  /* Android may choose an EGL config without a stencil buffer */
  GLint stencil_bits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);

  if (!IsSlowCPU() && stencil_bits > 0) {
    /* the pill is translucent, so keep its shadow out from under it:
       mark the pill's area in the stencil buffer, and draw the shadow
       only around it */
    const GLEnable<GL_STENCIL_TEST> stencil_test;
    glClear(GL_STENCIL_BUFFER_BIT);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    canvas.SelectWhiteBrush();
    canvas.DrawRoundRectangle(rc, ellipse);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    DrawBoxShadow(rc, BoxShadowStyle::FLOATING, rc.GetHeight() / 2);
  } else
    /* no shadow on a slow CPU or without a stencil buffer: a black
       outline sets the pill off the map */
    canvas.SelectBlackPen();

  const ScopeAlphaBlend alpha_blend;
  canvas.Select(Brush(COLOR_WHITE.WithAlpha(PILL_ALPHA)));
#else
  /* no shadow without OpenGL: a black outline sets the pill off the
     map */
  canvas.SelectBlackPen();
  canvas.SelectWhiteBrush();
#endif

  canvas.DrawRoundRectangle(rc, ellipse);
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

  /* the round ends of a pill need room of their own */
  const unsigned side_padding = mode.shape == LabelShape::PILL
    ? (tsize.height + 2 * padding) / 2
    : padding;

  PixelRect rc;
  rc.left = p.x - side_padding - 1;
  rc.right = p.x + tsize.width + side_padding;
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
  } else if (mode.shape == LabelShape::PILL) {
    DrawPill(canvas, rc);

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
