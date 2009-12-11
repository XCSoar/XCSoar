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

#include "TaskVisitor.hpp"
#include "Task.h"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Math/Constants.h"
#include "Math/FastMath.h"
#include "Components.hpp"
#include "WayPointList.hpp"

#include <stdio.h>

// note: if this calls any task member functions they will be locked
// recursively


void Task::scan_point_forward(RelativeTaskPointVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();
  if (settings.EnableMultipleStartPoints) {
    for (unsigned j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_start_point(task_start_points[j], j);
    }
  }
  if (!ValidTaskPoint(0)) {
    visitor.visit_null();
    return;
  }

  unsigned i=0;
  while (i<ActiveTaskPoint) {
    visitor.visit_task_point_before(task_points[i], i);
    i++;
  }
  visitor.visit_task_point_current(task_points[i], i);
  i++;
  while (ValidTaskPoint(i)) {
    visitor.visit_task_point_after(task_points[i], i);
    i++;
  }
}


void Task::scan_point_forward(AbsoluteTaskPointVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();

  if (settings.EnableMultipleStartPoints) {
    for (unsigned j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_start_point(task_start_points[j], j);
    }
  }

  if (!Valid()) {
    visitor.visit_null();
    return;
  }
  unsigned i=0;
  while ((i+1<MAXTASKPOINTS) && (task_points[i+1].Index != -1)) {
    if (i==0) {
      visitor.visit_task_point_start(task_points[0], 0);
    } else {
      visitor.visit_task_point_intermediate(task_points[i], i);
    }
    i++;
  }
  visitor.visit_task_point_final(task_points[i], i);
}


void Task::scan_leg_forward(RelativeTaskLegVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();

  if (!Valid()) {
    visitor.visit_null();
    return;
  }
  if (!ValidTaskPoint(1)) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  unsigned i=0;
  while (i+1<ActiveTaskPoint) {
    visitor.visit_leg_before(task_points[i], i,
                             task_points[i+1], i+1);
    i++;
  }
  if (i+1==ActiveTaskPoint) {
    visitor.visit_leg_current(task_points[i], i,
                              task_points[i+1], i+1);
    i++;
  }
  while (ValidTaskPoint(i+1)) {
    visitor.visit_leg_after(task_points[i], i,
                            task_points[i+1], i+1);
    i++;
  }
}




void Task::scan_leg_forward(AbsoluteTaskLegVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();

  if (!Valid()) {
    visitor.visit_null();
    return;
  }
  if (settings.EnableMultipleStartPoints) {
    for (unsigned j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_leg_multistart(task_start_points[j], j, task_points[0]);
    }
  }
  if (!ValidTaskPoint(1)) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  unsigned i=0;
  while (ValidTaskPoint(i+2)) {
    visitor.visit_leg_intermediate(task_points[i], i,
				   task_points[i+1], i+1);
    i++;
  }
  visitor.visit_leg_final(task_points[i], i,
			  task_points[i+1], i+1);
}


void Task::scan_leg_reverse(AbsoluteTaskLegVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();

  if (!Valid()) {
    visitor.visit_null();
    return;
  }
  unsigned final = getFinalWaypoint();
  if (final==0) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  int i= final-1;
  while (i>=0) {
    if (i+1==(int)final) {
      visitor.visit_leg_final(task_points[i], i,
                              task_points[i+1], i+1);
    } else {
      visitor.visit_leg_intermediate(task_points[i], i,
                                     task_points[i+1], i+1);
    }
    i--;
  }

  if (settings.EnableMultipleStartPoints) {
    for (unsigned j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_leg_multistart(task_start_points[j], j, task_points[0]);
    }
  }
}


