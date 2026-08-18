// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/memory/Buffer.hpp"
#include "ui/canvas/memory/Export.hpp"
#include "TestUtil.hpp"

#include <array>
#include <cstdint>

static constexpr PixelSize SIZE{3, 2};

int
main()
{
  plan_tests(15);

  std::array<BGRA8Color, 6> pixels{
    BGRA8Color{255, 0, 0}, BGRA8Color{0, 255, 0}, BGRA8Color{0, 0, 255},
    BGRA8Color{0, 0, 0}, BGRA8Color{255, 255, 255}, BGRA8Color{127, 127, 127},
  };
  const ConstImageBuffer<BGRAPixelTraits> source{
    pixels.data(), SIZE.width * sizeof(BGRA8Color), SIZE,
  };

  std::array<uint8_t, 8> y8;
  y8.fill(0xee);
  CopyFromBGRA(y8.data(), 4, 1, source);
  ok1(y8[0] == Luminosity8{RGB8Color{pixels[0]}}.GetLuminosity());
  ok1(y8[1] == Luminosity8{RGB8Color{pixels[1]}}.GetLuminosity());
  ok1(y8[2] == Luminosity8{RGB8Color{pixels[2]}}.GetLuminosity());
  ok1(y8[3] == 0xee);
  ok1(y8[4] == Luminosity8{RGB8Color{pixels[3]}}.GetLuminosity());

  std::array<RGB565Color, 8> rgb565;
  rgb565.fill(RGB565Color{1, 2, 3});
  CopyFromBGRA(rgb565.data(), 8, 2, source);
  ok1(rgb565[0].GetNativeValue() == ToRGB565(pixels[0]).GetNativeValue());
  ok1(rgb565[1].GetNativeValue() == ToRGB565(pixels[1]).GetNativeValue());
  ok1(rgb565[2].GetNativeValue() == ToRGB565(pixels[2]).GetNativeValue());
  ok1(rgb565[3].GetNativeValue() == RGB565Color(1, 2, 3).GetNativeValue());
  ok1(rgb565[4].GetNativeValue() == ToRGB565(pixels[3]).GetNativeValue());

  std::array<BGRA8Color, 8> bgra;
  bgra.fill(BGRA8Color{1, 2, 3});
  CopyFromBGRA(bgra.data(), 16, 4, source);
  ok1(bgra[0] == pixels[0]);
  ok1(bgra[1] == pixels[1]);
  ok1(bgra[2] == pixels[2]);
  ok1(bgra[3] == BGRA8Color(1, 2, 3));
  ok1(bgra[4] == pixels[3]);

  return exit_status();
}
