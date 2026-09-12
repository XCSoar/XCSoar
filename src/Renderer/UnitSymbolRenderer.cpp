// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnitSymbolRenderer.hpp"
#include "TextInBox.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "util/Macros.hpp"

#include <algorithm>

#include <cstdio>

struct UnitSymbolStrings {
  const char *line1;
  const char *line2;
  bool is_fraction;
};

static constexpr UnitSymbolStrings symbol_strings[] = {
  { nullptr, nullptr },
  { nullptr, "km", false },
  { nullptr, "NM", false },
  { nullptr, "mi", false },
  { "km", "h", true },
  { nullptr, "kt", false },
  { "mp", "h", false },
  { "m", "s", true },
  { "ft", "min", true },
  { nullptr, "m", false },
  { nullptr, "ft", false },
  { nullptr, "FL", false },
  { nullptr, "K", false },
  { DEG, "C", false },
  { DEG, "F", false },
  { "h", "Pa", false },
  { nullptr, "mb", false },
  { "mm", "Hg", false },
  { "in", "Hg", false },
  { "kg", "m²", true },
  { "lb", "ft²", true },
  { nullptr, "kg", false },
  { nullptr, "lb", false },
  { "%", " ", false },
  { nullptr, ":1", false },
  { nullptr, "V", false },
  { nullptr, "Hz", false },
  { nullptr, "rpm", false },
};

static_assert(ARRAY_SIZE(symbol_strings) == (size_t)Unit::COUNT,
              "number of unit symbols does not match number of units");

PixelSize
UnitSymbolRenderer::GetSize(const Font &font, const Unit unit) noexcept
{
  assert((size_t)unit < ARRAY_SIZE(symbol_strings));

  const UnitSymbolStrings &strings = symbol_strings[(unsigned)unit];

  if (!strings.line1 && !strings.line2)
    return {0, 0};

  assert(strings.line2 != nullptr);

  if (!strings.line1)
    return font.TextSize(strings.line2);

  PixelSize size1 = font.TextSize(strings.line1);
  PixelSize size2 = font.TextSize(strings.line2);

  return {std::max(size1.width, size2.width), size1.height + size2.height};
}

PixelSize
UnitSymbolRenderer::GetSize(const Canvas &canvas, const Unit unit) noexcept
{
  assert((size_t)unit < ARRAY_SIZE(symbol_strings));

  const UnitSymbolStrings &strings = symbol_strings[(unsigned)unit];

  if (!strings.line1 && !strings.line2)
    return {0, 0};

  assert(strings.line2 != nullptr);

  if (!strings.line1)
    return canvas.CalcTextSize(strings.line2);

  PixelSize size1 = canvas.CalcTextSize(strings.line1);
  PixelSize size2 = canvas.CalcTextSize(strings.line2);

  return {std::max(size1.width, size2.width), size1.height + size2.height};
}

unsigned
UnitSymbolRenderer::GetAscentHeight(const Font &font, const Unit unit) noexcept
{
  assert((size_t)unit < ARRAY_SIZE(symbol_strings));

  const UnitSymbolStrings &strings = symbol_strings[(unsigned)unit];

  if (!strings.line1 && !strings.line2)
    return 0;

  assert(strings.line2 != nullptr);

  if (!strings.line1)
    return font.GetAscentHeight();

  return font.GetAscentHeight() + font.GetHeight();
}

/**
 * Draw one line of the symbol, with a halo when one was asked for.
 */
static void
DrawSymbolLine(Canvas &canvas, const PixelPoint p, const char *text,
               const Color *halo) noexcept
{
  if (halo == nullptr)
    canvas.DrawText(p, text);
  else
    RenderShadowedText(canvas, text, p, halo[0], halo[1]);
}

static void
DrawSymbol(Canvas &canvas, const PixelPoint pos,
           const Unit unit,
           const Pen &unit_fraction_pen,
           const Color *halo) noexcept
{
  assert((size_t)unit < ARRAY_SIZE(symbol_strings));

  const UnitSymbolStrings &strings = symbol_strings[(unsigned)unit];

  if (!strings.line1 && !strings.line2)
    return;

  assert(strings.line2 != nullptr);

  if (!strings.line1) {
    DrawSymbolLine(canvas, pos, strings.line2, halo);
    return;
  }

  PixelSize size1 = canvas.CalcTextSize(strings.line1);
  PixelSize size2 = canvas.CalcTextSize(strings.line2);

  if (size1.width > size2.width) {
    if (strings.is_fraction) {
      canvas.Select(unit_fraction_pen);
      canvas.DrawLine(pos.At(0, size1.height),
                      pos.At(size1.width, size1.height));
    }

    DrawSymbolLine(canvas, pos, strings.line1, halo);
    DrawSymbolLine(canvas, pos.At((size1.width - size2.width) / 2,
                                  size1.height),
                   strings.line2, halo);
  } else {
    if (strings.is_fraction) {
      canvas.Select(unit_fraction_pen);
      canvas.DrawLine(pos.At(0, size1.height),
                      pos.At(size2.width, size1.height));
    }

    DrawSymbolLine(canvas, pos.At((size2.width - size1.width) / 2, 0),
                   strings.line1, halo);
    DrawSymbolLine(canvas, pos.At(0, size1.height), strings.line2, halo);
  }
}

void
UnitSymbolRenderer::Draw(Canvas &canvas, const PixelPoint pos,
                         const Unit unit,
                         const Pen &unit_fraction_pen) noexcept
{
  DrawSymbol(canvas, pos, unit, unit_fraction_pen, nullptr);
}

void
UnitSymbolRenderer::Draw(Canvas &canvas, const PixelPoint pos,
                         const Unit unit,
                         const Pen &unit_fraction_pen,
                         const Color text_color,
                         const Color outline_color) noexcept
{
  const Color halo[]{text_color, outline_color};
  DrawSymbol(canvas, pos, unit, unit_fraction_pen, halo);
}
