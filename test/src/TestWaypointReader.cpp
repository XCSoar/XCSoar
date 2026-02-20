// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint/WaypointReader.hpp"
#include "Waypoint/WaypointReaderBase.hpp"
#include "Waypoint/WaypointReaderSeeYou.hpp"
#include "Waypoint/CupWriter.hpp"
#include "Waypoint/Factory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Terrain/RasterMap.hpp"
#include "Units/System.hpp"
#include "TestUtil.hpp"
#include "system/Path.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"
#include "io/MemoryReader.hxx"
#include "io/StringOutputStream.hxx"
#include "util/StringAPI.hxx"
#include "util/StringStrip.hxx"
#include "Operation/Operation.hpp"

#include <string>
#include <string_view>
#include <vector>

using std::string_view_literals::operator""sv;

typedef std::vector<Waypoint> wp_vector;

static bool
TestWaypointFile(Path filename, Waypoints &way_points, unsigned num_wps)
{
  NullOperationEnvironment operation;

  try {
    ReadWaypointFile(filename, way_points,
                     WaypointFactory(WaypointOrigin::NONE),
                     operation);
    ok1(true);
  } catch (...) {
    ok1(false);
    skip(2, 0, "parsing waypoint file failed");
    return false;
  }

  way_points.Optimise();

  ok1(!way_points.IsEmpty());
  ok1(way_points.size() == num_wps);

  return true;
}

static WaypointPtr
GetWaypoint(const Waypoint org_wp, const Waypoints &way_points)
{
  const WaypointPtr wp = way_points.LookupName(org_wp.name);
  if (!ok1(wp != NULL)) {
    skip(2, 0, "waypoint not found");
    return NULL;
  }
  if(!ok1(wp->location.Distance(org_wp.location) <= 1000))
    printf("%f %f\n", (double)wp->location.latitude.Degrees(), (double)wp->location.longitude.Degrees());
  ok1(org_wp.has_elevation);
  ok1(wp->has_elevation);
  ok1(fabs(wp->elevation - org_wp.elevation) < 0.5);

  return wp;
}

static void
TestWinPilotWaypoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(7, 0, "waypoint not found");
    return;
  }

  ok1(wp->type == ((!org_wp.IsLandable()) ?
                    Waypoint::Type::NORMAL : (Waypoint::Type)org_wp.type));
  ok1(wp->flags.turn_point == org_wp.flags.turn_point);
  ok1(wp->flags.home == org_wp.flags.home);
  ok1(wp->flags.start_point == org_wp.flags.start_point);
  ok1(wp->flags.finish_point == org_wp.flags.finish_point);
  ok1(wp->runway.IsDirectionDefined() == org_wp.runway.IsDirectionDefined() &&
      (!wp->runway.IsDirectionDefined() ||
       wp->runway.GetDirectionDegrees() == org_wp.runway.GetDirectionDegrees()));
}

static void
TestWinPilot(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints.dat"), way_points,
                        org_wp.size())) {
    skip(10 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (const auto &i : org_wp) {
    const auto wp = GetWaypoint(i, way_points);
    TestWinPilotWaypoint(i, wp.get());
  }
}

static void
TestSeeYouWaypoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(6, 0, "waypoint not found");
    return;
  }

  ok1(wp->type == org_wp.type);
  ok1(wp->flags.turn_point == org_wp.flags.turn_point);
  // No home waypoints in a SeeYou file
  //ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->flags.start_point == org_wp.flags.start_point);
  ok1(wp->flags.finish_point == org_wp.flags.finish_point);
  ok1(wp->comment == org_wp.comment);
  ok1(wp->runway.IsDirectionDefined() == org_wp.runway.IsDirectionDefined() &&
      (!wp->runway.IsDirectionDefined() ||
       wp->runway.GetDirectionDegrees() == org_wp.runway.GetDirectionDegrees()));
  ok1(wp->runway.IsLengthDefined() == org_wp.runway.IsLengthDefined() &&
      (!wp->runway.IsLengthDefined() ||
       wp->runway.GetLength() == org_wp.runway.GetLength()));
}

static void
TestSeeYou(const wp_vector &org_wp)
{
  // Test a SeeYou waypoint file with no runway width field:
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints.cup"), way_points,
                        org_wp.size())) {
    skip(9 * org_wp.size(), 0, "opening waypoints.cup failed");
  } else {
    for (const auto &i : org_wp) {
      const auto wp = GetWaypoint(i, way_points);
      TestSeeYouWaypoint(i, wp.get());
    }
  }

  // Test a SeeYou waypoint file with a runway width field:
  Waypoints way_points2;
  if (!TestWaypointFile(Path("test/data/waypoints2.cup"), way_points2,
                        org_wp.size())) {
    skip(9 * org_wp.size(), 0, "opening waypoints2.cup failed");
    return;
  }

  for (const auto &i : org_wp) {
    const auto wp2 = GetWaypoint(i, way_points2);
    TestSeeYouWaypoint(i, wp2.get());
  }
  // Test a SeeYou waypoint file with userdata and pics fields:
  Waypoints way_points3;
  if (!TestWaypointFile(Path("test/data/waypoints3.cup"), way_points3,
                        org_wp.size())) {
    skip(9 * org_wp.size(), 0, "opening waypoints3.cup failed");
    return;
  }

  for (const auto &i : org_wp) {
    const auto wp3 = GetWaypoint(i, way_points3);
    TestSeeYouWaypoint(i, wp3.get());
  }
}

