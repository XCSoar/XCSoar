// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TeamCode/TeamCode.hpp"
#include "TestUtil.hpp"
#include "Math/Util.hpp"
#include "util/StringUtil.hpp"

int main()
{
  plan_tests(12 + 29);

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

  ok1(tc.Update("AB12"));
  const auto bearing = tc.GetBearing();
  const auto range = tc.GetRange();
  ok1(tc.Update("ab12"));
  ok1(StringIsEqual(tc.GetCode(), "AB12"));
  ok1(tc.GetBearing() == bearing);
  ok1(tc.GetRange() == range);

  ok1(tc.Update("r01"));
  ok1(StringIsEqual(tc.GetCode(), "R01"));
  ok1(tc.Update("i0668"));
  ok1(StringIsEqual(tc.GetCode(), "I0668"));

  for (const char *invalid : {"A", "AB", "AB1234", "AB!2", "AB 2",
                              "ABé", "AB\n2", "AB１２"}) {
    ok1(!tc.Update(invalid));
    ok1(StringIsEqual(tc.GetCode(), "I0668"));
  }

  ok1(tc.Update(""));
  ok1(!tc.IsDefined());
  ok1(!tc.Update(nullptr));
  ok1(!tc.IsDefined());

  return exit_status();
}
