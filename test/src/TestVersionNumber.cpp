// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/VersionNumber.hxx"
#include "util/StaticString.hxx"
#include "TestUtil.hpp"

int main()
{
  plan_tests(8);

  ok1(VersionNumber("1.2.3") == VersionNumber(1, 2, 3));
  ok1(VersionNumber("1.2") == VersionNumber(1, 2, 0));
  ok1(VersionNumber("1.2") != VersionNumber(1, 9));
  ok1(VersionNumber("1.2") <  VersionNumber(1, 9));
  ok1(VersionNumber("1.2") >= VersionNumber(1, 1, 1));

  const VersionNumber v(1, 2, 3);
  ok1(v.toString() == "1.2.3");
  ok1(v.toString() != "1.2");
  ok1(v.toString(false) == "1.2");

  return exit_status();
}
