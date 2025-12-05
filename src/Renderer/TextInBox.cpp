// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextInBox.hpp"
#include "LabelBlock.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
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

void
RenderShadowedText(Canvas &canvas, const TCHAR *text, PixelPoint p,
                   Color text_color, Color outline_color) noexcept
{
  canvas.SetBackgroundTransparent();

  // Calculate offset based on current font size (font must be selected before calling)
  // Use font height which is already DPI-aware
  // For smaller fonts, use a relatively thicker outline for better visibility
  // For larger fonts, use a proportionally thinner outline to avoid dominating the text
  const unsigned font_height = canvas.GetFontHeight();
  const int offset = font_height <= 16
    ? std::max(1, int(font_height) / 20)  // Thicker for small fonts (≤16px)
    : std::max(1, int(font_height) / 32); // Thinner for larger fonts
  if (offset > 0) {
    // Draw outline in all 8 directions to avoid gaps
    // Note: Font is not changed - uses whatever font is currently selected on canvas
    canvas.SetTextColor(outline_color);
    canvas.DrawText({p.x + offset, p.y + offset}, text);  // SE
    canvas.DrawText({p.x - offset, p.y + offset}, text);  // SW
    canvas.DrawText({p.x + offset, p.y - offset}, text);  // NE
    canvas.DrawText({p.x - offset, p.y - offset}, text);  // NW
    canvas.DrawText({p.x + offset, p.y}, text);          // E
    canvas.DrawText({p.x - offset, p.y}, text);          // W
    canvas.DrawText({p.x, p.y + offset}, text);          // S
    canvas.DrawText({p.x, p.y - offset}, text);          // N
  }

  // Draw main text at exact position with exact font size (same as without outline)
  canvas.SetTextColor(text_color);
  canvas.DrawText(p, text);
}

void
RenderShadowedText(Canvas &canvas, const TCHAR *text, PixelPoint p,
                   bool inverted) noexcept
{
  RenderShadowedText(canvas, text, p,
                     inverted ? COLOR_WHITE : COLOR_BLACK,
                     inverted ? COLOR_BLACK : COLOR_WHITE);
}

// returns true if really wrote something
bool
TextInBox(Canvas &canvas, const TCHAR *text, PixelPoint p, TextInBoxMode mode,
          const PixelRect &map_rc, LabelBlock *label_block) noexcept
{
  // landable waypoint label inside white box

  PixelSize tsize = canvas.CalcTextSize(text);

  if (mode.align == TextInBoxMode::Alignment::RIGHT) p.x -= tsize.width;
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
  rc.top = p.y;
  rc.bottom = p.y + tsize.height + 1;

  if (mode.move_in_view) {
    auto offset = TextInBoxMoveInView(rc, map_rc);
    p.x += offset.x;
    p.y += offset.y;
  }

  if (label_block != nullptr && !label_block->check(rc)) return false;

  if (mode.shape == LabelShape::ROUNDED_BLACK ||
      mode.shape == LabelShape::ROUNDED_WHITE) {
    if (mode.shape == LabelShape::ROUNDED_BLACK) canvas.SelectBlackPen();
    else canvas.SelectWhitePen();

    {
#ifdef ENABLE_OPENGL
      const ScopeAlphaBlend alpha_blend;
      canvas.Select(Brush(COLOR_WHITE.WithAlpha(0xa0)));
#else
      canvas.SelectWhiteBrush();
#endif

      canvas.DrawRoundRectangle(rc, PixelSize{Layout::VptScale(8)});
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
TextInBox(Canvas &canvas, const TCHAR *text, PixelPoint p, TextInBoxMode mode,
          PixelSize screen_size, LabelBlock *label_block) noexcept
{
  return TextInBox(canvas, text, p, mode, PixelRect{screen_size}, label_block);
}
