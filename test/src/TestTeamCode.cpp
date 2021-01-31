/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TeamCode/TeamCode.hpp"
#include "TestUtil.hpp"
#include "Math/Util.hpp"
#include "util/StringUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(12);

  TeamCode tc;

  tc.Update(Angle::Degrees(90), 5000);

  ok1(StringIsEqual(tc.GetCode(), _T("901E")));
  ok1(iround(tc.GetBearing().Degrees()) == 90);
  ok1(equals(tc.GetRange(), 5000));

  tc.Update(Angle::Degrees(359), 25000);
  ok1(StringIsEqual(tc.GetCode(), _T("ZW6Y")));
  ok1(iround(tc.GetBearing().Degrees()) == 359);
  ok1(equals(tc.GetRange(), 25000));

  tc.Update(Angle::Degrees(180), 800000);
  ok1(StringIsEqual(tc.GetCode(), _T("I0668")));
  ok1(iround(tc.GetBearing().Degrees()) == 180);
  ok1(equals(tc.GetRange(), 800000));

  tc.Update(Angle::Degrees(270), 100);
  ok1(StringIsEqual(tc.GetCode(), _T("R01")));
  ok1(iround(tc.GetBearing().Degrees()) == 270);
  ok1(equals(tc.GetRange(), 100));

  return exit_status();
}
