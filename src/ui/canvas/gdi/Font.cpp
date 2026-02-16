// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Font.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Screen/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "Asset.hpp"

#include <cassert>
#include <stdexcept>

void
Font::Load(const FontDescription &d)
{
  assert(IsScreenInitialized());

  Destroy();

  font = ::CreateFontIndirect(&(const LOGFONT &)d);
  if (font == nullptr)
    throw std::runtime_error{"CreateFontIndirect() failed"};

  if (GetObjectType(font) != OBJ_FONT) {
    Destroy();
    throw std::runtime_error{"CreateFontIndirect() did not return a font"};
  }

  CalculateHeights();
}

PixelSize
Font::TextSize(std::string_view text) const noexcept
{
  AnyCanvas canvas;
  canvas.Select(*this);
  return canvas.CalcTextSize(text);
}

void
Font::CalculateHeights() noexcept
{
  AnyCanvas canvas;
  canvas.Select(*this);

  TEXTMETRIC tm;
  ::GetTextMetrics(canvas, &tm);

  height = tm.tmHeight;
  ascent_height = tm.tmAscent;
  capital_height = tm.tmAscent - 1 - tm.tmHeight / 10;
}

void
Font::Destroy() noexcept
{
  if (font != nullptr) {
    assert(IsScreenInitialized());

#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(font);
    assert(success);

    font = nullptr;
  }
}
