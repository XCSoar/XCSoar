/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "TestUtil.hpp"
#include "tstring.hpp"

#include <vector>

typedef std::vector<Waypoint> wp_vector;

short
RasterMap::GetField(const GeoPoint &location) const
{
  return 0;
}

static bool
TestWayPointFile(const TCHAR* filename, Waypoints &way_points, unsigned num_wps)
{
  WayPointFile *f = WayPointFile::create(filename, 0);
  if (!ok1(f != NULL)) {
    skip(3, 0, "opening waypoint file failed");
    return false;
  }

  if(!ok1(f->Parse(way_points, NULL))) {
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
    skip(3, 0, "waypoint not found");
    return NULL;
  }
  ok1(equals(wp->Location.Longitude, org_wp.Location.Longitude));
  ok1(equals(wp->Location.Latitude, org_wp.Location.Latitude));
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

  ok1(wp->Flags.Airport == org_wp.Flags.Airport);
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  ok1(wp->Flags.LandPoint == org_wp.Flags.LandPoint);
  ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
  ok1(wp->Flags.Restricted == org_wp.Flags.Restricted);
}

static void
TestWinPilot(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.dat"), way_points,
                        org_wp.size())) {
    skip(11 * org_wp.size(), 0, "opening waypoint file failed");
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

  ok1(wp->Flags.Airport == org_wp.Flags.Airport);
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  ok1(wp->Flags.LandPoint == org_wp.Flags.LandPoint);
  // No home waypoints in a SeeYou file
  //ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
  ok1(wp->Flags.Restricted == org_wp.Flags.Restricted);
}

static void
TestSeeYou(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.cup"), way_points,
                        org_wp.size())) {
    skip(10 * org_wp.size(), 0, "opening waypoint file failed");
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

  ok1(wp->Flags.Airport == org_wp.Flags.Airport);
  ok1(wp->Flags.TurnPoint == org_wp.Flags.TurnPoint);
  ok1(wp->Flags.LandPoint == org_wp.Flags.LandPoint);
  ok1(wp->Flags.Home == org_wp.Flags.Home);
  ok1(wp->Flags.StartPoint == org_wp.Flags.StartPoint);
  ok1(wp->Flags.FinishPoint == org_wp.Flags.FinishPoint);
  ok1(wp->Flags.Restricted == org_wp.Flags.Restricted);
}

static void
TestZander(wp_vector org_wp)
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.wpz"), way_points,
                        org_wp.size())) {
    skip(11 * org_wp.size(), 0, "opening waypoint file failed");
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

  wp.Flags.Airport = true;
  wp.Flags.TurnPoint = true;
  wp.Flags.LandPoint = false;
  wp.Flags.Home = true;
  wp.Flags.StartPoint = false;
  wp.Flags.FinishPoint = false;
  wp.Flags.Restricted = false;
  org_wp.push_back(wp);

  // Aconcagua
  loc.Latitude = Angle::dms(fixed(32), fixed(39), fixed(12)).flipped();
  loc.Longitude = Angle::dms(fixed(70), fixed(0), fixed(42)).flipped();

  Waypoint wp2(loc);
  wp2.Altitude = fixed(6962);
  wp2.Name = _T("Aconcagua");

  wp2.Flags.Airport = false;
  wp2.Flags.TurnPoint = true;
  wp2.Flags.LandPoint = false;
  wp2.Flags.Home = false;
  wp2.Flags.StartPoint = false;
  wp2.Flags.FinishPoint = false;
  wp2.Flags.Restricted = false;
  org_wp.push_back(wp2);

  // Golden Gate Bridge
  loc.Latitude = Angle::dms(fixed(37), fixed(49), fixed(3));
  loc.Longitude = Angle::dms(fixed(122), fixed(28), fixed(42)).flipped();

  Waypoint wp3(loc);
  wp3.Altitude = fixed(227);
  wp3.Name = _T("Golden Gate Bridge");

  wp3.Flags.Airport = false;
  wp3.Flags.TurnPoint = true;
  wp3.Flags.LandPoint = false;
  wp3.Flags.Home = false;
  wp3.Flags.StartPoint = false;
  wp3.Flags.FinishPoint = false;
  wp3.Flags.Restricted = false;
  org_wp.push_back(wp3);

  // Red Square
  loc.Latitude = Angle::dms(fixed(55), fixed(45), fixed(15));
  loc.Longitude = Angle::dms(fixed(37), fixed(37), fixed(12));

  Waypoint wp4(loc);
  wp4.Altitude = fixed(123);
  wp4.Name = _T("Red Square");

  wp4.Flags.Airport = false;
  wp4.Flags.TurnPoint = true;
  wp4.Flags.LandPoint = true;
  wp4.Flags.Home = false;
  wp4.Flags.StartPoint = false;
  wp4.Flags.FinishPoint = false;
  wp4.Flags.Restricted = false;
  org_wp.push_back(wp4);

  // Sydney Opera
  loc.Latitude = Angle::dms(fixed(33), fixed(51), fixed(25)).flipped();
  loc.Longitude = Angle::dms(fixed(151), fixed(12), fixed(55));

  Waypoint wp5(loc);
  wp5.Altitude = fixed(5);
  wp5.Name = _T("Sydney Opera");

  wp5.Flags.Airport = false;
  wp5.Flags.TurnPoint = true;
  wp5.Flags.LandPoint = false;
  wp5.Flags.Home = false;
  wp5.Flags.StartPoint = false;
  wp5.Flags.FinishPoint = false;
  wp5.Flags.Restricted = false;
  org_wp.push_back(wp5);

  return org_wp;
}

int main(int argc, char **argv)
{
  wp_vector org_wp = CreateOriginalWaypoints();

  plan_tests(3 * 4 + (11 + 10 + 11) * org_wp.size());

  TestWinPilot(org_wp);
  TestSeeYou(org_wp);
  TestZander(org_wp);

  return exit_status();
}
