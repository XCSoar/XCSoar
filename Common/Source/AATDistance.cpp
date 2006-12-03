/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

        M Roberts (original release)
        Robin Birch <robinb@ruffnready.co.uk>
        Samuel Gisiger <samuel.gisiger@triadis.ch>
        Jeff Goodenough <jeff@enborne.f2s.com>
        Alastair Harrison <aharrison@magic.force9.co.uk>
        Scott Penrose <scottp@dd.com.au>
        John Wharington <jwharington@bigfoot.com>

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
  Foundation, Inc., 59 Temple Placel - Suite 330, Boston, MA  02111-1307, USA.
}

*/

#include "stdafx.h"
#include "AATDistance.h"
#include "Task.h"
#include "Airspace.h"
#include "XCSoar.h"
#include "Calculations.h"
#include "externs.h"

#define DISTANCETHRESHOLD 500

AATDistance::AATDistance() {
  Reset();
}


void AATDistance::Reset() {
  int i, j;

  max_achieved_distance=0;

  for (i=0; i<MAXTASKPOINTS; i++) {
    imax[i]= 0;
    distancethreshold[i]= DISTANCETHRESHOLD;
    num_points[i]=0;
    for (j=0; j<MAXNUM_AATDISTANCE; j++) {
      Dmax[i][j]=0;
      lat_points[i][j]=0;
      lon_points[i][j]=0;
    }
  }
}


void AATDistance::AddPoint(double longitude, double latitude,
                           int taskwaypoint) {

  // should only add ONE point to start.
  // If restart, need to reset

  if (num_points[taskwaypoint]<MAXNUM_AATDISTANCE) {

    int n = num_points[taskwaypoint];

    bool new_point= false;

    if (n>1) {
      double dist;
      DistanceBearing(lat_points[taskwaypoint][n-2],
                      lon_points[taskwaypoint][n-2],
                      latitude,
                      longitude, &dist, NULL);
      if ((dist>distancethreshold[taskwaypoint])||(n==1)) {
        new_point = true;
      }
    } else {
      new_point = true;
    }

    if (new_point) {
      if (taskwaypoint>0) {
        num_points[taskwaypoint]++;
      } else {
        // just replace current start
        num_points[taskwaypoint]= 1;
      }
      if (num_points[taskwaypoint]==MAXNUM_AATDISTANCE) {
        ThinData(taskwaypoint);
      }
    }

    // always replace last point
    lat_points[taskwaypoint][max(0,num_points[taskwaypoint]-1)]= latitude;
    lon_points[taskwaypoint][max(0,num_points[taskwaypoint]-1)]= longitude;

    // rescan max so can adjust targets as required
    DistanceCovered_internal(longitude, latitude,
                             taskwaypoint,
                             (ActiveWayPoint==taskwaypoint));

    if (taskwaypoint>0) {
      for (int i=taskwaypoint; i<MAXTASKPOINTS-1; i++) {
        UpdateSearch(i);
      }
    }
  }

}

