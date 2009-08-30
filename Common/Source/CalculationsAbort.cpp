/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Calculations.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Math/FastMath.h"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "McReady.h"
#include "GlideSolvers.hpp"
#include "StatusMessage.hpp"

//////////////////////////////////////////////////////////////////


void LatLon2Flat(double lon, double lat, int *scx, int *scy) {
  *scx = (int)(lon*fastcosine(lat)*100);
  *scy = (int)(lat*100);
}


int CalculateWaypointApproxDistance(int scx_aircraft, int scy_aircraft,
                                    int i) {

  // Do preliminary fast search, by converting to screen coordinates
  int sc_x, sc_y;
  LatLon2Flat(WayPointList[i].Longitude,
              WayPointList[i].Latitude, &sc_x, &sc_y);
  int dx, dy;
  dx = scx_aircraft-sc_x;
  dy = scy_aircraft-sc_y;

  return isqrt4(dx*dx+dy*dy);
}



double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic,
                                        DERIVED_INFO *Calculated,
                                        int i) {
  double AltReqd;
  double wDistance, wBearing;

  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  WayPointList[i].Latitude,
                  WayPointList[i].Longitude,
                  &wDistance, &wBearing);

  AltReqd = GlidePolar::MacCreadyAltitude
    (GlidePolar::AbortSafetyMacCready(),
     wDistance,
     wBearing,
     Calculated->WindSpeed,
     Calculated->WindBearing,
     0,
     0,
     true,
     NULL);

  WayPointCalc[i].Distance = wDistance; // VENTA3
  WayPointCalc[i].Bearing  = wBearing; // VENTA3
  WayPointCalc[i].AltReqd  = AltReqd;  // VENTA3

  return ((Calculated->NavAltitude) - AltReqd
          - WayPointList[i].Altitude - SAFETYALTITUDEARRIVAL);
}



void SortLandableWaypoints(NMEA_INFO *Basic,
                           DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXTASKPOINTS];
  double SortedArrivalAltitude[MAXTASKPOINTS];
  int SortedApproxDistance[MAXTASKPOINTS*2];
  int SortedApproxIndex[MAXTASKPOINTS*2];
  int i, k, l;
  double arrival_altitude;
  int active_waypoint_on_entry;

  if (!WayPointList) return;

  //  LockFlightData();
  mutexTaskData.Lock();
  active_waypoint_on_entry = ActiveWayPoint;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  // Clear search lists
  for (i=0; i<MAXTASKPOINTS*2; i++) {
    SortedApproxIndex[i]= -1;
    SortedApproxDistance[i] = 0;
  }

  for (i=0; i<(int)NumberOfWayPoints; i++) {
    if (!(((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
          ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    int approx_distance =
      CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, i);

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

      if ((scan_airports_slot==0) &&
	  ((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic,
                                         Calculated,
                                         SortedApproxIndex[i]);

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
            DistanceBearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude,
                            &wp_distance, &wp_bearing);

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, Basic, Calculated,
                                       NULL,
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
    if (ValidTaskPoint(ActiveWayPoint)) {
      if (SortedLandableIndex[i] == Task[ActiveWayPoint].Index) {
        found_active_waypoint = i;
      }
    }
    if ((HomeWaypoint>=0) && (SortedLandableIndex[i] == HomeWaypoint)) {
      found_home_waypoint = i;
    }
  }

  if ((found_home_waypoint == -1)&&(HomeWaypoint>=0)) {
    // home not found in top list, so see if we can sneak it in

    arrival_altitude = CalculateWaypointArrivalAltitude(Basic,
                                                        Calculated,
                                                        HomeWaypoint);
    if (arrival_altitude>0) {
      // only put it in if reachable
      SortedLandableIndex[MAXTASKPOINTS-2] = HomeWaypoint;
    }
  }

  bool new_closest_waypoint = false;

  if (found_active_waypoint != -1) {
    ActiveWayPoint = found_active_waypoint;
  } else {
    // if not found, keep on field or set active waypoint to closest
    if (ValidTaskPoint(ActiveWayPoint)){
      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic, Calculated,
                                         Task[ActiveWayPoint].Index);
    } else {
      arrival_altitude = 0;
    }
    if (arrival_altitude <= 0){   // last active is no more reachable,
                                  // switch to new closest
      new_closest_waypoint = true;
      ActiveWayPoint = 0;
    } else {
      // last active is reachable but not in list, add to end of
      // list (or overwrite laste one)
      if (ActiveWayPoint>=0){
        for (i=0; i<MAXTASKPOINTS-1; i++) {     // find free slot
          if (SortedLandableIndex[i] == -1)     // free slot found (if
                                                // not, i index the
                                                // last entry of the
                                                // list)
            break;
        }
        SortedLandableIndex[i] = Task[ActiveWayPoint].Index;
        ActiveWayPoint = i;
      }
    }
  }

  // set new waypoints in task

  for (i=0; i<(int)NumberOfWayPoints; i++) {
    WayPointList[i].InTask = false;
  }

  int last_closest_waypoint=0;
  if (new_closest_waypoint) {
    last_closest_waypoint = Task[0].Index;
  }

  for (i=0; i<MAXTASKPOINTS; i++){
    Task[i].Index = SortedLandableIndex[i];
    if (ValidTaskPoint(i)) {
      WayPointList[Task[i].Index].InTask = true;
    }
  }

  if (new_closest_waypoint) {
    if ((Task[0].Index != last_closest_waypoint) && ValidTaskPoint(0)) {
      double last_wp_distance= 10000.0;
      if (last_closest_waypoint>=0) {
        DistanceBearing(WayPointList[Task[0].Index].Latitude,
                        WayPointList[Task[0].Index].Longitude,
                        WayPointList[last_closest_waypoint].Latitude,
                        WayPointList[last_closest_waypoint].Longitude,
                        &last_wp_distance, NULL);
      }
      if (last_wp_distance>2000.0) {
        // don't display the message unless the airfield has moved by more
        // than 2 km
        AddStatusMessage(TEXT("Closest Airfield Changed!"));
      }

    }
  }

  if (EnableMultipleStartPoints) {
    for (i=0; i<MAXSTARTPOINTS; i++) {
      if (StartPoints[i].Active && (ValidWayPoint(StartPoints[i].Index))) {
        WayPointList[StartPoints[i].Index].InTask = true;
      }
    }
  }

  if (active_waypoint_on_entry != ActiveWayPoint){
    SelectedWaypoint = ActiveWayPoint;
  }
  mutexTaskData.Unlock();
  //  UnlockFlightData();
}
