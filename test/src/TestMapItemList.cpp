// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* Which airspace the "Map elements at this location" list selects
 * when it opens: the one drawn at the touched location. */

#include "MapWindow/Items/List.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Radio/TransponderCode.hpp"
#include "time/BrokenDateTime.hpp"
#include "TestUtil.hpp"

#include <memory>
#include <vector>

/* Grenoble-Isère (LFLS) */
static constexpr GeoPoint CENTER{Angle::Degrees(5.33),
                                 Angle::Degrees(45.36)};

static AirspaceAltitude
MSL(double altitude) noexcept
{
  AirspaceAltitude a{};
  a.altitude = altitude;
  a.reference = AltitudeReference::MSL;
  return a;
}

static void
SetProperties(AbstractAirspace &airspace, const char *name,
              double base, double top)
{
  airspace.SetProperties(name, false,
                         BrokenDateTime::Invalid(),
                         BrokenDateTime::Invalid(),
                         true, false, "", TransponderCode{},
                         AirspaceClass::CLASSD, AirspaceClass::CLASSD,
                         MSL(base), MSL(top));
}

static AirspacePtr
MakeCircle(const char *name, double radius, double base, double top)
{
  auto airspace = std::make_shared<AirspaceCircle>(CENTER, radius);
  SetProperties(*airspace, name, base, top);
  return airspace;
}

/** A square of #half_side degrees around #CENTER. */
static AirspacePtr
MakeSquare(const char *name, double half_side, double base, double top)
{
  const Angle d = Angle::Degrees(half_side);
  const std::vector<GeoPoint> points{
    {CENTER.longitude - d, CENTER.latitude - d},
    {CENTER.longitude + d, CENTER.latitude - d},
    {CENTER.longitude + d, CENTER.latitude + d},
    {CENTER.longitude - d, CENTER.latitude + d},
  };
  auto airspace = std::make_shared<AirspacePolygon>(points);
  SetProperties(*airspace, name, base, top);
  return airspace;
}

int
main()
{
  plan_tests(4);

  MapItemList list;
  list.append(new LocationMapItem(CENTER, GeoVector::Zero(), 400));
  ok1(list.FindSmallestAirspace() == list.size());

  /* sorted as in the dialog: highest base first; the CTR is last
     although it is the one drawn at this location */
  list.append(new AirspaceMapItem(MakeSquare("CTA", 0.8, 3500, 4400)));
  list.append(new AirspaceMapItem(MakeCircle("TMA", 40000, 1370, 1980)));
  list.append(new AirspaceMapItem(MakeCircle("CTR", 9000, 0, 1067)));
  ok1(list.FindSmallestAirspace() == 3);

  /* polygons and circles compare by their size alone */
  list.append(new AirspaceMapItem(MakeSquare("RMZ", 0.05, 0, 900)));
  ok1(list.FindSmallestAirspace() == 4);

  /* on a tie the first one in list order wins */
  list.append(new AirspaceMapItem(MakeSquare("TMZ", 0.05, 0, 900)));
  ok1(list.FindSmallestAirspace() == 4);

  return exit_status();
}
