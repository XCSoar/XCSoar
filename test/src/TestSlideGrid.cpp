// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Terrain/HeightMatrix.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <cstdint>

static bool
Same(const uint16_t *got, const uint16_t *expect, unsigned n) noexcept
{
  return std::equal(expect, expect + n, got);
}

int
main()
{
  plan_tests(5);

  uint16_t packed[12];
  for (unsigned i = 0; i < 12; ++i)
    packed[i] = uint16_t(i);
  SlideGrid(packed, 4, 4, 3, sizeof(uint16_t), 1, 0);
  const uint16_t east[] = {
    1, 2, 3, 3,
    5, 6, 7, 7,
    9, 10, 11, 11,
  };
  ok1(Same(packed, east, 12));

  for (unsigned i = 0; i < 12; ++i)
    packed[i] = uint16_t(i);
  SlideGrid(packed, 4, 4, 3, sizeof(uint16_t), 0, 1);
  const uint16_t north[] = {
    0, 1, 2, 3,
    0, 1, 2, 3,
    4, 5, 6, 7,
  };
  ok1(Same(packed, north, 12));

  for (unsigned i = 0; i < 12; ++i)
    packed[i] = uint16_t(i);
  SlideGrid(packed, 4, 4, 3, sizeof(uint16_t), -1, 0);
  const uint16_t west[] = {
    0, 0, 1, 2,
    4, 4, 5, 6,
    8, 8, 9, 10,
  };
  ok1(Same(packed, west, 12));

  for (unsigned i = 0; i < 12; ++i)
    packed[i] = uint16_t(i);
  SlideGrid(packed, 4, 4, 3, sizeof(uint16_t), 0, -1);
  const uint16_t south[] = {
    4, 5, 6, 7,
    8, 9, 10, 11,
    8, 9, 10, 11,
  };
  ok1(Same(packed, south, 12));

  /* Wider pitch than the used samples.  East and north together
     must leave the vacated corner and the padding alone. */
  uint16_t wide[18];
  for (unsigned y = 0; y < 3; ++y)
    for (unsigned x = 0; x < 6; ++x)
      wide[y * 6 + x] = x < 4 ? uint16_t(y * 4 + x) : 0xeeee;
  SlideGrid(wide, 6, 4, 3, sizeof(uint16_t), 1, 1);
  const uint16_t both[] = {
    0, 1, 2, 3, 0xeeee, 0xeeee,
    1, 2, 3, 3, 0xeeee, 0xeeee,
    5, 6, 7, 7, 0xeeee, 0xeeee,
  };
  ok1(Same(wide, both, 18));

  return exit_status();
}