void Task::scan_leg_reverse(RelativeTaskLegVisitor &visitor)
{
  visitor.setTask(*this);
  visitor.visit_reset();

  if (!Valid()) {
    visitor.visit_null();
    return;
  }
  if (!ValidTaskPoint(1)) {
    visitor.visit_single(task_points[0], 0);
    return;
  }

  unsigned final = getFinalWaypoint();
  if (final==0) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  unsigned a= ActiveTaskPoint;
  unsigned i= final;
  while (i>0) {
    if (i<a) {
      visitor.visit_leg_before(task_points[i-1], i-1,
                               task_points[i], i);
    } else if (i==a) {
      visitor.visit_leg_current(task_points[i-1], i-1,
                                task_points[i], i);
    } else {
      visitor.visit_leg_after(task_points[i-1], i-1,
                              task_points[i], i);
    }
    i--;
  }

}

class RelativeTaskLegVisitorExample:
  public RelativeTaskLegVisitor {
public:
  // returns false when no more processing required
  virtual void visit_null()
  {
    printf("null task\n");
  };
  virtual void visit_single(TASK_POINT &point0, const unsigned index0)
  {
    printf("single point %d\n", index0);
  };
  virtual void visit_leg_before(TASK_POINT &point0, const unsigned index0,
				TASK_POINT &point1, const unsigned index1)
  {
    printf("leg before %d %d\n", index0, index1);
  };
  virtual void visit_leg_current(TASK_POINT &point0, const unsigned index0,
				 TASK_POINT &point1, const unsigned index1)
  {
    printf("leg current %d %d\n", index0, index1);
  };
  virtual void visit_leg_after(TASK_POINT &point0, const unsigned index0,
			       TASK_POINT &point1, const unsigned index1)
  {
    printf("leg after %d %d\n", index0, index1);
  };
};


class RelativeTaskPointVisitorExample:
  public RelativeTaskPointVisitor
{
public:
  // returns false when no more processing required
  virtual void visit_null()
  {
    printf("no task\n");
  };
  virtual void visit_task_point_before(TASK_POINT &point, const unsigned index)
  {
    printf("point before %d\n",index);
  };
  virtual void visit_task_point_current(TASK_POINT &point, const unsigned index)
  {
    printf("point current %d\n",index);
  };
  virtual void visit_task_point_after(TASK_POINT &point, const unsigned index)
  {
    printf("point after %d\n",index);
  };
};

class AbsoluteTaskPointVisitorExample:
  public AbsoluteTaskPointVisitor {
public:
  // returns false when no more processing required
  virtual void visit_null() {
    val=0;
  };
  virtual void visit_task_point_start(TASK_POINT &point, const unsigned index) {
    val=1;
  };
  virtual void visit_task_point_intermediate(TASK_POINT &point, const unsigned index) {
    val++;
  };
  virtual void visit_task_point_final(TASK_POINT &point, const unsigned index) {
    val++;
    printf("final there are %d points\n",val);
  };
  int val;
};

#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "WayPointParser.h"

class TaskSectorsVisitor:
  public AbsoluteTaskPointVisitor {
public:
  void visit_task_point_start(TASK_POINT &point, const unsigned index) {
    if (_task->getSettings().StartType== START_SECTOR) {
      SectorAngle = 45+90;
    } else {
      SectorAngle = 90;
    }
    SectorSize = _task->getSettings().StartRadius;
    SectorBearing = point.OutBound;
    setStartEnd(point);
    clearAAT(point, index);
  };
  void visit_task_point_intermediate(TASK_POINT &point, const unsigned index) {
    SectorAngle = 45;
    if (_task->getSettings().SectorType == 2) {
      SectorSize = 10000; // German DAe 0.5/10
    } else {
      SectorSize = _task->getSettings().SectorRadius;  // FAI sector
    }
    SectorBearing = point.Bisector;
    if (!_task->getSettings().AATEnabled) {
      point.AATStartRadial  = AngleLimit360(SectorBearing - SectorAngle);
      point.AATFinishRadial = AngleLimit360(SectorBearing + SectorAngle);
    }
    setStartEnd(point);
  };
  void visit_task_point_final(TASK_POINT &point, const unsigned index) {
    // finish line
    if (_task->getSettings().FinishType==FINISH_SECTOR) {
      SectorAngle = 45;
    } else {
      SectorAngle = 90;
    }
    SectorSize = _task->getSettings().FinishRadius;
    SectorBearing = point.InBound;
    setStartEnd(point);

    clearAAT(point, index);
  };

private:
  double SectorAngle;
  double SectorSize;
  double SectorBearing;
  //
  void setStartEnd(TASK_POINT &pt) {
    FindLatitudeLongitude(way_points.get(pt.Index).Location,
			  SectorBearing + SectorAngle, SectorSize,
			  &pt.SectorStart);
    FindLatitudeLongitude(way_points.get(pt.Index).Location,
			  SectorBearing - SectorAngle, SectorSize,
			  &pt.SectorEnd);
  }
  void clearAAT(TASK_POINT &point, const unsigned i) {
    point.AATTargetOffsetRadius = 0.0;
    point.AATTargetOffsetRadial = 0.0;
    point.AATTargetLocation = way_points.get(point.Index).Location;
  };
};


