// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WarningRibbonRenderer.hpp"
#include "Asset.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

#include <algorithm>

struct Ribbon {
  unsigned pad_x, pad_y;
  PixelSize outer;
};

/** Cloth around the letters.  Vertical is tight; the ends are longer. */
static Ribbon
Measure(const Font &font, const char *text) noexcept
{
  Ribbon ribbon{0, 0, {0, 0}};
  if (text == nullptr || *text == '\0' || !font.IsDefined())
    return ribbon;

  const PixelSize text_size = font.TextSize(text);
  if (text_size.height == 0)
    return ribbon;

  ribbon.pad_y = std::max(1u, text_size.height / 8);
  ribbon.pad_x = std::max(ribbon.pad_y, text_size.height / 3);
  ribbon.outer = {text_size.width + ribbon.pad_x * 2,
                  text_size.height + ribbon.pad_y * 2};
  return ribbon;
}

PixelSize
WarningRibbonRenderer::Size(const Font &font, const char *text) noexcept
{
  return Measure(font, text).outer;
}

void
WarningRibbonRenderer::Draw(Canvas &canvas, PixelPoint top_left,
                            const Font &font, const char *text) noexcept
{
  canvas.Select(font);
  const Ribbon ribbon = Measure(font, text);
  if (ribbon.outer.height == 0)
    return;

  const PixelRect banner{top_left.x, top_left.y,
                         top_left.x + int(ribbon.outer.width),
                         top_left.y + int(ribbon.outer.height)};
  canvas.DrawFilledRectangle(banner,
                             HasColors() ? COLOR_RED : COLOR_BLACK);
  canvas.SetTextColor(COLOR_WHITE);
  canvas.SetBackgroundTransparent();
  canvas.DrawText({top_left.x + int(ribbon.pad_x),
                   top_left.y + int(ribbon.pad_y)},
                  text);
}
