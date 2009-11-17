/*
Copyright_License {

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

#include "GlideComputerTask.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Math/FastMath.h"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/Screen.hpp"
#include "McReady.h"
#include "GlideSolvers.hpp"
#include "Message.h"
#include "Components.hpp"
#include "WayPointList.hpp"

int
GlideComputerTask::CalculateWaypointApproxDistance(const POINT &screen,
                                                   const WAYPOINT &way_point) {

  // Do preliminary fast search, by converting to screen coordinates
  POINT ws;
  LatLon2Flat(way_point.Location, ws);
  return Distance(ws, screen);
}

double
GlideComputerTask::CalculateWaypointArrivalAltitude(const WAYPOINT &way_point,
                                                    WPCALC &calc)
{
  double AltReqd;
  double wDistance, wBearing;

  DistanceBearing(Basic().Location, way_point.Location,
                  &wDistance, &wBearing);

  AltReqd = GlidePolar::MacCreadyAltitude
    (GlidePolar::AbortSafetyMacCready(),
     wDistance,
     wBearing,
     Calculated().WindSpeed,
     Calculated().WindBearing,
     0,
     0,
     true,
     NULL);

  calc.Distance = wDistance;
  calc.Bearing = wBearing;
  calc.AltReqd = AltReqd;

  return ((Calculated().NavAltitude) - AltReqd - way_point.Altitude -
	  SettingsComputer().SAFETYALTITUDEARRIVAL);
}

void
GlideComputerTask::SortLandableWaypoints()
{
  int SortedLandableIndex[MAXTASKPOINTS];
  double SortedArrivalAltitude[MAXTASKPOINTS];
  int SortedApproxDistance[MAXTASKPOINTS*2];
  int SortedApproxIndex[MAXTASKPOINTS*2];
  int i, k, l;
  double arrival_altitude;
  unsigned active_waypoint_on_entry;

  active_waypoint_on_entry = task.getActiveIndex();

  // Do preliminary fast search
  POINT sc_aircraft;
  LatLon2Flat(Basic().Location, sc_aircraft);

  // Clear search lists
  for (i=0; i<MAXTASKPOINTS*2; i++) {
    SortedApproxIndex[i]= -1;
    SortedApproxDistance[i] = 0;
  }

  for (i = 0; way_points.verify_index(i); i++) {
    const WAYPOINT &way_point = way_points.get(i);

    if (!(((way_point.Flags & AIRPORT) == AIRPORT) ||
          ((way_point.Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    int approx_distance =
      CalculateWaypointApproxDistance(sc_aircraft, way_point);

    // see if this fits into slot
    for (k=0; k< MAXTASKPOINTS*2; k++)  {

      if (((approx_distance < SortedApproxDistance[k])
           // wp is closer than this one
          || (SortedApproxIndex[k]== -1))   // or this one isn't filled
          && (SortedApproxIndex[k]!= i))    // and not replacing with same
        {
            // ok, got new biggest, put it into the slot.
          for (l=MAXTASKPOINTS*2-1; l>k; l--) {
            if (l>0) {
                SortedApproxDistance[l] = SortedApproxDistance[l-1];
                SortedApproxIndex[l] = SortedApproxIndex[l-1];
            }
          }

          SortedApproxDistance[k] = approx_distance;
          SortedApproxIndex[k] = i;
          k=MAXTASKPOINTS*2;
        }
    }
  }

  // Now do detailed search
  for (i=0; i<MAXTASKPOINTS; i++) {
    SortedLandableIndex[i]= -1;
    SortedArrivalAltitude[i] = 0;
  }

  bool found_reachable_airport = false;

  for (int scan_airports_slot=0;
       scan_airports_slot<2;
       scan_airports_slot++) {

    if (found_reachable_airport) {
      continue; // don't bother filling the rest of the list
    }

    for (i=0; i<MAXTASKPOINTS*2; i++) {
      if (SortedApproxIndex[i]<0) { // ignore invalid points
        continue;
      }

      const WAYPOINT &way_point = way_points.get(SortedApproxIndex[i]);

      if ((scan_airports_slot==0) &&
	  ((way_point.Flags & AIRPORT) != AIRPORT)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude =
        CalculateWaypointArrivalAltitude(way_point,
                                         way_points.set_calc(SortedApproxIndex[i]));

      if (scan_airports_slot==0) {
        if (arrival_altitude<0) {
          // in first scan, this airport is unreachable, so ignore it.
          continue;
        } else {
          // this airport is reachable
          found_reachable_airport = true;
        }
      }

      // see if this fits into slot
      for (k=0; k< MAXTASKPOINTS; k++) {
        if (((arrival_altitude > SortedArrivalAltitude[k])
             // closer than this one
             ||(SortedLandableIndex[k]== -1))
            // or this one isn't filled
             &&(SortedLandableIndex[k]!= i))  // and not replacing
                                              // with same
          {

            double wp_distance, wp_bearing;
            DistanceBearing(Basic().Location ,
                            way_point.Location,
                            &wp_distance, &wp_bearing);

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, &Basic(), &Calculated(),
				       SettingsComputer(),
                                       NULL,
                                       wp_distance,
                                       &out_of_range, NULL);

            if ((distance_soarable>= wp_distance)||(arrival_altitude<0)) {
              // only put this in the index if it is reachable
              // and doesn't go through terrain, OR, if it is unreachable
              // it doesn't matter if it goes through terrain because
              // pilot has to climb first anyway

              // ok, got new biggest, put it into the slot.
              for (l=MAXTASKPOINTS-1; l>k; l--) {
                if (l>0) {
                  SortedArrivalAltitude[l] = SortedArrivalAltitude[l-1];
                  SortedLandableIndex[l] = SortedLandableIndex[l-1];
                }
              }

              SortedArrivalAltitude[k] = arrival_altitude;
              SortedLandableIndex[k] = SortedApproxIndex[i];
              k=MAXTASKPOINTS;
            }
          }
      }
    }
  }

  // now we have a sorted list.
  // check if current waypoint or home waypoint is in the sorted list
  int found_active_waypoint = -1;
  int found_home_waypoint = -1;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (task.Valid()) {
      if (SortedLandableIndex[i] == task.getWaypointIndex()) {
        found_active_waypoint = i;
      }
    }
    if ((SettingsComputer().HomeWaypoint>=0)
	&& (SortedLandableIndex[i] == SettingsComputer().HomeWaypoint)) {
      found_home_waypoint = i;
    }
  }

  if ((found_home_waypoint == -1)
      &&(way_points.verify_index(SettingsComputer().HomeWaypoint))) {
    // home not found in top list, so see if we can sneak it in

    arrival_altitude =
      CalculateWaypointArrivalAltitude(way_points.get(SettingsComputer().HomeWaypoint),
                                       way_points.set_calc(SettingsComputer().HomeWaypoint));
    if (arrival_altitude>0) {
      // only put it in if reachable
      SortedLandableIndex[MAXTASKPOINTS-2] = SettingsComputer().HomeWaypoint;
    }
  }

  bool new_closest_waypoint = false;

  if (found_active_waypoint != -1) {
    task.setActiveIndex(found_active_waypoint);
  } else {
    // if not found, keep on field or set active waypoint to closest
    int wp_index = task.getWaypointIndex();
    if (wp_index>=0){
      arrival_altitude =
        CalculateWaypointArrivalAltitude(way_points.get(wp_index),
                                         way_points.set_calc(wp_index));
    } else {
      arrival_altitude = 0;
    }
    if (arrival_altitude <= 0){   // last active is no more reachable,
                                  // switch to new closest
      new_closest_waypoint = true;
      task.setActiveIndex(0);
    } else {
      // last active is reachable but not in list, add to end of
      // list (or overwrite laste one)
      for (i=0; i<MAXTASKPOINTS-1; i++) {     // find free slot
        if (SortedLandableIndex[i] == -1)     // free slot found (if
                                                // not, i index the
                                                // last entry of the
                                                // list)
            break;
      }
      SortedLandableIndex[i] = task.getWaypointIndex();
      task.setActiveIndex(i);
    }
  }

  int last_closest_waypoint=0;
  if (new_closest_waypoint) {
    last_closest_waypoint = task.getWaypointIndex(0);
  }

  task.setTaskIndices(SortedLandableIndex);
  task.RefreshTask(SettingsComputer());

  if (new_closest_waypoint) {
    if ((task.getWaypointIndex(0) != last_closest_waypoint)
        && task.ValidTaskPoint(0)) {
      double last_wp_distance= 10000.0;
      if (last_closest_waypoint>=0) {
        last_wp_distance = Distance(task.getTaskPointLocation(0),
                                    way_points.get(last_closest_waypoint).Location);
      }
      if (last_wp_distance>2000.0) {
        // don't display the message unless the airfield has moved by more
        // than 2 km
        Message::AddMessage(TEXT("Closest Airfield Changed!"));
      }
    }
  }

  if (active_waypoint_on_entry != task.getActiveIndex()){
    task.setSelected();
  }
}
