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

#include "AATDistance.h"
#include "StdAfx.h"
#include "Task.h"
#include "Airspace.h"
#include "XCSoar.h"
#include "Calculations.h"
#include "Math/Geometry.hpp"
#include "externs.h"
#include "Math/Earth.hpp"
#include "WayPoint.hpp"

#include <math.h>

#define DISTANCETHRESHOLD 500

extern NMEA_INFO GPS_INFO;

AATDistance::AATDistance() {
  Reset();
}


void AATDistance::Reset() {
  int i, j;

  max_achieved_distance=0;

  for (i=0; i<MAXTASKPOINTS; i++) {
    has_entered[i]= 0;
    legdistance_achieved[i]= 0;
    distancethreshold[i]= DISTANCETHRESHOLD;
    num_points[i]=0;
    best[i]=0;
    for (j=0; j<MAXNUM_AATDISTANCE; j++) {
      Dmax[i][j]=0;
      lat_points[i][j]=0;
      lon_points[i][j]=0;
      imax[i][j]=0;
    }
  }
}


double AATDistance::LegDistanceAchieved(int taskwaypoint) {
  double retval;
  LockTaskData();
  retval= legdistance_achieved[taskwaypoint];
  UnlockTaskData();
  return retval;
}

void AATDistance::ResetEnterTrigger(int taskwaypoint) {
  LockTaskData();
  has_entered[taskwaypoint] = false;
  UnlockTaskData();
}

void AATDistance::AddPoint(double longitude, double latitude,
                           int taskwaypoint) {
  if (taskwaypoint<0) return;

  bool was_entered = has_entered[taskwaypoint];
  has_entered[taskwaypoint] = true;

  if (!AATEnabled) return; // nothing else to do for non-AAT tasks

  LockTaskData();

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
      if (dist>distancethreshold[taskwaypoint]) {
        new_point = true;
      }
    } else {
      // first point in sector
      new_point = true;

      if ((!was_entered) && (taskwaypoint>0) &&
          !Task[taskwaypoint].AATTargetLocked) {
        double qdist, bearing0, bearing1;
        DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                        Task[taskwaypoint-1].AATTargetLon,
                        latitude,
                        longitude,
                        &qdist, &bearing0);
        DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                        Task[taskwaypoint-1].AATTargetLon,
                        Task[taskwaypoint].AATTargetLat,
                        Task[taskwaypoint].AATTargetLon,
                        &qdist, &bearing1);
        // JMWAAT
        Task[taskwaypoint].AATTargetOffsetRadial = 0.0;
        // 20080615 JMW
	// was AngleLimit180(bearing1-bearing0);
	// now project along track line
	// target will be moved by ShiftTargetFromBehind
      }

    }
    if (taskwaypoint==0) {
      // force updating of start point
      new_point = true;
    }

    if (new_point) {
      if (taskwaypoint>0) {
        num_points[taskwaypoint]++;
        if (num_points[taskwaypoint]==MAXNUM_AATDISTANCE) {
          ThinData(taskwaypoint);
        }
      } else {
        // just replace current start
        num_points[taskwaypoint]= 1;
      }
    }

    // always replace last point
    lat_points[taskwaypoint][max(0,num_points[taskwaypoint]-1)]= latitude;
    lon_points[taskwaypoint][max(0,num_points[taskwaypoint]-1)]= longitude;

    // update max search for this and future waypoints
    if (taskwaypoint>0) {
      for (int i= taskwaypoint; i<MAXTASKPOINTS-1; i++) {
        UpdateSearch(i);
      }
      if (taskwaypoint == ActiveWayPoint) {
        DistanceCovered_internal(longitude, latitude, true);
      }
    }
  }
  UnlockTaskData();

}


void AATDistance::ShiftTargetOutside(double longitude, double latitude,
                                    int taskwaypoint) {
  // if no improvement possible, vector to outside
  double bearing;

  if (taskwaypoint>0) {
    DistanceBearing(latitude,
                    longitude,
                    WayPointList[Task[taskwaypoint+1].Index].Latitude,
                    WayPointList[Task[taskwaypoint+1].Index].Longitude,
                    NULL, &bearing);

    FindLatitudeLongitude(latitude, longitude,
                          bearing, 100.0,
                          &Task[taskwaypoint].AATTargetLat,
                          &Task[taskwaypoint].AATTargetLon);
    TargetModified = true;

  }

  //JMWAAT  Task[taskwaypoint].AATTargetOffsetRadial = bearing;

  // Move previous target to location that yields longest distance,
  // plus a little so optimal path vector points to next waypoint.
}


