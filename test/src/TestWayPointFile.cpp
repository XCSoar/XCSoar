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

#include "WayPoint/WayPointFile.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Terrain/RasterMap.hpp"
#include "Units/Units.hpp"
#include "TestUtil.hpp"
#include "tstring.hpp"

#include <vector>

static void
TestExtractParameters()
{
  TCHAR buffer[1024];
  const TCHAR *params[64];
  unsigned n;

  // test basic functionality

  n = WayPointFile::extractParameters(_T(""), buffer, params, 64);
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("")) == 0);

  n = WayPointFile::extractParameters(_T("foo"), buffer, params, 64);
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo")) == 0);

  n = WayPointFile::extractParameters(_T("foo,bar"), buffer, params, 64);
  ok1(n == 2);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);

  n = WayPointFile::extractParameters(_T("foo,bar"), buffer, params, 1);
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo")) == 0);

  n = WayPointFile::extractParameters(_T("foo,bar,"), buffer, params, 64);
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);
  ok1(_tcscmp(params[2], _T("")) == 0);

  n = WayPointFile::extractParameters(_T("foo,bar,,"), buffer, params, 64);
  ok1(n == 4);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);
  ok1(_tcscmp(params[2], _T("")) == 0);
  ok1(_tcscmp(params[3], _T("")) == 0);


  // with qoutes but no quote handling

  n = WayPointFile::extractParameters(_T("\"foo,comma\",\"bar\""),
                                      buffer, params, 64);
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("\"foo")) == 0);
  ok1(_tcscmp(params[1], _T("comma\"")) == 0);
  ok1(_tcscmp(params[2], _T("\"bar\"")) == 0);


  // quote handling

  n = WayPointFile::extractParameters(_T("\"\""),
                                      buffer, params, 64, false, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("")) == 0);

  n = WayPointFile::extractParameters(_T("\"\"\""),
                                      buffer, params, 64, false, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("\"")) == 0);

  n = WayPointFile::extractParameters(_T("\"\"\"\""),
                                      buffer, params, 64, false, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("\"")) == 0);

  n = WayPointFile::extractParameters(_T("\"foo,comma\",\"bar\""),
                                      buffer, params, 64, false, _T('"'));
  ok1(n == 2);
  ok1(_tcscmp(params[0], _T("foo,comma")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);


  // no quotes, whitespace removal

  n = WayPointFile::extractParameters(_T("foo bar"),
                                      buffer, params, 64, true);
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo bar")) == 0);

  n = WayPointFile::extractParameters(_T("foo , bar, baz"),
                                      buffer, params, 64, true);
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);
  ok1(_tcscmp(params[2], _T("baz")) == 0);

  n = WayPointFile::extractParameters(_T(" foo  ,  bar  , baz "),
                                      buffer, params, 64, true);
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("bar")) == 0);
  ok1(_tcscmp(params[2], _T("baz")) == 0);

  n = WayPointFile::extractParameters(_T(" foo\"  , \" bar \"  , \"baz "),
                                      buffer, params, 64, true);
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo\"")) == 0);
  ok1(_tcscmp(params[1], _T("\" bar \"")) == 0);
  ok1(_tcscmp(params[2], _T("\"baz")) == 0);

  // quote handling, whitespace removal

  n = WayPointFile::extractParameters(_T("\"foo \" , \" bar\", \" baz\""),
                                      buffer, params, 64, true, _T('"'));
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo ")) == 0);
  ok1(_tcscmp(params[1], _T(" bar")) == 0);
  ok1(_tcscmp(params[2], _T(" baz")) == 0);

  n = WayPointFile::extractParameters(
                            _T(" \" foo  \"  ,  \"  bar  \"  , \" baz \" "),
                            buffer, params, 64, true, _T('"'));
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T(" foo  ")) == 0);
  ok1(_tcscmp(params[1], _T("  bar  ")) == 0);
  ok1(_tcscmp(params[2], _T(" baz ")) == 0);

  n = WayPointFile::extractParameters(_T("\"foo\",\"\",\"bar\""),
                                      buffer, params, 64, true, _T('"'));
  ok1(n == 3);
  ok1(_tcscmp(params[0], _T("foo")) == 0);
  ok1(_tcscmp(params[1], _T("")) == 0);
  ok1(_tcscmp(params[2], _T("bar")) == 0);

  // missing end quote
  n = WayPointFile::extractParameters(_T("\"foo, bar"),
                                      buffer, params, 64, true, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo, bar")) == 0);

  // embedded quotes and commas
  n = WayPointFile::extractParameters(_T("\"foo, \"bar\"\""),
                                      buffer, params, 64, true, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo, \"bar\"")) == 0);

  n = WayPointFile::extractParameters(_T("\"foo, \"\"bar\"\"\""),
                                      buffer, params, 64, true, _T('"'));
  ok1(n == 1);
  ok1(_tcscmp(params[0], _T("foo, \"bar\"")) == 0);
}

