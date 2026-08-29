// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/OPERA/Radar.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "TestUtil.hpp"

#include <chrono>
#include <cmath>

#include <string.h>

[[gnu::pure]]
static bool
Contains(const std::string &haystack, const char *needle) noexcept
{
  return haystack.find(needle) != std::string::npos;
}

int main()
{
  plan_tests(28);

  /* 12:07:30 minus the seven minute dissemination margin is 12:00:30,
     which rounds down to the 12:00 composite */
  const BrokenDateTime noon{BrokenDate{2026, 8, 28}, BrokenTime{12, 7, 30}};
  const auto url = OPERA::MakeCompositeURL(noon);

  ok1(Contains(url, "https://s3.waw3-1.cloudferro.com/openradar-24h/"));
  ok1(Contains(url, "/2026/08/28/OPERA/COMP/"));
  ok1(Contains(url, "OPERA@20260828T1200@0@DBZH.tiff"));

  /* the margin may cross midnight, and then the date has to follow */
  const BrokenDateTime after_midnight{BrokenDate{2026, 8, 28},
                                      BrokenTime{0, 3, 0}};
  const auto wrapped = OPERA::MakeCompositeURL(after_midnight);
  ok1(Contains(wrapped, "/2026/08/27/OPERA/COMP/"));
  ok1(Contains(wrapped, "OPERA@20260827T2355@0@DBZH.tiff"));

  /* without a clock we must not guess */
  ok1(OPERA::MakeCompositeURL(BrokenDateTime::Invalid()).empty());

  /* CompositeTime() must name the very frame the URL points at, or
     the age of the picture on the map would be computed against the
     wrong instant */
  const auto composite = OPERA::CompositeTime(noon);
  ok1(composite.hour == 12 && composite.minute == 0);
  ok1(composite.second == 0);
  ok1(!OPERA::CompositeTime(BrokenDateTime::Invalid()).IsPlausible());

  /* the frame we request is never newer than the margin and never
     older than the margin plus one cadence step; the age limit has to
     sit clear of that, or a frame would expire as it arrived */
  const auto fresh_age = noon - composite;
  ok1(fresh_age >= std::chrono::minutes{OPERA::LATENCY_MINUTES});
  ok1(fresh_age < std::chrono::minutes{OPERA::LATENCY_MINUTES +
                                       OPERA::CADENCE_MINUTES});
  ok1(fresh_age < std::chrono::minutes{OPERA::MAX_AGE_MINUTES});

  /* the projection centre lands on the grid's false origin */
  const auto centre = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                              Angle::Degrees(55)});
  ok1(equals(centre.x, 1950000));
  ok1(equals(centre.y, -2100000));

  /* east is +x, north is +y */
  const auto east = OPERA::Project(GeoPoint{Angle::Degrees(15),
                                            Angle::Degrees(55)});
  const auto north = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                             Angle::Degrees(60)});
  ok1(east.x > centre.x);
  ok1(std::fabs(east.y - centre.y) < std::fabs(east.x - centre.x));
  ok1(north.y > centre.y);
  ok1(std::fabs(north.x - centre.x) < std::fabs(north.y - centre.y));

  /* one degree of latitude is roughly 111km in this projection */
  const auto one_degree = OPERA::Project(GeoPoint{Angle::Degrees(10),
                                                  Angle::Degrees(56)});
  ok1(std::fabs(one_degree.y - centre.y) > 110000);
  ok1(std::fabs(one_degree.y - centre.y) < 112000);

  /* weak echo is not drawn at all; the reference product clips it */
  ok1(OPERA::ClassifyReflectivity(0) < 0);
  ok1(OPERA::ClassifyReflectivity(18) < 0);

  /* and neither is "no radar here", which the composite marks with a
     large negative number.  Its other marker, a NaN, cannot be tested
     here: we are built with -ffast-math, under which a literal NaN is
     folded away before it ever reaches the function. */
  ok1(OPERA::ClassifyReflectivity(-9999000) < 0);

  /* and the classes climb from there */
  ok1(OPERA::ClassifyReflectivity(19) == 0);
  ok1(OPERA::ClassifyReflectivity(40) > OPERA::ClassifyReflectivity(30));
  ok1(OPERA::GetClassColour(999) ==
      OPERA::GetClassColour(OPERA::N_CLASSES - 1));

  /* every class must be reachable, and every colour therefore
     reachable with it: two equal bounds would make the classifier step
     over one and that colour would never be drawn.  Walk the whole
     scale and check that it visits each index in turn, once. */
  int previous = -1;
  unsigned distinct = 0;
  for (double dbz = 0; dbz < 80; dbz += 0.01) {
    const int i = OPERA::ClassifyReflectivity(dbz);
    if (i == previous)
      continue;

    /* it may never skip an index or go backwards */
    if (i != previous + 1)
      break;

    previous = i;
    ++distinct;
  }

  ok1(distinct == OPERA::N_CLASSES);
  ok1(previous == int(OPERA::N_CLASSES) - 1);

  return exit_status();
}
