// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/OverwritingRingBuffer.hpp"
#include "FlightInfo.hpp"

struct PixelRect;
class Canvas;
class Font;

class FlightListRenderer {
  const Font &font, &header_font;

  OverwritingRingBuffer<FlightInfo, 128u> flights;

public:
  FlightListRenderer(const Font &_font, const Font &_header_font)
    :font(_font), header_font(_header_font) {}

  void AddFlight(const FlightInfo &flight);

  void Draw(Canvas &canvas, PixelRect rc);
};
