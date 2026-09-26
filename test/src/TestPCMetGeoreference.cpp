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
  plan_tests(45);

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
  ok1(PCMet::FindImageGeoreference("rad_lokal/einzelstandorte.htm",
                                   "eddm") == nullptr);
  ok1(PCMet::FindImageGeoreference("blitzkarte_bild.htm", "0") == nullptr);

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
  const auto a = germany->ToPixel(MakeGeoPoint(PCMet::CENTRAL_MERIDIAN, 48));
  const auto b = germany->ToPixel(MakeGeoPoint(PCMet::CENTRAL_MERIDIAN, 54));
  ok1(equals(a.x, b.x));
  ok1(a.y > b.y);

  /* the meridians converge, so north is up on the central meridian
     only, and the bearing grows with the distance from it */
  ok1(equals(germany->GetUpBearing(
    MakeGeoPoint(PCMet::CENTRAL_MERIDIAN, 50)).Degrees(), 0));
  ok1(equals(germany->GetUpBearing(
    MakeGeoPoint(PCMet::CENTRAL_MERIDIAN + 5, 50)).Degrees(), 5));

  /* IsInside() */
  ok1(germany->IsInside(germany->ToPixel(MakeGeoPoint(11, 48))));
  ok1(!germany->IsInside(germany->ToPixel(MakeGeoPoint(11, 60))));
  ok1(!germany->IsInside(germany->ToPixel(MakeGeoPoint(-5, 48))));

  /* The RADAR composites and the combined SAT RAD BLITZ image.  The
     pixel positions below were read off the downloaded images: the
     crosshair of each was checked to sit on the city marker, and the
     latitude circles were measured at the central meridian, which is
     the only meridian this projection draws vertically. */
  {
    struct { const char *type, *area; unsigned w, map_h; } const areas[] = {
      { "rad/index.htm", "rx", 600, 698 },
      { "rad/index.htm", "rxn", 1000, 698 },
      { "rad/index.htm", "rxm", 1000, 698 },
      { "rad/index.htm", "rxs", 1000, 698 },
      { "rad/index.htm", "eu", 1000, 697 },
      { "rad/index.htm", "fa", 1000, 697 },
      { "satradblitz/index.htm", "eh", 1000, 727 },
    };

    bool all_defined = true;
    for (const auto &a : areas) {
      const auto *g = PCMet::FindImageGeoreference(a.type, a.area);
      if (g == nullptr || !g->IsDefined() ||
          g->nominal_size.width != a.w || g->map_height != a.map_h)
        all_defined = false;
    }
    ok1(all_defined);
  }

  ok1(PCMet::FindImageGeoreference("rad/index.htm", "xx") == nullptr);

  const auto *rx = PCMet::FindImageGeoreference("rad/index.htm", "rx");
  ok1(ProjectsTo(*rx, 9.9937, 53.5511, 287.72, 158.79, 3));   // Hamburg
  ok1(ProjectsTo(*rx, 11.5820, 48.1351, 369.02, 566.26, 3));  // Munich

  const auto *rxn = PCMet::FindImageGeoreference("rad/index.htm", "rxn");
  ok1(ProjectsTo(*rxn, 9.9937, 53.5511, 478.51, 292.29, 3));  // Hamburg
  ok1(ProjectsTo(*rxn, 7.4653, 51.5136, 271.98, 553.58, 3));  // Dortmund

  const auto *rxm = PCMet::FindImageGeoreference("rad/index.htm", "rxm");
  ok1(ProjectsTo(*rxm, 6.9603, 50.9375, 217.16, 296.23, 3));  // Cologne
  ok1(ProjectsTo(*rxm, 12.3731, 51.3397, 677.64, 244.45, 3)); // Leipzig

  const auto *rxs = PCMet::FindImageGeoreference("rad/index.htm", "rxs");
  ok1(ProjectsTo(*rxs, 11.5820, 48.1351, 638.28, 467.29, 3)); // Munich
  ok1(ProjectsTo(*rxs, 8.6821, 50.1109, 376.39, 195.96, 3));  // Frankfurt

  /* the Europa composite draws its graticule every ten degrees, so it
     rests on fewer reference points and gets a wider tolerance */
  const auto *radar_eu = PCMet::FindImageGeoreference("rad/index.htm", "eu");
  ok1(ProjectsTo(*radar_eu, -0.1276, 51.5072, 358.50, 277.62, 5)); // London
  ok1(ProjectsTo(*radar_eu, 2.3522, 48.8566, 399.98, 380.06, 5));  // Paris

  const auto *alps = PCMet::FindImageGeoreference("rad/index.htm", "fa");
  ok1(ProjectsTo(*alps, 8.5417, 47.3769, 347.17, 214.54, 3));  // Zurich
  ok1(ProjectsTo(*alps, 6.1432, 46.2044, 157.91, 336.81, 3));  // Geneva

  const auto *srb = PCMet::FindImageGeoreference("satradblitz/index.htm",
                                                 "eh");
  ok1(ProjectsTo(*srb, 8.5417, 47.3769, 575.20, 509.53, 3));   // Zurich
  ok1(ProjectsTo(*srb, 9.1900, 45.4642, 595.97, 607.06, 3));   // Milan

  /* a position that projects into the legend strip is not on the map */
  ok1(rx->IsInside({300.0, 690.0}));
  ok1(!rx->IsInside({300.0, 700.0}));
  ok1(!rx->IsInside({300.0, 740.0}));

  return exit_status();
}