typedef std::vector<Waypoint> wp_vector;

static bool
TestWayPointFile(const TCHAR* filename, Waypoints &way_points, unsigned num_wps)
{
  WayPointFile *f = WayPointFile::create(filename, 0);
  if (!ok1(f != NULL)) {
    skip(3, 0, "opening waypoint file failed");
    return false;
  }

  if(!ok1(f->Parse(way_points))) {
    delete f;
    skip(2, 0, "parsing waypoint file failed");
  }

  delete f;

  way_points.optimise();

  ok1(!way_points.empty());
  ok1(way_points.size() == num_wps);

  return true;
}

static const Waypoint*
GetWayPoint(const Waypoint org_wp, const Waypoints &way_points)
{
  const Waypoint *wp = way_points.lookup_name(org_wp.Name);
  if (!ok1(wp != NULL)) {
    skip(2, 0, "waypoint not found");
    return NULL;
  }
  if(!ok1(wp->Location.distance(org_wp.Location) <= fixed(1000)))
    printf("%f %f\n", (double)wp->Location.Latitude.value_degrees(), (double)wp->Location.Longitude.value_degrees());
  ok1(equals(wp->Altitude, org_wp.Altitude));

  return wp;
}

static void
TestWinPilotWayPoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(7, 0, "waypoint not found");
    return;
  }

  ok1(wp->Type == ((!org_wp.is_landable()) ? wtNormal : org_wp.Type));
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
  ok1(wp->RunwayDirection == org_wp.RunwayDirection);
}

static void
TestWinPilot(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.dat"), way_points,
                        org_wp.size())) {
    skip(10 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  wp_vector::iterator it;
  for (it = org_wp.begin(); it < org_wp.end(); it++) {
    const Waypoint *wp = GetWayPoint(*it, way_points);
    TestWinPilotWayPoint(*it, wp);
  }
}

static void
TestSeeYouWayPoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(6, 0, "waypoint not found");
    return;
  }

  ok1(wp->Type == org_wp.Type);
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  // No home waypoints in a SeeYou file
  //ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
  ok1(wp->Comment == org_wp.Comment);
  ok1(wp->RunwayLength == org_wp.RunwayLength);
  ok1(wp->RunwayDirection == org_wp.RunwayDirection);
}

static void
TestSeeYou(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.cup"), way_points,
                        org_wp.size())) {
    skip(9 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  wp_vector::iterator it;
  for (it = org_wp.begin(); it < org_wp.end(); it++) {
    const Waypoint *wp = GetWayPoint(*it, way_points);
    TestSeeYouWayPoint(*it, wp);
  }
}

static void
TestZanderWayPoint(const Waypoint org_wp, const Waypoint *wp)
{
  if (wp == NULL) {
    skip(7, 0, "waypoint not found");
    return;
  }

  ok1(wp->Type == ((!org_wp.is_landable()) ? wtNormal : org_wp.Type));
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
}

static void
TestZander(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.wpz"), way_points,
                        org_wp.size())) {
    skip(10 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  wp_vector::iterator it;
  for (it = org_wp.begin(); it < org_wp.end(); it++) {
    if (it->Name.length() > 12)
      it->Name = it->Name.erase(12);
    trim_inplace(it->Name);
    const Waypoint *wp = GetWayPoint(*it, way_points);
    TestZanderWayPoint(*it, wp);
  }
}

static void
TestFS(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints_geo.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  wp_vector::iterator it;
  for (it = org_wp.begin(); it < org_wp.end(); it++) {
    if (it->Name.length() > 8)
      it->Name = it->Name.erase(8);
    trim_inplace(it->Name);
    GetWayPoint(*it, way_points);
  }
}

static void
TestFS_UTM(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints_utm.wpt"), way_points,
                        org_wp.size())) {
    skip(3 * org_wp.size(), 0, "opening waypoint file failed");
    return;
  }

  wp_vector::iterator it;
  for (it = org_wp.begin(); it < org_wp.end(); it++) {
    if (it->Name.length() > 8)
      it->Name = it->Name.erase(8);
    trim_inplace(it->Name);
    GetWayPoint(*it, way_points);
  }
}

