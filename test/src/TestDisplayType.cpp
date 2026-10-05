// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayType.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(6);

  ok1(!IsEPaperDisplayType(DisplayType::LCD));
  ok1(IsEPaperDisplayType(DisplayType::E_INK));
  ok1(IsEPaperDisplayType(DisplayType::COLOR_E_INK));

  ok1(!DisplayTypeUsesMonochromeFonts(DisplayType::LCD));
  ok1(DisplayTypeUsesMonochromeFonts(DisplayType::E_INK));
  ok1(DisplayTypeUsesMonochromeFonts(DisplayType::COLOR_E_INK));

  return exit_status();
}
