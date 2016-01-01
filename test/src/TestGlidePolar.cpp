/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Units/System.hpp"

#include <cstdio>

class GlidePolarTest
{
  GlidePolar polar;

public:
  void Run();

private:
  void Init();
  void TestBasic();
  void TestBallast();
  void TestBugs();
  void TestMC();
};

void
GlidePolarTest::Init()
{
  // Polar 1 from PolarStore (206 Hornet)
  polar.SetCoefficients(PolarCoefficients(0.0022032, -0.08784,
                                          1.47), false);

  polar.SetReferenceMass(318, false);
  polar.SetDryMass(318, false);
  polar.SetBallastRatio(100 / polar.reference_mass);

  polar.SetWingArea(9.8);

  // No ballast and no bugs on the wings
  polar.ballast = 0;
  polar.bugs = 1;

  // MC zero
  polar.mc = 0;

  polar.SetVMax(Units::ToSysUnit(200, Unit::KILOMETER_PER_HOUR), false);
}

void
GlidePolarTest::TestBasic()
{
  polar.Update();

  ok1(equals(polar.polar.a, polar.ideal_polar.a));
  ok1(equals(polar.polar.b, polar.ideal_polar.b));
  ok1(equals(polar.polar.c, polar.ideal_polar.c));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)), 0.606));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)), 0.99));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)), 1.918));

  ok1(equals(polar.GetSMax(), polar.SinkRate(polar.GetVMax())));

  ok1(equals(polar.GetVMin(), 19.934640523));
  ok1(equals(polar.GetSMin(), polar.SinkRate(polar.GetVMin())));
  ok1(equals(polar.GetVTakeoff(), polar.GetVMin() / 2));

  ok1(equals(polar.GetVBestLD(), 25.830434162));
  ok1(equals(polar.GetSBestLD(), polar.SinkRate(polar.GetVBestLD())));
  ok1(equals(polar.GetBestLD(), polar.GetVBestLD() / polar.GetSBestLD()));

  ok1(equals(polar.GetTotalMass(), 318));
  ok1(equals(polar.GetWingLoading(), 32.448979592));
  ok1(equals(polar.GetBallast(), 0));
  ok1(equals(polar.GetBallastLitres(), 0));
  ok1(polar.IsBallastable());
  ok1(!polar.HasBallast());
}

void
GlidePolarTest::TestBallast()
{
  polar.SetBallast(0.25);

  ok1(equals(polar.GetBallastLitres(), 25));
  ok1(equals(polar.GetBallast(), 0.25));

  polar.SetBallastLitres(50);

  ok1(equals(polar.GetBallastLitres(), 50));
  ok1(equals(polar.GetBallast(), 0.5));
  ok1(equals(polar.GetTotalMass(), 368));
  ok1(equals(polar.GetWingLoading(), 37.551020408));
  ok1(polar.HasBallast());

  double loading_factor = sqrt(polar.GetTotalMass() / polar.reference_mass);
  ok1(equals(polar.polar.a, polar.ideal_polar.a / loading_factor));
  ok1(equals(polar.polar.b, polar.ideal_polar.b));
  ok1(equals(polar.polar.c, polar.ideal_polar.c * loading_factor));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)),
             0.640739));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)),
             0.928976));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)),
             1.722908));

  ok1(equals(polar.GetVMin(), 21.44464));
  ok1(equals(polar.GetVBestLD(), 27.78703));

  polar.SetBallast(0);
  ok1(!polar.HasBallast());
}

void
GlidePolarTest::TestBugs()
{
  polar.SetBugs(0.75);
  ok1(equals(polar.GetBugs(), 0.75));

  ok1(equals(polar.polar.a, polar.ideal_polar.a * 4 / 3));
  ok1(equals(polar.polar.b, polar.ideal_polar.b * 4 / 3));
  ok1(equals(polar.polar.c, polar.ideal_polar.c * 4 / 3));

  ok1(equals(polar.SinkRate(Units::ToSysUnit(80, Unit::KILOMETER_PER_HOUR)),
             0.808));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(120, Unit::KILOMETER_PER_HOUR)),
             1.32));
  ok1(equals(polar.SinkRate(Units::ToSysUnit(160, Unit::KILOMETER_PER_HOUR)),
             2.557333));

  ok1(equals(polar.GetVMin(), 19.93464));
  ok1(equals(polar.GetVBestLD(), 25.83043));

  polar.SetBugs(1);
}

void
GlidePolarTest::TestMC()
{
  polar.SetMC(1);
  ok1(equals(polar.GetVBestLD(), 33.482780452));

  polar.SetMC(0);
  ok1(equals(polar.GetVBestLD(), 25.830434162));
}

void
GlidePolarTest::Run()
{
  Init();
  TestBasic();
  TestBallast();
  TestBugs();
  TestMC();
}

int main(int argc, char **argv)
{
  plan_tests(46);

  GlidePolarTest test;
  test.Run();

  return exit_status();
}
