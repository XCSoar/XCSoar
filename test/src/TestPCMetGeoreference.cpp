// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/PCMet/Georeference.hpp"
#include "Geo/GeoPoint.hpp"
#include "TestUtil.hpp"

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return GeoPoint(Angle::Degrees(longitude), Angle::Degrees(latitude));
}

/**
 * Check that an airport lands on the pixel where the DWD marker was
 * measured in a downloaded image.
 */
static bool
ProjectsTo(const PCMet::ImageGeoreference &g,
           double longitude, double latitude,
           double x, double y, double tolerance = 2.5) noexcept
{
  const auto p = g.ToPixel(MakeGeoPoint(longitude, latitude));
  return fabs(p.x - x) <= tolerance && fabs(p.y - y) <= tolerance;
}

int main()
{
  plan_tests(25);

  /* the satellite images are georeferenced, and all three channels of
     one area share the same section */
  const auto *germany = PCMet::FindImageGeoreference("sat/index.htm",
                                                     "vis_hrv_mdl");
  ok1(germany != nullptr);
  ok1(germany->IsDefined());
  ok1(PCMet::FindImageGeoreference("sat/index.htm", "ir_108_mdl") == germany);
  ok1(PCMet::FindImageGeoreference("sat/index.htm", "ir_rgb_mdl") == germany);

  /* unknown products have no georeference */
  ok1(PCMet::FindImageGeoreference("sat/index.htm", "vis_hrv_xyz") == nullptr);
  ok1(PCMet::FindImageGeoreference("rad/index.htm", "rx") == nullptr);

  /* the image tables are null-terminated, so a caller walking one
     entry too far must not crash */
  ok1(PCMet::FindImageGeoreference("sat/index.htm", nullptr) == nullptr);
  ok1(PCMet::FindImageGeoreference(nullptr, nullptr) == nullptr);

  /* airport markers measured in nb_ir_rgb_mdl_2608261845_sat.jpg */
  ok1(ProjectsTo(*germany,  9.9882, 53.6304, 487.12, 109.50));  // EDDH
  ok1(ProjectsTo(*germany, 13.5033, 52.3667, 695.29, 223.44));  // EDDB
  ok1(ProjectsTo(*germany,  8.5706, 50.0333, 398.04, 456.46));  // EDDF
  ok1(ProjectsTo(*germany, 11.7861, 48.3538, 605.47, 621.32));  // EDDM
  ok1(ProjectsTo(*germany, 16.5697, 48.1103, 922.29, 622.71));  // LOWW

  /* the northern and southern halves are separate products */
  const auto *north = PCMet::FindImageGeoreference("sat/index.htm",
                                                   "vis_hrv_ndl");
  const auto *south = PCMet::FindImageGeoreference("sat/index.htm",
                                                   "vis_hrv_sdl");
  ok1(ProjectsTo(*north,  9.9882, 53.6304, 510.34, 441.60));   // EDDH
  ok1(ProjectsTo(*south, 11.7861, 48.3538, 577.95, 181.85));   // EDDM

  /* the Europa section, checked against markers far apart */
  const auto *europe = PCMet::FindImageGeoreference("sat/index.htm",
                                                    "ir_rgb_eu");
  ok1(europe != nullptr);
  ok1(ProjectsTo(*europe, 37.9063, 55.4088, 831.04, 217.86, 2));  // UUDD
  ok1(ProjectsTo(*europe, -6.2701, 53.4213, 394.36, 294.64, 2));  // EIDW

  /* the central meridian runs straight up the image */
  const auto a = germany->ToPixel(MakeGeoPoint(10, 48));
  const auto b = germany->ToPixel(MakeGeoPoint(10, 54));
  ok1(equals(a.x, b.x));
  ok1(a.y > b.y);

  /* the meridians converge, so north is up on 10E only */
  ok1(equals(germany->GetUpBearing(MakeGeoPoint(10, 50)).Degrees(), 0));
  ok1(equals(germany->GetUpBearing(MakeGeoPoint(15, 50)).Degrees(), 5));

  /* IsInside() */
  ok1(germany->IsInside(germany->ToPixel(MakeGeoPoint(11, 48))));
  ok1(!germany->IsInside(germany->ToPixel(MakeGeoPoint(11, 60))));
  ok1(!germany->IsInside(germany->ToPixel(MakeGeoPoint(-5, 48))));

  return exit_status();
}