void AATDistance::ShiftTargetFromInFront(double longitude, double latitude,
                                         int taskwaypoint) {

  double course_bearing;

  // this point is in sector and is improved

  // JMW, now moves target to in line with previous target whenever
  // you are in AAT sector and improving on the target distance

  //JMWAAT  Task[taskwaypoint].AATTargetOffsetRadial = -1.0;

  if (Task[taskwaypoint].AATTargetLocked) {
    // have improved on the locked value, so unlock it in case user
    // wants to move it.
    Task[taskwaypoint].AATTargetOffsetRadius = -1.0;
    Task[taskwaypoint].AATTargetOffsetRadial = 0;
    Task[taskwaypoint].AATTargetLocked = false;
  }

  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
                                 Task[taskwaypoint].AATTargetOffsetRadial);

  FindLatitudeLongitude(latitude, longitude,
                        course_bearing, AATCloseDistance(),
                        &Task[taskwaypoint].AATTargetLat,
                        &Task[taskwaypoint].AATTargetLon);
  // JMW, distance here was 100m, now changed to speed * 2

  TargetModified = true;
  CalculateAATIsoLines();
}


extern bool TargetDialogOpen;


void AATDistance::ShiftTargetFromBehind(double longitude, double latitude,
                              int taskwaypoint) {

  // JMWAAT if being externally updated e.g. from task dialog, don't move it
  if (TargetDialogOpen) return;
  if (taskwaypoint==0) return;

  // best is decreasing or first entry in sector, so project
  // target in direction of improvement or first entry into sector

  double course_bearing;
  double course_bearing_orig;
  double d_total_orig;
  double d_total_this;

  d_total_this = DoubleLegDistance(taskwaypoint,
                                   longitude,
                                   latitude);

  d_total_orig = DoubleLegDistance(taskwaypoint,
                                   Task[taskwaypoint].AATTargetLon,
                                   Task[taskwaypoint].AATTargetLat);

  if (d_total_this>d_total_orig-2.0*AATCloseDistance()) {
    // this is better than the previous best! (or very close)
    ShiftTargetFromInFront(longitude, latitude, taskwaypoint);
    return;
  }

  // JMWAAT if locked, don't move it
  if (Task[taskwaypoint].AATTargetLocked) {
    // 20080615 JMW don't do this; locked stays locked
    // Task[taskwaypoint].AATTargetLocked = false; // JMWAAT JB
    return;
  }

  /*
  // check to see if deviation is big enough to adjust target along track
  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);

  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  Task[taskwaypoint].AATTargetLat,
                  Task[taskwaypoint].AATTargetLon,
                  NULL, &course_bearing_orig);

  if (fabs(AngleLimit180(course_bearing-course_bearing_orig))<5.0) {
    // don't update it if course deviation is less than 5 degrees,
    // otherwise we end up wasting a lot of CPU in recalculating, and also
    // the target ends up drifting.
    return;
  }

  course_bearing = AngleLimit360(course_bearing+
                                 Task[taskwaypoint].AATTargetOffsetRadial);
  //JMWAAT  Task[taskwaypoint].AATTargetOffsetRadial = course_bearing;
  */

  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);
  course_bearing = AngleLimit360(course_bearing+
                                 Task[taskwaypoint].AATTargetOffsetRadial);

  DistanceBearing(latitude,
                  longitude,
                  Task[taskwaypoint].AATTargetLat,
                  Task[taskwaypoint].AATTargetLon,
                  NULL, &course_bearing_orig);

  if (fabs(AngleLimit180(course_bearing-course_bearing_orig))<5.0) {
    // don't update it if course deviation is less than 5 degrees,
    // otherwise we end up wasting a lot of CPU in recalculating, and also
    // the target ends up drifting.
    return;
  }

  double max_distance =
    FindInsideAATSectorDistance(latitude,
                                longitude,
                                taskwaypoint,
                                course_bearing,
                                0);

  // total distance of legs from previous through this to next target
  double delta = max_distance/2;

  // move target in line with previous target along track
  // at an offset to improve on max distance

  double t_distance_lower = 0;
  double t_distance = delta*2;

  int steps = 0;

  do {
    // find target position along projected line but
    // make sure it is in sector, and set at a distance
    // to preserve total task distance
    // we are aiming to make d_total_this = d_total_orig

    double t_lat, t_lon;
    FindLatitudeLongitude(latitude, longitude,
                          course_bearing, t_distance,
                          &t_lat,
                          &t_lon);

    if (InAATTurnSector(t_lon, t_lat, taskwaypoint)) {
      d_total_this = DoubleLegDistance(taskwaypoint,
                                       t_lon,
                                       t_lat);
      if (d_total_orig - d_total_this>0.0) {
        t_distance_lower = t_distance;
        // ok, can go further
        t_distance += delta;
      } else {
        t_distance -= delta;
      }
    } else {
      t_distance -= delta;
    }
    delta /= 2.0;
  } while ((delta>5.0) && (steps++<20));

  // now scan to edge of sector to find approximate range %
  if (t_distance_lower>5.0) {
    FindLatitudeLongitude(latitude, longitude,
                          course_bearing, t_distance_lower,
                          &Task[taskwaypoint].AATTargetLat,
                          &Task[taskwaypoint].AATTargetLon);

    Task[taskwaypoint].AATTargetOffsetRadius =
      FindInsideAATSectorRange(latitude,
                               longitude,
                               taskwaypoint, course_bearing,
                               t_distance_lower);
    TargetModified = true;
    CalculateAATIsoLines();

  }

  //  if ((!t_in_sector) && (d_diff_total>1.0)) {
    // JMW TODO enhancement: this is too short now so need to lengthen the
    // next waypoint if possible
    // (re discussion with paul mander)
  //  }
}