double AATDistance::DistanceCovered_internal(double longitude,
                                             double latitude,
                                             int taskwaypoint,
                                             bool insector) {

  // find maximum distance from start to this location,
  // projected along target

  if (taskwaypoint==0) {
    return 0.0;
  }

  double target_distance, best_target_distance;
  double projected_distance, achieved_distance, best_achieved_distance;
  bool best_update_target = false;

  best_target_distance = 0;
  best_achieved_distance = 0;
  achieved_distance = 0;

  if (taskwaypoint>0) {
    int nlast = num_points[taskwaypoint-1];

    double w1lat = Task[taskwaypoint].AATTargetLat;
    double w1lon = Task[taskwaypoint].AATTargetLon;

    for (int j=0; j<nlast; j++) {

      bool update_target = false;

      double w0lon = lon_points[taskwaypoint-1][j];
      double w0lat = lat_points[taskwaypoint-1][j];

      double d01;
      DistanceBearing(w0lat, w0lon,
                      w1lat, w1lon, &d01, NULL);

      target_distance = Dmax[taskwaypoint-1][j] + d01;

      projected_distance = Dmax[taskwaypoint-1][j]
        + ProjectedDistance(w0lon, w0lat,
                            w1lon, w1lat,
                            longitude,
                            latitude);

      if (insector) {

        int nthis = num_points[taskwaypoint];

        // TODO: check all points in this sector as some existing
        // ones may be better than the current target

        for (int k=-1; k<nthis; k++) {

          double lat0;
          double lon0;

          if (k== -1) {
            lat0 = latitude;
            lon0 = longitude;
          } else {
            lat0 = lat_points[taskwaypoint][k];
            lon0 = lon_points[taskwaypoint][k];
          }

          double dtmp, bearing;
          DistanceBearing(w0lat, w0lon, lat0, lon0, &dtmp, &bearing);
          achieved_distance = Dmax[taskwaypoint-1][j] + dtmp;

          if (achieved_distance>target_distance) {
            update_target = true;
            target_distance = achieved_distance;

            // move current target to obtain best distance
            // (this is new minimum)
            // TODO: shuffle following targets to maintain current
            // distance setting

            Task[taskwaypoint].AATTargetLat = lat0;
            Task[taskwaypoint].AATTargetLon = lon0;
            Task[taskwaypoint].AATTargetOffsetRadial = bearing;
            w1lat = Task[taskwaypoint].AATTargetLat;
            w1lon = Task[taskwaypoint].AATTargetLon;

          }
        }

      } else {

        achieved_distance = projected_distance;

      }

      if (target_distance> best_target_distance) {
        best_target_distance = target_distance;
        best_achieved_distance = achieved_distance;
        best_update_target = update_target;

        if (taskwaypoint>1) {

          // move previous target to obtain best distance
          Task[taskwaypoint-1].AATTargetLat= w0lat;
          Task[taskwaypoint-1].AATTargetLon= w0lon;

          //          CalculateAATTaskSectors(taskwaypoint);
        }
      }
    }
  }

  if ((taskwaypoint<MAXTASKPOINTS-1)
      &&(Task[taskwaypoint+1].Index>=0)) {

    if (insector && (taskwaypoint==ActiveWayPoint)) {

      double bearing;

      if ((best_achieved_distance<max_achieved_distance)
          || (num_points[taskwaypoint]==1) || 1) {
        // best is decreasing, so project target in direction of improvement
        // or first entry into sector

        double d_tar;
        double d_this;

        DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                        Task[taskwaypoint-1].AATTargetLon,
                        Task[taskwaypoint].AATTargetLat,
                        Task[taskwaypoint].AATTargetLon,
                        &d_tar, NULL);

        DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                        Task[taskwaypoint-1].AATTargetLon,
                        latitude,
                        longitude,
                        &d_this, &bearing);

        double d_diff = d_tar-d_this;

        if (d_diff>0) {

          // move target in line with previous target along track
          // at an offset to improve on max distance

          Task[taskwaypoint].AATTargetOffsetRadial = bearing;

          do {

            FindLatitudeLongitude(latitude, longitude,
                                  bearing, d_diff,
                                  &Task[taskwaypoint].AATTargetLat,
                                  &Task[taskwaypoint].AATTargetLon);

            d_diff -= 100.0;

          } while ((!InAATTurnSector(Task[taskwaypoint].AATTargetLon,
                                     Task[taskwaypoint].AATTargetLat,
                                     taskwaypoint)) && (d_diff>0.0));

          // check if this point is inside sector, if not,
          // vector the aircraft to the next waypoint since no improvement
          // is possible

          if (d_diff<0.0) {

            DistanceBearing(latitude,
                            longitude,
                            WayPointList[Task[taskwaypoint+1].Index].Latitude,
                            WayPointList[Task[taskwaypoint+1].Index].Longitude,
                            NULL, &bearing);

            FindLatitudeLongitude(latitude, longitude,
                                  bearing, 100.0,
                                  &Task[taskwaypoint].AATTargetLat,
                                  &Task[taskwaypoint].AATTargetLon);
          }

          // Also update Task range for this waypoint based on percentage
          // to edge of sector.  In AdjustAAT, if in sector, adjust target
          // from current aircraft position (-100%) to the extremity (100%)
          // but must be careful not to move target behind what is already
          // achieved.

        } else {

          // ERROR!
          d_diff = 0;

        }

      } else if (best_achieved_distance>= best_target_distance) {

        // this point is in sector and is improved

        // JMW, now moves target to in line with previous target whenever
        // you are in AAT sector and improving on the target distance

        DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                        Task[taskwaypoint-1].AATTargetLon,
                        latitude,
                        longitude,
                        NULL, &bearing);

        FindLatitudeLongitude(latitude, longitude,
                              bearing, 100.0,
                              &Task[taskwaypoint].AATTargetLat,
                              &Task[taskwaypoint].AATTargetLon);

        Task[taskwaypoint].AATTargetOffsetRadial = bearing;

      }
      /* if no improvement possible, vector to outside
        {

        DistanceBearing(latitude,
                        longitude,
                        WayPointList[Task[taskwaypoint+1].Index].Latitude,
                        WayPointList[Task[taskwaypoint+1].Index].Longitude,
                        NULL, &bearing);

        FindLatitudeLongitude(latitude, longitude,
                              bearing, 100.0,
                              &Task[taskwaypoint].AATTargetLat,
                              &Task[taskwaypoint].AATTargetLon);

        Task[taskwaypoint].AATTargetOffsetRadial = bearing;

        // Move previous target to location that yields longest distance,
        // plus a little so optimal path vector points to next waypoint.

        }
      */

    } /* else if (best_update_target) {

      redundant code

      Task[taskwaypoint].AATTargetLat= latitude;
      Task[taskwaypoint].AATTargetLon= longitude;

      }  */
  }

  max_achieved_distance = max(best_achieved_distance, max_achieved_distance);
  return best_achieved_distance;
}



