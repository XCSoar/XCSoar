// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextInBox.hpp"
#include "LabelBlock.hpp"
#include "Asset.hpp"
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

/** Alias raster (e-paper panels). */
static bool
LabelUsesAliasRaster() noexcept
{
  return HasEPaper() || IsDithered();
}

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

  /* OUTLINED_INVERTED uses a black halo for LCD contrast; alias panels
     turn that fringe into extra ink. */
  if (LabelUsesAliasRaster())
    inverted = false;

  canvas.SetBackgroundTransparent();

  canvas.SetTextColor(inverted ? COLOR_BLACK : COLOR_WHITE);

  /* at least 1px, or tiny fonts get no halo at all */
  const unsigned halo = LabelUsesAliasRaster()
    ? std::max(1u, canvas.GetFontHeight() / 16u)
    : std::max(1u, canvas.GetFontHeight() / 12u);
  DrawTextHalo(canvas, text, p, halo);

  canvas.SetTextColor(inverted ? COLOR_WHITE : COLOR_BLACK);
  canvas.DrawText(p, text);
}

/**
 * Fill opacity of a rounded label, shared with the NOTAM labels.
 */
static constexpr uint8_t LABEL_FILL_ALPHA = 0xa0;

/**
 * Corner diameter shared with the NOTAM labels. Capped so a short
 * label stays a rounded rectangle.
 */
static unsigned
RoundedLabelCorner(unsigned height) noexcept
{
  return std::min(Layout::VptScale(8),
                  std::max(2u, height / 2));
}

void
DrawRoundedLabel(Canvas &canvas, const PixelRect &rc, uint8_t opacity,
         Color fill, bool solid) noexcept
{
  const unsigned corner = RoundedLabelCorner(rc.GetHeight());
  const PixelSize ellipse{corner};

  canvas.SelectNullPen();

#ifdef ENABLE_OPENGL
  /* Android may choose an EGL config without a stencil buffer */
  GLint stencil_bits = 0;
  glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
  const bool shadow = !IsSlowCPU() && stencil_bits > 0;

  if (shadow) {
    /* keep the shadow out from under the fill */
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

    BoxShadowStyle style = BoxShadowStyle::FLOATING;
    for (auto &layer : style.layers)
      layer.alpha = layer.alpha * opacity / 0xff;
    DrawBoxShadow(rc, style, corner / 2);
  } else
    canvas.Select(Pen(1, COLOR_BLACK.WithAlpha(opacity)));

  const ScopeAlphaBlend alpha_blend;
  const uint8_t alpha = solid
    ? opacity
    : uint8_t(LABEL_FILL_ALPHA * opacity / 0xff);
  canvas.Select(Brush(fill.WithAlpha(alpha)));
#else
  (void)opacity;
  (void)solid;
  canvas.SelectBlackPen();
  canvas.Select(Brush(fill));
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
  const unsigned side_padding = padding;

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
      canvas.Select(Brush(COLOR_WHITE.WithAlpha(LABEL_FILL_ALPHA)));
#else
      canvas.SelectWhiteBrush();
#endif

      /* DrawRoundRectangle takes an ellipse diameter (radius =
         diameter/2). */
      const unsigned ellipse = RoundedLabelCorner(rc.GetHeight());
      canvas.DrawRoundRectangle(rc, PixelSize{ellipse});
    }

    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(p, text);
  } else if (mode.shape == LabelShape::ROUNDED) {
    DrawRoundedLabel(canvas, rc);

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