class WaypointsInTaskVisitor:
  public AbsoluteTaskPointVisitor
{
public:
  WaypointsInTaskVisitor(const SETTINGS_COMPUTER& _settings_computer):
    settings_computer(&_settings_computer)
  {
  }
  void visit_reset() {
    for (unsigned i = 0; way_points.verify_index(i); i++)
      way_points.set_calc(i).InTask =
        (way_points.get(i).Flags & HOME) == HOME;

    if (way_points.verify_index(settings_computer->HomeWaypoint)) {
      way_points.set_calc(settings_computer->HomeWaypoint).InTask = true;
    }
  }
  void visit_start_point(START_POINT &point, const unsigned i) {
    way_points.set_calc(task_start_points[i].Index).InTask = true;
  }
  void visit_task_point_start(TASK_POINT &point, const unsigned i) {
    addTaskPoint(point.Index);
  }
  void visit_task_point_intermediate(TASK_POINT &point, const unsigned i) {
    addTaskPoint(point.Index);
  }
  void visit_task_point_final(TASK_POINT &point, const unsigned i) {
    addTaskPoint(point.Index);
  }
private:
  const SETTINGS_COMPUTER *settings_computer;
  void addTaskPoint(const unsigned i) {
    way_points.set_calc(i).InTask = true;
  }
};


class TaskLegGeometryVisitor:
  public AbsoluteTaskLegVisitor {
public:
  void visit_reset()
  {
    total_length = 0;
  };
  void visit_single(TASK_POINT &point0, const unsigned index0) {
    point0.LegDistance=0;
    point0.LegBearing=0;
    point0.InBound=0;
  };
  void visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
			      TASK_POINT &point1, const unsigned index1)
  {
    if (index0==0) {
      // first leg is special
      point0.LegDistance=0;
      point0.LegBearing=0;
      point0.InBound=0;
    }
    DistanceBearing(way_points.get(point0.Index).Location,
		    way_points.get(point1.Index).Location,
		    &point1.LegDistance, &point1.InBound);

    if (_task->getSettings().AATEnabled) {
      DistanceBearing(point0.AATTargetLocation,
		      point1.AATTargetLocation,
		      &point1.LegDistance, &point1.LegBearing);
    } else {
      point1.LegBearing = point1.InBound;
    }
    point0.OutBound = point1.InBound;
    point0.Bisector = BiSector(point0.InBound, point0.OutBound);
    total_length += point1.LegDistance;
  };
  void visit_leg_final(TASK_POINT &point0, const unsigned index0,
		       TASK_POINT &point1, const unsigned index1)
  {
    visit_leg_intermediate(point0, index0, point1, index1);
  };
  void visit_leg_multistart(START_POINT &start, const unsigned index0, TASK_POINT &point)
  {
    start.OutBound = Bearing(way_points.get(start.Index).Location,
                             way_points.get(point.Index).Location);
  };
  double total_length;
};