static void
TestZanderWaypoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(7, 0, "waypoint not found");
    return;
  }

  ok1(wp->type == ((!org_wp.IsLandable()) ?
                    Waypoint::Type::NORMAL : (Waypoint::Type)org_wp.type));
  ok1(wp->flags.turn_point == org_wp.flags.turn_point);
  ok1(wp->flags.home == org_wp.flags.home);
  ok1(wp->flags.start_point == org_wp.flags.start_point);
  ok1(wp->flags.finish_point == org_wp.flags.finish_point);
}

static void
TruncateStrip(std::string &s, std::size_t max_length) noexcept
{
  std::string_view v = s;
  if (v.size() > max_length)
    v = v.substr(0, max_length);
  v = Strip(v);
  s.assign(v);
}

static void
TestZander(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints.wpz"), way_points,
                        org_wp.size())) {
    skip(10 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    TruncateStrip(i.name, 12);
    const auto wp = GetWaypoint(i, way_points);
    TestZanderWaypoint(i, wp.get());
  }
}

static void
TestFS(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints_geo.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    TruncateStrip(i.name, 8);
    GetWaypoint(i, way_points);
  }
}

static void
TestFS_UTM(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints_utm.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    TruncateStrip(i.name, 8);
    GetWaypoint(i, way_points);
  }
}

static void
TestOzi(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints_ozi.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    i.name = std::string{Strip(i.name)};
    GetWaypoint(i, way_points);
  }
}

static void
TestCompeGPS(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints_compe_geo.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    size_t pos;
    while ((pos = i.name.find_first_of(' ')) != std::string::npos)
      i.name.erase(pos, 1);

    TruncateStrip(i.name, 6);
    const auto wp = GetWaypoint(i, way_points);
    ok1(wp->comment == i.comment);
  }
}

static void
TestCompeGPS_UTM(const wp_vector &org_wp)
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/waypoints_compe_utm.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  for (auto i : org_wp) {
    size_t pos;
    while ((pos = i.name.find_first_of(' ')) != std::string::npos)
      i.name.erase(pos, 1);

    TruncateStrip(i.name, 6);
    const auto wp = GetWaypoint(i, way_points);
    ok1(wp->comment == i.comment);
  }
}

static std::string
WriteCupToString(const wp_vector &org_wp, bool with_header = false)
{
  StringOutputStream sos;
  WithBufferedOutputStream(sos, [&](BufferedOutputStream &bos){
    if (with_header)
      WriteCupHeader(bos);
    for (const auto &i : org_wp)
      WriteCup(bos, i);
  });
  return std::move(sos).GetValue();
}

static void
TestCupWriter(const wp_vector &org_wp)
{
  // Test exact output format (2022 CUP spec with rwwidth, userdata, pics)
  const auto s = WriteCupToString(org_wp);
  ok1(s == R"cup("Bergneustadt","",,5103.117N,00742.367E,488M,4,040,590M,,,"Rabbit holes, 20"" ditch south end of rwy","",""
"Aconcagua","",,3239.200S,07000.700W,6962M,7,,,,,"Highest mountain in south-america","",""
"Golden Gate Bridge","",,3749.050N,12228.700W,227M,14,,,,,"","",""
"Red Square","",,5545.250N,03737.200E,123M,3,090,016M,,,"","",""
"Sydney Opera","",,3351.417S,15112.917E,5M,1,,,,,"","",""
)cup"sv);
}

static void
TestCupRoundTrip(const wp_vector &org_wp)
{
  // Write waypoints to CUP string (with header for 2022 format detection)
  const auto s = WriteCupToString(org_wp, true);

  // Parse them back
  auto bytes = std::as_bytes(std::span{s.data(), s.size()});
  MemoryReader mr(bytes);
  BufferedReader br(mr);

  Waypoints waypoints;
  WaypointFactory factory(WaypointOrigin::USER);
  ParseSeeYou(factory, waypoints, br);
  waypoints.Optimise();

  ok1(waypoints.size() == org_wp.size());

  for (const auto &org : org_wp) {
    auto wp = waypoints.LookupName(org.name);
    if (!ok1(wp != nullptr)) {
      skip(8, 0, "waypoint not found in round-trip");
      continue;
    }

    ok1(wp->location.Distance(org.location) <= 1000);
    ok1(wp->has_elevation == org.has_elevation);
    ok1(!wp->has_elevation || fabs(wp->elevation - org.elevation) < 0.5);
    ok1(wp->type == org.type);
    ok1(wp->comment == org.comment);
    ok1(wp->details == org.details);
    ok1(wp->runway.IsDirectionDefined() == org.runway.IsDirectionDefined());
    ok1(wp->runway.IsLengthDefined() == org.runway.IsLengthDefined());
  }
}

