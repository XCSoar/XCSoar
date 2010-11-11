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

short
RasterMap::GetField(const GeoPoint &location) const
{
  return 0;
}

static bool
TestWayPointFile(const TCHAR* filename, Waypoints &way_points)
{
  WayPointFile *f = WayPointFile::create(filename, 0);
  if (!ok1(f != NULL)) {
    skip(3, 0, "opening waypoint file failed");
    return false;
  }

  bool success = f->Parse(way_points, NULL);
  ok1(success);
  delete f;

  way_points.optimise();

  ok1(!way_points.empty());
  ok1(way_points.size() == 1);

  return success;
}

static void
TestWinPilot()
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.dat"), way_points)) {
    skip(11, 0, "opening waypoint file failed");
    return;
  }

  const Waypoint *wp = way_points.lookup_name(_T("Bergneustadt"));
  if (!ok1(wp != NULL)) {
    skip(10, 0, "waypoint not found");
    return;
  }
  ok1(equals(wp->Location.Longitude, 7.7061111111111114));
  ok1(equals(wp->Location.Latitude, 51.051944444444445));
  ok1(equals(wp->Altitude, 488));

  ok1(wp->Flags.Airport);
  ok1(wp->Flags.TurnPoint);
  ok1(!wp->Flags.LandPoint);
  ok1(wp->Flags.Home);
  ok1(!wp->Flags.StartPoint);
  ok1(!wp->Flags.FinishPoint);
  ok1(!wp->Flags.Restricted);
}

static void
TestSeeYou()
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.cup"), way_points)) {
    skip(10, 0, "opening waypoint file failed");
    return;
  }

  const Waypoint *wp = way_points.lookup_name(_T("Bergneustadt"));
  if (!ok1(wp != NULL)) {
    skip(9, 0, "waypoint not found");
    return;
  }
  ok1(equals(wp->Location.Longitude, 7.7061111111111114));
  ok1(equals(wp->Location.Latitude, 51.051944444444445));
  ok1(equals(wp->Altitude, 488));

  ok1(wp->Flags.Airport);
  ok1(wp->Flags.TurnPoint);
  ok1(!wp->Flags.LandPoint);
  // No home waypoints in a SeeYou file
  //ok1(wp->Flags.Home);
  ok1(!wp->Flags.StartPoint);
  ok1(!wp->Flags.FinishPoint);
  ok1(!wp->Flags.Restricted);
}

static void
TestZander()
{
  Waypoints way_points;
  if (!TestWayPointFile(_T("test/data/waypoints.wpz"), way_points)) {
    skip(11, 0, "opening waypoint file failed");
    return;
  }

  const Waypoint *wp = way_points.lookup_name(_T("Bergneustadt"));
  if (!ok1(wp != NULL)) {
    skip(10, 0, "waypoint not found");
    return;
  }
  ok1(equals(wp->Location.Longitude, 7.7061111111111114));
  ok1(equals(wp->Location.Latitude, 51.051944444444445));
  ok1(equals(wp->Altitude, 488));

  ok1(wp->Flags.Airport);
  ok1(wp->Flags.TurnPoint);
  ok1(!wp->Flags.LandPoint);
  ok1(wp->Flags.Home);
  ok1(!wp->Flags.StartPoint);
  ok1(!wp->Flags.FinishPoint);
  ok1(!wp->Flags.Restricted);
}

int main(int argc, char **argv)
{
  plan_tests(44);

  TestWinPilot();
  TestSeeYou();
  TestZander();

  return exit_status();
}