class TaskLegPercentVisitor:
  public AbsoluteTaskLegVisitor {
public:
  TaskLegPercentVisitor(const TaskLegGeometryVisitor& geom) {
    total_length = geom.total_length;
  }
  void visit_single(TASK_POINT &point0, const unsigned index0) {
    point0.LengthPercent=1.0;
  };
  void visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
			      TASK_POINT &point1, const unsigned index1)
  {
    if (index0==0) {
      point0.LengthPercent=0;
    } else {
      point0.LengthPercent=point0.LegDistance/total_length;
    }
  };
  void visit_leg_final(TASK_POINT &point0, const unsigned index0,
		       TASK_POINT &point1, const unsigned index1)
  {
    visit_leg_intermediate(point0, index0, point1, index1);
  };
private:
  double total_length;
};


void Task::RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer)
{
  TaskLegGeometryVisitor geom;
  scan_leg_forward(geom);

  TaskLegPercentVisitor legpercent(geom);
  scan_leg_forward(legpercent);

  TaskSectorsVisitor sectors;
  scan_point_forward(sectors);

  WaypointsInTaskVisitor waypoint_in_task(settings_computer);
  scan_point_forward(waypoint_in_task);

}


class AATIsoLineVisitor: public RelativeTaskPointVisitor
{
public:
  virtual void visit_task_point_current(TASK_POINT &point, const unsigned i)
  {
    double stepsize = 25.0;
    if (!_task->ValidTaskPoint(i+1)) {
      // This must be the final waypoint, so it's not an AAT OZ
      return;
    }
    unsigned j;
    for (j=0; j<MAXISOLINES; j++) {
      point.IsoLine_valid[j]= false;
    }
    GEOPOINT location = point.AATTargetLocation;
    double dist_0, dist_north, dist_east;
    bool in_sector = true;

    double max_distance, delta;
    if(point.AATType == AAT_SECTOR) {
      max_distance = point.AATSectorRadius;
    } else {
      max_distance = point.AATCircleRadius;
    }
    delta = max_distance*2.4 / (MAXISOLINES);
    bool left = false;

    /*
      double distance_glider=0;
      if ((i==ActiveTaskPoint) && (CALCULATED_INFO.IsInSector)) {
      distance_glider = DoubleLegDistance(i, GPS_INFO.Longitude, GPS_INFO.Latitude);
      }
    */

    // fill
    j=0;
    // insert start point
    point.IsoLine_Location[j]= location;
    point.IsoLine_valid[j]= true;
    j++;

    do {
      dist_0 = _task->DoubleLegDistance(i, location);

      GEOPOINT loc_north;
      FindLatitudeLongitude(location,
			    0, stepsize,
			    &loc_north);
      dist_north = _task->DoubleLegDistance(i, loc_north);

      GEOPOINT loc_east;
      FindLatitudeLongitude(location,
			    90, stepsize,
			    &loc_east);
      dist_east = _task->DoubleLegDistance(i, loc_east);

      double angle = AngleLimit360(RAD_TO_DEG*atan2(dist_east-dist_0,
                                                    dist_north-dist_0)+90);
      if (left) {
	angle += 180;
      }

      FindLatitudeLongitude(location,
			    angle, delta,
			    &location);

      in_sector = _task->InAATTurnSector(location, i);
      /*
        if (dist_0 < distance_glider) {
	in_sector = false;
        }
      */
      if (in_sector) {
	point.IsoLine_Location[j] = location;
	point.IsoLine_valid[j] = true;
	j++;
      } else {
	j++;
	if (!left && (j<MAXISOLINES-2))  {
	  left = true;
	  location = point.AATTargetLocation;
	  in_sector = true; // cheat to prevent early exit

	  // insert start point (again)
	  point.IsoLine_Location[j] = location;
	  point.IsoLine_valid[j] = true;
	  j++;
	}
      }
    } while (in_sector && (j<MAXISOLINES));
  };
  virtual void visit_task_point_after(TASK_POINT &point, const unsigned i) {
    visit_task_point_current(point, i);
  };
};



void Task::CalculateAATIsoLines(void) {
  if (settings.AATEnabled) {
    AATIsoLineVisitor av;
    scan_point_forward(av);
  }
}