static wp_vector
CreateOriginalWaypoints()
{
  wp_vector org_wp;

  GeoPoint loc;

  // Bergneustadt
  loc.Latitude = Angle::degrees(fixed(51.051944444444445));
  loc.Longitude = Angle::degrees(fixed(7.7061111111111114));

  Waypoint wp(loc);
  wp.Altitude = fixed(488);
  wp.Name = _T("Bergneustadt");
  wp.Comment = _T("123.650 040° 590m Rabbit holes, 20\" ditch south end of rwy");
  wp.RunwayDirection = Angle::degrees(fixed(40));
  wp.RunwayLength = 590;

  wp.Type = wtAirfield;
  wp.Flags.TurnPoint = true;
  wp.Flags.Home = true;
  wp.Flags.StartPoint = false;
  wp.Flags.FinishPoint = false;
  org_wp.push_back(wp);

  // Aconcagua
  loc.Latitude = Angle::dms(fixed(32), fixed(39), fixed(12)).flipped();
  loc.Longitude = Angle::dms(fixed(70), fixed(0), fixed(42)).flipped();

  Waypoint wp2(loc);
  wp2.Altitude = fixed(6962);
  wp2.Name = _T("Aconcagua");
  wp2.Comment = _T("Highest mountain in south-america");
  wp2.RunwayDirection = Angle::degrees(fixed(-1));
  wp2.RunwayLength = 0;

  wp2.Type = wtMountainTop;
  wp2.Flags.TurnPoint = true;
  wp2.Flags.Home = false;
  wp2.Flags.StartPoint = false;
  wp2.Flags.FinishPoint = false;
  org_wp.push_back(wp2);

  // Golden Gate Bridge
  loc.Latitude = Angle::dms(fixed(37), fixed(49), fixed(3));
  loc.Longitude = Angle::dms(fixed(122), fixed(28), fixed(42)).flipped();

  Waypoint wp3(loc);
  wp3.Altitude = fixed(227);
  wp3.Name = _T("Golden Gate Bridge");
  wp3.Comment = _T("");
  wp3.RunwayDirection = Angle::degrees(fixed(-1));
  wp3.RunwayLength = 0;

  wp3.Type = wtBridge;
  wp3.Flags.TurnPoint = true;
  wp3.Flags.Home = false;
  wp3.Flags.StartPoint = false;
  wp3.Flags.FinishPoint = false;
  org_wp.push_back(wp3);

  // Red Square
  loc.Latitude = Angle::dms(fixed(55), fixed(45), fixed(15));
  loc.Longitude = Angle::dms(fixed(37), fixed(37), fixed(12));

  Waypoint wp4(loc);
  wp4.Altitude = fixed(123);
  wp4.Name = _T("Red Square");
  wp4.Comment = _T("90° 0.01ml");
  wp4.RunwayDirection = Angle::degrees(fixed(90));
  wp4.RunwayLength = Units::ToSysUnit(fixed(0.01), unStatuteMiles);

  wp4.Type = wtOutlanding;
  wp4.Flags.TurnPoint = true;
  wp4.Flags.Home = false;
  wp4.Flags.StartPoint = false;
  wp4.Flags.FinishPoint = false;
  org_wp.push_back(wp4);

  // Sydney Opera
  loc.Latitude = Angle::dms(fixed(33), fixed(51), fixed(25)).flipped();
  loc.Longitude = Angle::dms(fixed(151), fixed(12), fixed(55));

  Waypoint wp5(loc);
  wp5.Altitude = fixed(5);
  wp5.Name = _T("Sydney Opera");
  wp5.Comment = _T("");
  wp5.RunwayDirection = Angle::degrees(fixed(-1));
  wp5.RunwayLength = 0;

  wp5.Type = wtNormal;
  wp5.Flags.TurnPoint = true;
  wp5.Flags.Home = false;
  wp5.Flags.StartPoint = false;
  wp5.Flags.FinishPoint = false;
  org_wp.push_back(wp5);

  return org_wp;
}

int main(int argc, char **argv)
{
  wp_vector org_wp = CreateOriginalWaypoints();

  plan_tests(63 + 5 * 4 + (9 + 10 + 8 + 3 + 3) * org_wp.size());

  TestExtractParameters();

  TestWinPilot(org_wp);
  TestSeeYou(org_wp);
  TestZander(org_wp);
  TestFS(org_wp);
  TestFS_UTM(org_wp);

  return exit_status();
}