double AATDistance::DistanceCovered_internal(double longitude,
                                             double latitude,
                                             bool insector) {
  double achieved;
  if (!ValidTaskPoint(ActiveWayPoint) || (ActiveWayPoint==0)) {
    //   max_achieved_distance = 0;
    return 0.0;
  }
  LockTaskData();
  if (insector) {
    achieved = DistanceCovered_inside(longitude, latitude);
  } else {
    achieved = DistanceCovered_outside(longitude, latitude);
  }

  UnlockTaskData();
  //  max_achieved_distance = max(achieved, max_achieved_distance);
  return achieved;
}


double AATDistance::DistanceCovered_inside(double longitude,
                                           double latitude) {

  int taskwaypoint = ActiveWayPoint;

  double best_achieved_distance = 0;

  int nthis = num_points[taskwaypoint];
  if (nthis>0) {
    int kbest = 0;
    for (int k=0; k<nthis; k++) {
      double achieved_distance = Dmax[taskwaypoint][k];
      if (achieved_distance>best_achieved_distance) {
        best_achieved_distance = achieved_distance;
        best[taskwaypoint] = k;
        kbest = k;
      }
    }
    if (ValidTaskPoint(taskwaypoint+1)) {
      ShiftTargetFromBehind(longitude, latitude, taskwaypoint);
    }
    return distance_achieved(taskwaypoint, kbest, longitude, latitude);
  } else {
    // not actually in this sector?
    return 0.0;
  }
}


double AATDistance::distance_achieved(int taskwaypoint, int jbest,
                                      double longitude, double latitude) {
  double achieved = Dmax[taskwaypoint][jbest];
  double d0a;
  DistanceBearing(lat_points[taskwaypoint][jbest],
                  lon_points[taskwaypoint][jbest],
                  latitude,
                  longitude,
                  &d0a, NULL);

  legdistance_achieved[taskwaypoint] = 0;
  if (d0a>0) {
    // Calculates projected distance from P3 along line P1-P2
    legdistance_achieved[taskwaypoint] =
      ProjectedDistance(lon_points[taskwaypoint][jbest],
                        lat_points[taskwaypoint][jbest],
                        Task[taskwaypoint+1].AATTargetLon,
                        Task[taskwaypoint+1].AATTargetLat,
                        longitude, latitude);
    achieved += legdistance_achieved[taskwaypoint];
  }

  return achieved;
}


