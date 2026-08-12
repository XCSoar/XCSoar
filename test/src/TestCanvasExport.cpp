// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/memory/Buffer.hpp"
#include "ui/canvas/memory/Dither.hpp"
#include "ui/canvas/memory/Export.hpp"
#include "TestUtil.hpp"

#include <array>
#include <cstdint>

static constexpr PixelSize SIZE{3, 2};

int
main()
{
  plan_tests(18);

  std::array<Luminosity8, 6> pixels{
    Luminosity8{0}, Luminosity8{255}, Luminosity8{0},
    Luminosity8{255}, Luminosity8{0}, Luminosity8{255},
  };
  const ConstImageBuffer<GreyscalePixelTraits> source{
    pixels.data(), SIZE.width, SIZE,
  };
  Dither dither;

  std::array<uint8_t, 8> y8;
  y8.fill(0x7f);
  CopyFromGreyscale(dither, true, y8.data(), 4, 1, source);
  ok1(y8[0] == 0);
  ok1(y8[1] == 255);
  ok1(y8[2] == 0);
  ok1(y8[3] == 0x7f);
  ok1(y8[4] == 255);
  ok1(y8[5] == 0);
  ok1(y8[6] == 255);
  ok1(y8[7] == 0x7f);

  std::array<uint16_t, 8> rgb565;
  rgb565.fill(0x1234);
  CopyFromGreyscale(dither, true, rgb565.data(), 8, 2, source);
  ok1(rgb565[0] == 0);
  ok1(rgb565[1] == 0xffff);
  ok1(rgb565[2] == 0);
  ok1(rgb565[3] == 0x1234);
  ok1(rgb565[4] == 0xffff);

  std::array<uint32_t, 8> rgb32;
  rgb32.fill(0x12345678);
  CopyFromGreyscale(dither, true, rgb32.data(), 16, 4, source);
  ok1(rgb32[0] == 0);
  ok1(rgb32[1] == 0xffffffff);
  ok1(rgb32[2] == 0);
  ok1(rgb32[3] == 0x12345678);
  ok1(rgb32[4] == 0xffffffff);

  return exit_status();
}
