// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/PackedFloat.hxx"
#include "TestUtil.hpp"

int main()
{
  plan_tests(2);

  float test_float = 123.45e33;
  uint8_t expected_bytes_le[] = { 0x70, 0x34, 0xbe, 0x79 };
  uint8_t expected_bytes_be[] = { 0x79, 0xbe, 0x34, 0x70};
  PackedFloatLE pfle = test_float;
  PackedFloatBE pfbe = test_float;
  ok1(memcmp(&pfle, &expected_bytes_le, 4) == 0);
  ok1(memcmp(&pfbe, &expected_bytes_be, 4) == 0);
}
