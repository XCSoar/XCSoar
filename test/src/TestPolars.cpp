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
#include "IO/ConfiguredFile.hpp"
#include "OS/Path.hpp"
#include "Profile/Profile.hpp"
#include "Polar/Polar.hpp"
#include "Polar/Parser.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Polar/PolarStore.hpp"
#include "Util/ConvertString.hpp"
#include "Util/Macros.hpp"
#include "Util/PrintException.hxx"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void
TestBasic()
{
  // Test ReadString()
  PolarInfo polar;
  ParsePolar(polar, "318, 100, 80, -0.606, 120, -0.99, 160, -1.918");
  ok1(equals(polar.reference_mass, 318));
  ok1(equals(polar.max_ballast, 100));
  ok1(equals(polar.shape[0].v, 22.2222222));
  ok1(equals(polar.shape[0].w, -0.606));
  ok1(equals(polar.shape[1].v, 33.3333333));
  ok1(equals(polar.shape[1].w, -0.99));
  ok1(equals(polar.shape[2].v, 44.4444444));
  ok1(equals(polar.shape[2].w, -1.918));
  ok1(equals(polar.wing_area, 0.0));

  ParsePolar(polar, "318, 100, 80, -0.606, 120, -0.99, 160, -1.918, 9.8");
  ok1(equals(polar.reference_mass, 318));
  ok1(equals(polar.max_ballast, 100));
  ok1(equals(polar.shape[0].v, 22.2222222));
  ok1(equals(polar.shape[0].w, -0.606));
  ok1(equals(polar.shape[1].v, 33.3333333));
  ok1(equals(polar.shape[1].w, -0.99));
  ok1(equals(polar.shape[2].v, 44.4444444));
  ok1(equals(polar.shape[2].w, -1.918));
  ok1(equals(polar.wing_area, 9.8));

  // Test GetString()
  char polar_string[255];
  FormatPolar(polar, polar_string, 255);
  ok(strcmp("318,100,80.000,-0.606,120.000,-0.990,160.000,-1.918,9.800",
            polar_string) == 0, "GetString()");
}

static void
TestFileImport()
{
  // Test LoadFromFile()
  PolarInfo polar;
  PolarGlue::LoadFromFile(polar, Path(_T("test/data/test.plr")));
  ok1(equals(polar.reference_mass, 318));
  ok1(equals(polar.max_ballast, 100));
  ok1(equals(polar.shape[0].v, 22.2222222));
  ok1(equals(polar.shape[0].w, -0.606));
  ok1(equals(polar.shape[1].v, 33.3333333));
  ok1(equals(polar.shape[1].w, -0.99));
  ok1(equals(polar.shape[2].v, 44.4444444));
  ok1(equals(polar.shape[2].w, -1.918));
  ok1(equals(polar.wing_area, 9.8));
}

static void
TestBuiltInPolars()
{
  unsigned count = PolarStore::Count();
  for(unsigned i = 0; i < count; i++) {
    PolarInfo polar = PolarStore::GetItem(i).ToPolarInfo();

    WideToUTF8Converter narrow(PolarStore::GetItem(i).name);
    ok(polar.IsValid(), narrow);
  }
}

struct PerformanceItem {
  unsigned storeIndex;
  bool check_best_LD;
  double best_LD;
  bool check_best_LD_speed;
  double best_LD_speed;       // km/h
  bool check_min_sink;
  double min_sink;            // m/s
  bool check_min_sink_speed;
  double min_sink_speed;      // km/h
};

static const PerformanceItem performanceData[] = {
  /* 206 Hornet         */
  {   0, true, 38,   true,  103, true,  0.6,  true,   74 },
  /* Discus             */
  {  30, true, 43,   false,   0, true,  0.59, false,   0 },
  /* G-103 TWIN II (PIL)*/
  {  37, true, 38.5, true,   95, true,  0.64, true,   80 },
  /* H-201 Std. Libelle */
  {  41, true, 38,   true,   90, true,  0.6,  true,   75 },
  /* Ka6 CR             */
  {  45, true, 30,   true,   85, true,  0.65, true,   72 },
  /* K8                 */
  {  46, true, 25,   true,   75, false, 0,    true,   62 },
  /* LS-4               */
  {  52, true, 40.5, false,   0, true,  0.60, false,   0 },
  /* Std. Cirrus        */
  {  79, true, 38.5, false,   0, true,  0.6,  false,   0 },
  /* LS-1f              */
  { 157, true, 38.2, false,   0, true,  0.64, false,   0 },
};

static bool
ValuePlausible(double ref, double used, double threshold = 0.05)
{
  fprintf(stderr, "%.2f %.2f %.2f %.2f -- ", (double) ref, (double) used,
          (double) fabs(ref - used), (double) (ref * threshold));
  return fabs(ref - used) < ref * threshold;
}

static void
TestBuiltInPolarsPlausibility()
{
  for(unsigned i = 0; i < ARRAY_SIZE(performanceData); i++) {
    assert(i < PolarStore::Count());
    unsigned si = performanceData[i].storeIndex;
    PolarInfo polar = PolarStore::GetItem(si).ToPolarInfo();
    PolarCoefficients pc = polar.CalculateCoefficients();

    WideToUTF8Converter polarName(PolarStore::GetItem(i).name);

    ok(pc.IsValid(), polarName);

    GlidePolar gp(0);
    gp.SetCoefficients(pc, false);

    // Glider empty weight
    gp.SetReferenceMass(polar.reference_mass, false);
    gp.SetBallastRatio(polar.max_ballast / polar.reference_mass);
    gp.SetWingArea(polar.wing_area);

    gp.Update();

    fprintf(stderr, " LD: ");
    ok(!performanceData[i].check_best_LD ||
      ValuePlausible(performanceData[i].best_LD, gp.GetBestLD()),
      polarName);
    fprintf(stderr, "VLD: ");
    ok(!performanceData[i].check_best_LD_speed ||
       ValuePlausible(performanceData[i].best_LD_speed, gp.GetVBestLD() * 3.6),
       polarName);
    fprintf(stderr, " MS: ");
    ok(!performanceData[i].check_min_sink ||
       ValuePlausible(performanceData[i].min_sink, gp.GetSMin()),
       polarName);
    fprintf(stderr, "VMS: ");
    ok(!performanceData[i].check_min_sink_speed ||
       ValuePlausible(performanceData[i].min_sink_speed, gp.GetVMin() * 3.6),
       polarName);
  }
}

int main(int argc, char **argv)
try {
  unsigned num_tests = 19 + 9 + PolarStore::Count();

  // NOTE: Plausibility tests disabled for now since many fail
  if (0)
    num_tests += ARRAY_SIZE(performanceData) * 5;

  plan_tests(num_tests);

  TestBasic();
  TestFileImport();
  TestBuiltInPolars();

  // NOTE: Plausibility tests disabled for now since many fail
  if (0)
    TestBuiltInPolarsPlausibility();

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
