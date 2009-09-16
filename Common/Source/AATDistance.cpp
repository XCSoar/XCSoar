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
#include "Task.h"
#include "Math/Geometry.hpp"
#include "SettingsTask.hpp"
#include "Math/Earth.hpp"
#include "WayPoint.hpp"
#include "Components.hpp"
#include "WayPointList.hpp"

#include <math.h>

#define DISTANCETHRESHOLD 500


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
      loc_points[i][j].Latitude=0;
      loc_points[i][j].Longitude=0;
      imax[i][j]=0;
    }
  }
}


double AATDistance::LegDistanceAchieved(int taskwaypoint) {
  double retval;
  Lock();
  retval= legdistance_achieved[taskwaypoint];
  Unlock();
  return retval;
}

void AATDistance::ResetEnterTrigger(int taskwaypoint) {
  Lock();
  has_entered[taskwaypoint] = false;
  Unlock();
}

void AATDistance::AddPoint(const GEOPOINT &location,
                           const int taskwaypoint,
			   const double aatclosedistance) {
  if (taskwaypoint<0) return;

  bool was_entered = has_entered[taskwaypoint];
  has_entered[taskwaypoint] = true;

  if (!AATEnabled) return; // nothing else to do for non-AAT tasks

  Lock();

  // should only add ONE point to start.
  // If restart, need to reset

  if (num_points[taskwaypoint]<MAXNUM_AATDISTANCE) {

    int n = num_points[taskwaypoint];

    bool new_point= false;

    if (n>1) {
      double dist;
      DistanceBearing(loc_points[taskwaypoint][n-2],
                      location, &dist, NULL);
      if (dist>distancethreshold[taskwaypoint]) {
        new_point = true;
      }
    } else {
      // first point in sector
      new_point = true;

      if ((!was_entered) && (taskwaypoint>0) &&
          !task_stats[taskwaypoint].AATTargetLocked) {
        double qdist, bearing0, bearing1;
        DistanceBearing(task_stats[taskwaypoint-1].AATTargetLocation,
                        location,
                        &qdist, &bearing0);
        DistanceBearing(task_stats[taskwaypoint-1].AATTargetLocation,
                        task_stats[taskwaypoint].AATTargetLocation,
                        &qdist, &bearing1);
        // JMWAAT
        task_stats[taskwaypoint].AATTargetOffsetRadial = 0.0;
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
    loc_points[taskwaypoint][max(0,num_points[taskwaypoint]-1)]= location;

    // update max search for this and future waypoints
    if (taskwaypoint>0) {
      for (int i= taskwaypoint; i<MAXTASKPOINTS-1; i++) {
        UpdateSearch(i);
      }
      if (taskwaypoint == ActiveTaskPoint) {
        DistanceCovered_internal(location, true, aatclosedistance);
      }
    }
  }
}


void AATDistance::ShiftTargetOutside(const GEOPOINT &location,
                                    const int taskwaypoint) {
  // if no improvement possible, vector to outside
  double bearing;

  if (taskwaypoint>0) {
    DistanceBearing(location,
                    way_points.get(task_points[taskwaypoint+1].Index).Location,
                    NULL, &bearing);

    FindLatitudeLongitude(location,
                          bearing, 100.0,
                          &task_stats[taskwaypoint].AATTargetLocation);
    task.SetTargetModified();
  }

  //JMWAAT  task_stats[taskwaypoint].AATTargetOffsetRadial = bearing;

  // Move previous target to location that yields longest distance,
  // plus a little so optimal path vector points to next waypoint.
}


void AATDistance::ShiftTargetFromInFront(const GEOPOINT &location,
                                         const int taskwaypoint,
					 const double aatclosedistance) {

  double course_bearing;

  // this point is in sector and is improved

  // JMW, now moves target to in line with previous target whenever
  // you are in AAT sector and improving on the target distance

  //JMWAAT  task_stats[taskwaypoint].AATTargetOffsetRadial = -1.0;

  if (task_stats[taskwaypoint].AATTargetLocked) {
    // have improved on the locked value, so unlock it in case user
    // wants to move it.
    task_stats[taskwaypoint].AATTargetOffsetRadius = -1.0;
    task_stats[taskwaypoint].AATTargetOffsetRadial = 0;
    task_stats[taskwaypoint].AATTargetLocked = false;
  }

  DistanceBearing(task_stats[taskwaypoint-1].AATTargetLocation,
                  location,
                  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
                                 task_stats[taskwaypoint].AATTargetOffsetRadial);

  FindLatitudeLongitude(location,
                        course_bearing, aatclosedistance,
                        &task_stats[taskwaypoint].AATTargetLocation);
  // JMW, distance here was 100m, now changed to speed * 2

  task.SetTargetModified();
}


void AATDistance::ShiftTargetFromBehind(const GEOPOINT &location,
					const int taskwaypoint,
					const double aatclosedistance) {

  // JMWAAT if being extrnally updated e.g. from task dialog, don't move it
  if (targetManipEvent.test()) return;
  if (taskwaypoint==0) return;

  // best is decreasing or first entry in sector, so project
  // target in direction of improvement or first entry into sector

  double course_bearing;
  double course_bearing_orig;
  double d_total_orig;
  double d_total_this;

  d_total_this = task.DoubleLegDistance(taskwaypoint, location);

  d_total_orig = task.DoubleLegDistance(taskwaypoint,
                                   task_stats[taskwaypoint].AATTargetLocation);

  if (d_total_this>d_total_orig-2.0*aatclosedistance) {
    // this is better than the previous best! (or very close)
    ShiftTargetFromInFront(location, taskwaypoint, aatclosedistance);
    return;
  }

  // JMWAAT if locked, don't move it
  if (task_stats[taskwaypoint].AATTargetLocked) {
    // 20080615 JMW don't do this; locked stays locked
    // task_stats[taskwaypoint].AATTargetLocked = false; // JMWAAT JB
    return;
  }

  /*
  // check to see if deviation is big enough to adjust target along track
  DistanceBearing(task_stats[taskwaypoint-1].AATTargetLat,
                  task_stats[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);

  DistanceBearing(task_stats[taskwaypoint-1].AATTargetLat,
                  task_stats[taskwaypoint-1].AATTargetLon,
                  task_stats[taskwaypoint].AATTargetLat,
                  task_stats[taskwaypoint].AATTargetLon,
                  NULL, &course_bearing_orig);

  if (fabs(AngleLimit180(course_bearing-course_bearing_orig))<5.0) {
    // don't update it if course deviation is less than 5 degrees,
    // otherwise we end up wasting a lot of CPU in recalculating, and also
    // the target ends up drifting.
    return;
  }

  course_bearing = AngleLimit360(course_bearing+
                                 task_stats[taskwaypoint].AATTargetOffsetRadial);
  //JMWAAT  task_stats[taskwaypoint].AATTargetOffsetRadial = course_bearing;
  */

  DistanceBearing(task_stats[taskwaypoint-1].AATTargetLocation,
                  location,
                  NULL, &course_bearing);
  course_bearing = AngleLimit360(course_bearing+
                                 task_stats[taskwaypoint].AATTargetOffsetRadial);

  DistanceBearing(location,
                  task_stats[taskwaypoint].AATTargetLocation,
                  NULL, &course_bearing_orig);

  if (fabs(AngleLimit180(course_bearing-course_bearing_orig))<5.0) {
    // don't update it if course deviation is less than 5 degrees,
    // otherwise we end up wasting a lot of CPU in recalculating, and also
    // the target ends up drifting.
    return;
  }

  double max_distance =
    task.FindInsideAATSectorDistance(location,
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

    GEOPOINT t_loc;
    FindLatitudeLongitude(location,
                          course_bearing, t_distance,
                          &t_loc);

    if (task.InAATTurnSector(t_loc, taskwaypoint)) {
      d_total_this = task.DoubleLegDistance(taskwaypoint,
                                            t_loc);
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
    FindLatitudeLongitude(location,
                          course_bearing, t_distance_lower,
                          &task_stats[taskwaypoint].AATTargetLocation);

    task_stats[taskwaypoint].AATTargetOffsetRadius =
      task.FindInsideAATSectorRange(location,
                                    taskwaypoint, course_bearing,
                                    t_distance_lower);
    task.SetTargetModified();
  }

  //  if ((!t_in_sector) && (d_diff_total>1.0)) {
    // JMW TODO enhancement: this is too short now so need to lengthen the
    // next waypoint if possible
    // (re discussion with paul mander)
  //  }
}


double AATDistance::DistanceCovered_internal(const GEOPOINT &location,
                                             const bool insector,
					     const double aatclosedistance) {
  double achieved;
  if (!task.Valid() || (ActiveTaskPoint==0)) {
    //   max_achieved_distance = 0;
    return 0.0;
  }
  Lock();
  if (insector) {
    achieved = DistanceCovered_inside(location, aatclosedistance);
  } else {
    achieved = DistanceCovered_outside(location, aatclosedistance);
  }
  Unlock();
  //  max_achieved_distance = max(achieved, max_achieved_distance);
  return achieved;
}


double AATDistance::DistanceCovered_inside(const GEOPOINT &location,
                                           const double aatclosedistance) {

  int taskwaypoint = ActiveTaskPoint;

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
    if (task.ValidTaskPoint(taskwaypoint+1)) {
      ShiftTargetFromBehind(location, taskwaypoint, aatclosedistance);
    }
    return distance_achieved(taskwaypoint, kbest, location);
  } else {
    // not actually in this sector?
    return 0.0;
  }
}


double AATDistance::distance_achieved(const int taskwaypoint, 
                                      const int jbest,
                                      const GEOPOINT &location) {
  double achieved = Dmax[taskwaypoint][jbest];
  double d0a;
  DistanceBearing(loc_points[taskwaypoint][jbest],
                  location,
                  &d0a, NULL);

  legdistance_achieved[taskwaypoint] = 0;
  if (d0a>0) {
    // Calculates projected distance from P3 along line P1-P2
    legdistance_achieved[taskwaypoint] =
      ProjectedDistance(loc_points[taskwaypoint][jbest],
                        task_stats[taskwaypoint+1].AATTargetLocation,
                        location);
    achieved += legdistance_achieved[taskwaypoint];
  }

  return achieved;
}


double AATDistance::DistanceCovered_outside(const GEOPOINT &location,
					    const double aatclosedistance) {
  if (ActiveTaskPoint<=0) {
    return 0.0;
  }

  int taskwaypoint = ActiveTaskPoint;

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
      loc_points[0][0] = task_stats[0].AATTargetLocation;
      Dmax[0][0] = 0;
    }
  } else {
    nstart = 0;
  }
  if (nlast==0) {
    // no points in previous sector!
    return 0.0;
  }

  double retval = 0.0;
  Lock();
  double best_doubleleg_distance = 0;
  int jbest = -1;
  for (int j=nstart; j<nlast; j++) {

    double d0t;
    DistanceBearing(loc_points[taskwaypoint-1][j],
                    task_stats[taskwaypoint].AATTargetLocation,
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
    task_stats[taskwaypoint-1].AATTargetLocation= loc_points[taskwaypoint-1][jbest];
    retval = distance_achieved(taskwaypoint-1, jbest, location);
  } else {
    retval = 0.0;
  }
  Unlock();
  return retval;
}


/*
JMW
  Notes: distancecovered can decrease if the glider flys backwards along
  the task --- since points for outlanding are determined not on
  greatest distance along task but landing location, this is reasonable.

*/


double AATDistance::DistanceCovered(const GEOPOINT &location,
                                    const int taskwaypoint,
				    const double aatclosedistance) {
  (void)taskwaypoint; // unused
  double retval;
  Lock();
  retval= DistanceCovered_internal(location, false,
				   aatclosedistance);
  Unlock();
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
    loc_points[0][0] = task_stats[0].AATTargetLocation;
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
      DistanceBearing(loc_points[taskwaypoint-1][k],
                      loc_points[taskwaypoint][j], &dtmp, NULL);

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
  Lock();
  retval = has_entered[taskwaypoint];
  Unlock();
  return retval;
}

void AATDistance::ThinData(int taskwaypoint) {
  double contractfactor = 0.8;
  static bool do_delete[MAXNUM_AATDISTANCE];

  Lock();

  int i;
  for (i=0; i<MAXNUM_AATDISTANCE; i++) {
    do_delete[i]= false;
  }

  while (num_points[taskwaypoint]> MAXNUM_AATDISTANCE*contractfactor) {
    distancethreshold[taskwaypoint] /= contractfactor;

    for (i= num_points[taskwaypoint]-1; i>0; i--) {

      double d;
      DistanceBearing(loc_points[taskwaypoint][i],
                      loc_points[taskwaypoint][i-1], &d, NULL);
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
	loc_points[taskwaypoint][i] = loc_points[taskwaypoint][j];
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
  Unlock();
}

