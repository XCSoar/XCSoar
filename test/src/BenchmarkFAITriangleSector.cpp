// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Shapes/FAITriangleArea.hpp"
#include "Engine/Task/Shapes/FAITriangleSettings.hpp"
#include "Geo/GeoPoint.hpp"

int
main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
  FAITriangleSettings settings;
  settings.SetDefaults();

  const GeoPoint a(Angle::Degrees(7.70722),
                   Angle::Degrees(51.052));
  const GeoPoint b(Angle::Degrees(11.5228),
                   Angle::Degrees(50.3972));

  GeoPoint buffer[FAI_TRIANGLE_SECTOR_MAX];

  for (unsigned i = 256 * 1024; i-- > 0;)
    GenerateFAITriangleArea(buffer, a, b, false, settings);

  return 0;
}
