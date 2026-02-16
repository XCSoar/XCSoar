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
  { nullptr, _T("km"), false },
  { nullptr, _T("NM"), false },
  { nullptr, _T("mi"), false },
  { _T("km"), _T("h"), true },
  { nullptr, _T("kt"), false },
  { _T("mp"), _T("h"), false },
  { _T("m"), _T("s"), true },
  { _T("ft"), _T("min"), true },
  { nullptr, _T("m"), false },
  { nullptr, _T("ft"), false },
  { nullptr, _T("FL"), false },
  { nullptr, _T("K"), false },
  { _T(DEG), _T("C"), false },
  { _T(DEG), _T("F"), false },
  { _T("h"), _T("Pa"), false },
  { nullptr, _T("mb"), false },
  { _T("mm"), _T("Hg"), false },
  { _T("in"), _T("Hg"), false },
  { _T("kg"), _T("m²"), true },
  { _T("lb"), _T("ft²"), true },
  { nullptr, _T("kg"), false },
  { nullptr, _T("lb"), false },
  { _T("%"), _T(" "), false },
  { nullptr, _T(":1"), false },
  { nullptr, _T("V"), false },
  { nullptr, _T("Hz"), false },
  { nullptr, _T("rpm"), false },
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
