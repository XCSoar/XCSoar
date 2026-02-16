// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnitSymbolRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/Macros.hpp"

#include <algorithm>

#include <tchar.h>
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

void
UnitSymbolRenderer::Draw(Canvas &canvas, const PixelPoint pos,
                         const Unit unit,
                         const Pen &unit_fraction_pen) noexcept
{
  assert((size_t)unit < ARRAY_SIZE(symbol_strings));

  const UnitSymbolStrings &strings = symbol_strings[(unsigned)unit];

  if (!strings.line1 && !strings.line2)
    return;

  assert(strings.line2 != nullptr);

  if (!strings.line1) {
    canvas.DrawText(pos, strings.line2);
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

    canvas.DrawText(pos, strings.line1);
    canvas.DrawText(pos.At((size1.width - size2.width) / 2,
                           size1.height),
                    strings.line2);
  } else {
    if (strings.is_fraction) {
      canvas.Select(unit_fraction_pen);
      canvas.DrawLine(pos.At(0, size1.height),
                      pos.At(size2.width, size1.height));
    }

    canvas.DrawText(pos.At((size2.width - size1.width) / 2, 0), strings.line1);
    canvas.DrawText(pos.At(0, size1.height), strings.line2);
  }
}