double AATDistance::DistanceCovered_outside(double longitude,
                                            double latitude) {
  if (ActiveWayPoint<=0) {
    return 0.0;
  }

  int taskwaypoint = ActiveWayPoint;

  int nlast = num_points[taskwaypoint-1];
  int nstart = 0;
  if (taskwaypoint==1) {
    // only take last point in sector (crossing start line) for
    // distance calculations
    if (nlast > 0) {
      nstart = nlast-1;
    } else {
      // cheat first point to the task start point if no valid start
      nstart = 0;
      nlast = 1;
      lat_points[0][0] = Task[0].AATTargetLat;
      lon_points[0][0] = Task[0].AATTargetLon;
      Dmax[0][0] = 0;
    }
  } else {
    nstart = 0;
  }
  if (nlast==0) {
    // no points in previous sector!
    return 0.0;
  }

  LockTaskData();
  double retval = 0.0;
  double best_doubleleg_distance = 0;
  int jbest = -1;
  for (int j=nstart; j<nlast; j++) {

    double d0t;
    DistanceBearing(lat_points[taskwaypoint-1][j],
                    lon_points[taskwaypoint-1][j],
                    Task[taskwaypoint].AATTargetLat,
                    Task[taskwaypoint].AATTargetLon,
                    &d0t, NULL);

    double doubleleg_distance = Dmax[taskwaypoint-1][j] + d0t;

    if (doubleleg_distance > best_doubleleg_distance) {
      best_doubleleg_distance = doubleleg_distance;
      jbest = j;
    }
  }

  if (jbest>=0) {
    // set previous target for display purposes
    best[taskwaypoint-1] = jbest;
    Task[taskwaypoint-1].AATTargetLat= lat_points[taskwaypoint-1][jbest];
    Task[taskwaypoint-1].AATTargetLon= lon_points[taskwaypoint-1][jbest];
    retval = distance_achieved(taskwaypoint-1, jbest, longitude, latitude);
  } else {
    retval = 0.0;
  }
  UnlockTaskData();
  return retval;
}


/*
JMW
  Notes: distancecovered can decrease if the glider flys backwards along
  the task --- since points for outlanding are determined not on
  greatest distance along task but landing location, this is reasonable.

*/


double AATDistance::DistanceCovered(double longitude,
                                    double latitude,
                                    int taskwaypoint) {
  (void)taskwaypoint; // unused
  double retval;
  LockTaskData();
  retval= DistanceCovered_internal(longitude, latitude, false);
  UnlockTaskData();
  return retval;
}


void AATDistance::UpdateSearch(int taskwaypoint) {
  int n = num_points[taskwaypoint];
  int nlast = num_points[taskwaypoint-1];
  double dist, distbest;
  int ibest;
  int j, k;

  if ((nlast==0) && (taskwaypoint==1)) {
    // cheat first point to the task start point if no valid start
    nlast = 1;
    lat_points[0][0] = Task[0].AATTargetLat;
    lon_points[0][0] = Task[0].AATTargetLon;
  }

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
    Dmax[taskwaypoint][j]= distbest;
    imax[taskwaypoint][j]= ibest;
  }
}

bool AATDistance::HasEntered(int taskwaypoint) {
  bool retval = false;
  LockTaskData();
  retval = has_entered[taskwaypoint];
  UnlockTaskData();
  return retval;
}

void AATDistance::ThinData(int taskwaypoint) {
  double contractfactor = 0.8;
  static bool do_delete[MAXNUM_AATDISTANCE];

  LockTaskData();

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
      if ((d<distancethreshold[taskwaypoint]) && (best[taskwaypoint]!=i)) {
	do_delete[i] = true; // mark it for deletion
      }
    }

    // now shuffle points along
    int j;
    i = 0; j = i;

    int pnts_in_new=0;
    while (j< num_points[taskwaypoint]) {
      if (!do_delete[j]) {
	lat_points[taskwaypoint][i] = lat_points[taskwaypoint][j];
	lon_points[taskwaypoint][i] = lon_points[taskwaypoint][j];
	i++;
	pnts_in_new = i;
      }
      j++;
    }
    if (pnts_in_new) {
      num_points[taskwaypoint] = pnts_in_new;
    }
  }
  if (num_points[taskwaypoint]>=MAXNUM_AATDISTANCE) {
    // error!
    num_points[taskwaypoint]=MAXNUM_AATDISTANCE-1;
  }
  UnlockTaskData();
}


double AATCloseDistance(void) {
  return max(100,GPS_INFO.Speed*1.5);
}