static void
TestCupx()
{
  Waypoints way_points;
  if (!TestWaypointFile(Path("test/data/test.cupx"), way_points, 2)) {
    skip(7, 0, "opening CUPX file failed");
    return;
  }

  const auto wp = way_points.LookupName("Test Airfield");
  ok1(wp != nullptr);
  if (wp == nullptr) {
    skip(6, 0, "waypoint not found");
    return;
  }

  ok1(wp->type == Waypoint::Type::AIRFIELD);
  ok1(wp->has_elevation);
  ok1(fabs(wp->elevation - 500.0) < 0.5);
  ok1(wp->comment == "A test airfield");

  /* verify files_embed has the bare image filename */
  ok1(!wp->files_embed.empty());
  const auto &first_embed = wp->files_embed.front();
  ok1(first_embed == "test_image.jpg");
}

static wp_vector
CreateOriginalWaypoints()
{
  wp_vector org_wp;

  GeoPoint loc;

  // Bergneustadt
  loc.latitude = Angle::Degrees(51.051944444444445);
  loc.longitude = Angle::Degrees(7.7061111111111114);

  Waypoint wp(loc);
  wp.elevation = 488;
  wp.has_elevation = true;
  wp.name = "Bergneustadt";
  wp.comment = "Rabbit holes, 20\" ditch south end of rwy";
  wp.runway.SetDirection(Angle::Degrees(40));
  wp.runway.SetLength(590);

  wp.type = Waypoint::Type::AIRFIELD;
  wp.flags.turn_point = true;
  wp.flags.home = true;
  wp.flags.start_point = false;
  wp.flags.finish_point = false;
  org_wp.push_back(wp);

  // Aconcagua
  loc.latitude = Angle::DMS(32, 39, 12, true);
  loc.longitude = Angle::DMS(70, 0, 42, true);

  Waypoint wp2(loc);
  wp2.elevation = 6962;
  wp2.has_elevation = true;
  wp2.name = "Aconcagua";
  wp2.comment = "Highest mountain in south-america";

  wp2.type = Waypoint::Type::MOUNTAIN_TOP;
  wp2.flags.turn_point = true;
  wp2.flags.home = false;
  wp2.flags.start_point = false;
  wp2.flags.finish_point = false;
  org_wp.push_back(wp2);

  // Golden Gate Bridge
  loc.latitude = Angle::FromDMS(37, 49, 3);
  loc.longitude = Angle::FromDMS(122, 28, 42).Flipped();

  Waypoint wp3(loc);
  wp3.elevation = 227;
  wp3.has_elevation = true;
  wp3.name = "Golden Gate Bridge";
  wp3.comment = "";

  wp3.type = Waypoint::Type::BRIDGE;
  wp3.flags.turn_point = true;
  wp3.flags.home = false;
  wp3.flags.start_point = false;
  wp3.flags.finish_point = false;
  org_wp.push_back(wp3);

  // Red Square
  loc.latitude = Angle::DMS(55, 45, 15);
  loc.longitude = Angle::DMS(37, 37, 12);

  Waypoint wp4(loc);
  wp4.elevation = 123;
  wp4.has_elevation = true;
  wp4.name = "Red Square";
  wp4.runway.SetDirection(Angle::Degrees(90));
  wp4.runway.SetLength((unsigned)Units::ToSysUnit(0.01, Unit::STATUTE_MILES));

  wp4.type = Waypoint::Type::OUTLANDING;
  wp4.flags.turn_point = true;
  wp4.flags.home = false;
  wp4.flags.start_point = false;
  wp4.flags.finish_point = false;
  org_wp.push_back(wp4);

  // Sydney Opera
  loc.latitude = Angle::DMS(33, 51, 25, true);
  loc.longitude = Angle::DMS(151, 12, 55);

  Waypoint wp5(loc);
  wp5.elevation = 5;
  wp5.has_elevation = true;
  wp5.name = "Sydney Opera";
  wp5.comment = "";

  wp5.type = Waypoint::Type::NORMAL;
  wp5.flags.turn_point = true;
  wp5.flags.home = false;
  wp5.flags.start_point = false;
  wp5.flags.finish_point = false;
  org_wp.push_back(wp5);

  return org_wp;
}

int main()
{
  wp_vector org_wp = CreateOriginalWaypoints();

  plan_tests(507);

  TestWinPilot(org_wp);
  TestSeeYou(org_wp);
  TestCupx();
  TestZander(org_wp);
  TestFS(org_wp);
  TestFS_UTM(org_wp);
  TestOzi(org_wp);
  TestCompeGPS(org_wp);
  TestCompeGPS_UTM(org_wp);
  TestCupWriter(org_wp);
  TestCupRoundTrip(org_wp);

  return exit_status();
}
