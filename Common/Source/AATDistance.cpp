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
      if ((dist>distancethreshold[taskwaypoint])||(n==1)) {
        new_point = true;
      }
    } else {
      new_point = true;
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


double DoubleLegDistance(int taskwaypoint,
                         double longitude,
                         double latitude) {

  double d0;
  double d1;
  if (taskwaypoint>0) {
    DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                    Task[taskwaypoint-1].AATTargetLon,
                    latitude,
                    longitude,
                    &d0, NULL);
  } else {
    d0 = 0;
  }
  
  DistanceBearing(latitude,
                  longitude,
                  Task[taskwaypoint+1].AATTargetLat,
                  Task[taskwaypoint+1].AATTargetLon,
                  &d1, NULL);
  return d0 + d1;
}


void AATDistance::ShiftTargetOutside(double longitude, double latitude,
                                    int taskwaypoint) {
  // if no improvement possible, vector to outside
  double bearing;

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


void AATDistance::ShiftTargetFromInFront(double longitude, double latitude,
                                         int taskwaypoint) {

  double course_bearing;

  // this point is in sector and is improved
  
  // JMW, now moves target to in line with previous target whenever
  // you are in AAT sector and improving on the target distance

  Task[taskwaypoint].AATTargetOffsetRadial = -1.0;
  
  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);
  
  FindLatitudeLongitude(latitude, longitude, 
                        course_bearing, 100.0,
                        &Task[taskwaypoint].AATTargetLat,
                        &Task[taskwaypoint].AATTargetLon);
  
  Task[taskwaypoint].AATTargetOffsetRadial = course_bearing;
}


void AATDistance::ShiftTargetFromBehind(double longitude, double latitude,
                              int taskwaypoint) {

  // best is decreasing or first entry in sector, so project
  // target in direction of improvement or first entry into sector
  
  double course_bearing;
  double d_total_orig;
  double d_total_this;
  
  d_total_this = DoubleLegDistance(taskwaypoint, 
                                   longitude,
                                   latitude);

  d_total_orig = DoubleLegDistance(taskwaypoint, 
                                   Task[taskwaypoint].AATTargetLon,
                                   Task[taskwaypoint].AATTargetLat);

  if (d_total_this>d_total_orig) {
    // this is better than the previous best!
    ShiftTargetFromInFront(longitude, latitude, taskwaypoint);
    return;
  }

  DistanceBearing(Task[taskwaypoint-1].AATTargetLat,
                  Task[taskwaypoint-1].AATTargetLon,
                  latitude,
                  longitude,
                  NULL, &course_bearing);
  
  // total distance of legs from previous through this to next target
  
  double d_diff_total = d_total_orig-d_total_this;
  bool t_in_sector = false;
    
  // move target in line with previous target along track
  // at an offset to improve on max distance
  
  Task[taskwaypoint].AATTargetOffsetRadial = course_bearing;
  
  double t_distance = 0;
  bool updated = false;
  double p_found;
  double delta;
  double inv_slope= 0.4;
  double d_total_last;
  
  do {
    
    delta = max(5.0,min(fabs(d_diff_total*inv_slope),250.0));
    t_distance += delta;
    
    double t_lat, t_lon;
    
    // find target position along projected line but
    // make sure it is in sector, and set at a distance
    // to preserve total task distance
    // we are aiming to make d_total_this = d_total_orig
    
    FindLatitudeLongitude(latitude, longitude, 
                          course_bearing, t_distance,
                          &t_lat,
                          &t_lon);
    
    t_in_sector = InAATTurnSector(t_lon,
                                  t_lat,
                                  taskwaypoint);
    
    d_total_last = d_total_this;
    d_total_this = DoubleLegDistance(taskwaypoint, 
                                     t_lon,
                                     t_lat);
    
    d_diff_total = d_total_orig - d_total_this;

    if (t_in_sector && (d_diff_total>0.0)) {
      updated = true;
      Task[taskwaypoint].AATTargetLon = t_lon;
      Task[taskwaypoint].AATTargetLat = t_lat;
      p_found = t_distance;
      if (d_total_this-d_total_last>1.0) {
        inv_slope = delta/(d_total_this-d_total_last)*0.8;
      }
    }
    
  } while (t_in_sector);

  if ((updated) && (t_distance>0.0)) {
    Task[taskwaypoint].AATTargetOffsetRadius = (p_found / t_distance)*2-1;
  }

  if (d_diff_total>1.0) {
    // TODO: this is too short now so need to lengthen the next waypoint
    // if possible
  }
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
    return distance_achieved(taskwaypoint-1, jbest, longitude, latitude);
  } else {
    return 0.0;
  }
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
