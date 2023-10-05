// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/CRC16CCITT.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

int main()
{
  plan_tests(2);

  // XModem
  ok1(UpdateCRC16CCITT("123456789", 9, 0) == 0x31C3);

  // CCITT
  ok1(UpdateCRC16CCITT("123456789", 9, 0xFFFF) == 0x29B1);

  return exit_status();
}
