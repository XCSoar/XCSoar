/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "TestUtil.hpp"

#include "GlideSolvers/GlidePolar.hpp"
#include "Units/Units.hpp"

#include <cstdio>

class GlidePolarTest
{
  GlidePolar polar;

public:
  void Run();

private:
  void Init();
  void TestBasic();
  void TestMC();
};

void
GlidePolarTest::Init()
{
  // Polar 1 from PolarStore (206 Hornet)
  polar.ideal_polar_a = fixed(0.0022032);
  polar.ideal_polar_b = fixed(-0.08784);
  polar.ideal_polar_c = fixed(1.47);

  polar.reference_mass = fixed(318);
  polar.ballast_ratio = fixed(100) / polar.reference_mass;

  polar.wing_area = fixed(9.8);

  // No ballast and no bugs on the wings
  polar.ballast = fixed_zero;
  polar.bugs = fixed_one;

  // MC zero
  polar.mc = fixed_zero;

  polar.Vmax = Units::ToSysUnit(fixed(200), unKiloMeterPerHour);
}

void
GlidePolarTest::TestBasic()
{
  polar.update();

  ok1(equals(polar.polar_a, polar.ideal_polar_a));
  ok1(equals(polar.polar_b, polar.ideal_polar_b));
  ok1(equals(polar.polar_c, polar.ideal_polar_c));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(fixed(80), unKiloMeterPerHour)), 0.606));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(fixed(120), unKiloMeterPerHour)), 0.99));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(fixed(160), unKiloMeterPerHour)), 1.918));

  ok1(equals(polar.get_Smax(), polar.SinkRate(polar.get_Vmax())));

  ok1(equals(polar.get_Vmin(), 19.934640523));
  ok1(equals(polar.get_Smin(), polar.SinkRate(polar.get_Vmin())));
  ok1(equals(polar.get_Vtakeoff(), polar.get_Vmin() / 2));

  ok1(equals(polar.get_VbestLD(), 25.830434162));
  ok1(equals(polar.get_SbestLD(), polar.SinkRate(polar.get_VbestLD())));
  ok1(equals(polar.get_bestLD(), polar.get_VbestLD() / polar.get_SbestLD()));

  ok1(equals(polar.get_all_up_weight(), 318));
  ok1(equals(polar.get_wing_loading(), 32.448979592));
  ok1(equals(polar.get_ballast(), 0));
  ok1(equals(polar.get_ballast_litres(), 0));
  ok1(polar.is_ballastable());
  ok1(!polar.has_ballast());
}

void
GlidePolarTest::TestMC()
{
  polar.set_mc(fixed_one);

  ok1(equals(polar.get_VbestLD(), 33.482780452));
  ok1(equals(polar.get_SbestLD(), polar.SinkRate(polar.get_VbestLD())));
  ok1(equals(polar.get_bestLD(), polar.get_VbestLD() / polar.get_SbestLD()));

  polar.set_mc(fixed_zero);

  ok1(equals(polar.get_VbestLD(), 25.830434162));
  ok1(equals(polar.get_SbestLD(), polar.SinkRate(polar.get_VbestLD())));
  ok1(equals(polar.get_bestLD(), polar.get_VbestLD() / polar.get_SbestLD()));
}

void
GlidePolarTest::Run()
{
  Init();
  TestBasic();
  TestMC();
}

int main(int argc, char **argv)
{
  plan_tests(25);

  GlidePolarTest test;
  test.Run();

  return exit_status();
}
