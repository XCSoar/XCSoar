// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TeamCode/TeamCode.hpp"
#include "TestUtil.hpp"
#include "Math/Util.hpp"
#include "util/StringUtil.hpp"

int main()
{
  plan_tests(12);

  TeamCode tc;

  tc.Update(Angle::Degrees(90), 5000);

  ok1(StringIsEqual(tc.GetCode(), "901E"));
  ok1(iround(tc.GetBearing().Degrees()) == 90);
  ok1(equals(tc.GetRange(), 5000));

  tc.Update(Angle::Degrees(359), 25000);
  ok1(StringIsEqual(tc.GetCode(), "ZW6Y"));
  ok1(iround(tc.GetBearing().Degrees()) == 359);
  ok1(equals(tc.GetRange(), 25000));

  tc.Update(Angle::Degrees(180), 800000);
  ok1(StringIsEqual(tc.GetCode(), "I0668"));
  ok1(iround(tc.GetBearing().Degrees()) == 180);
  ok1(equals(tc.GetRange(), 800000));

  tc.Update(Angle::Degrees(270), 100);
  ok1(StringIsEqual(tc.GetCode(), "R01"));
  ok1(iround(tc.GetBearing().Degrees()) == 270);
  ok1(equals(tc.GetRange(), 100));

  return exit_status();
}