double AATDistance::DistanceCovered(double longitude,
                                    double latitude,
                                    int taskwaypoint) {
  return DistanceCovered_internal(longitude, latitude,
                                  taskwaypoint,
                                  false);
}


void AATDistance::UpdateSearch(int taskwaypoint) {
  int n = num_points[taskwaypoint];
  int nlast = num_points[taskwaypoint-1];
  double dist, distbest;
  int ibest;
  int j, k;

  if ((n==0)||(nlast==0)) {
    // nothing to do.
    return;
  }

  for (j=0; j<n; j++) {
    Dmax[taskwaypoint][j]=0;

    distbest = 0;
    ibest = 0;

    for (k=0; k<nlast; k++) {
      double dtmp;
      DistanceBearing(lat_points[taskwaypoint-1][k],
                      lon_points[taskwaypoint-1][k],
                      lat_points[taskwaypoint][j],
                      lon_points[taskwaypoint][j], &dtmp, NULL);

      dist = Dmax[taskwaypoint-1][k]+dtmp;

      if (dist>distbest) {
        distbest = dist;
        ibest = k;
      }
    }
    imax[taskwaypoint-1]= ibest;
    Dmax[taskwaypoint][j]= distbest;
  }
}


void AATDistance::ThinData(int taskwaypoint) {
  double contractfactor = 0.8;
  bool do_delete[MAXNUM_AATDISTANCE];

  int i;
  for (i=0; i<MAXNUM_AATDISTANCE; i++) {
    do_delete[i]= false;
  }

  while (num_points[taskwaypoint]> MAXNUM_AATDISTANCE*contractfactor) {
    distancethreshold[taskwaypoint] /= contractfactor;

    for (i= num_points[taskwaypoint]-1; i>0; i--) {

      double d;
      DistanceBearing(lat_points[taskwaypoint][i],
                      lon_points[taskwaypoint][i],
                      lat_points[taskwaypoint][i-1],
                      lon_points[taskwaypoint][i-1], &d, NULL);
      if (d<distancethreshold[taskwaypoint]) {
	do_delete[i] = true; // mark it for deletion
      }
    }

    // now shuffle points along
    int j;
    i = 0; j = i;

    int pnts_in_new;
    while (j< num_points[taskwaypoint]) {
      if (!do_delete[j]) {
	lat_points[taskwaypoint][i] = lat_points[taskwaypoint][j];
	lon_points[taskwaypoint][i] = lon_points[taskwaypoint][j];
	i++;
	pnts_in_new = i;
      }
      j++;
    }
    num_points[taskwaypoint] = pnts_in_new;
  }
  if (num_points[taskwaypoint]>=MAXNUM_AATDISTANCE) {
    // error!
    num_points[taskwaypoint]=MAXNUM_AATDISTANCE-1;
  }

}
