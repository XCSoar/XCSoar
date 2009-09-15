
#include "TaskVisitor.hpp"
#include <stdio.h>
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Protection.hpp"
#include <math.h>
#include "Math/FastMath.h"
#include "Units.hpp"
#include "Components.hpp"
#include "WayPointList.hpp"

void TaskScan::scan_point_forward(RelativeTaskPointVisitor &visitor)
{
  ScopeLock protect(mutexTaskData);

  visitor.visit_reset();
  if (EnableMultipleStartPoints) {
    for (int j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_start_point(task_start_points[j], j);
    }
  }
  if ((ActiveTaskPoint<0)||(task_points[0].Index<0)) {
    visitor.visit_null();
    return;
  }

  int i=0;
  while (i<ActiveTaskPoint) {
    visitor.visit_task_point_before(task_points[i], i);
    i++;
  } 
  visitor.visit_task_point_current(task_points[i], i);
  i++;
  while ((i<MAXTASKPOINTS) && (task_points[i].Index != -1)) {
    visitor.visit_task_point_after(task_points[i], i);
    i++;
  }
}


void TaskScan::scan_point_forward(AbsoluteTaskPointVisitor &visitor)
{
  ScopeLock protect(mutexTaskData);

  visitor.visit_reset();

  if (EnableMultipleStartPoints) {
    for (int j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_start_point(task_start_points[j], j);
    }
  }

  if (task_points[0].Index<0) {
    visitor.visit_null();
    return;
  }
  int i=0;
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


void TaskScan::scan_leg_forward(RelativeTaskLegVisitor &visitor)
{
  ScopeLock protect(mutexTaskData);

  visitor.visit_reset();

  if ((ActiveTaskPoint<0)||(task_points[0].Index<0)) {
    visitor.visit_null();
    return;
  }
  if (task_points[1].Index<0) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  int i=0;
  while ((i+1<MAXTASKPOINTS) && (task_points[i+1].Index != -1)) {
    if (i+1<ActiveTaskPoint) {
      visitor.visit_leg_before(task_points[i], i,
				task_points[i+1], i+1);
    } else if (i+1==ActiveTaskPoint) {
      visitor.visit_leg_current(task_points[i], i,
				 task_points[i+1], i+1);
    } else {
      visitor.visit_leg_after(task_points[i], i,
			       task_points[i+1], i+1);
    }
    i++;
  }
}




void TaskScan::scan_leg_forward(AbsoluteTaskLegVisitor &visitor)
{
  ScopeLock protect(mutexTaskData);

  visitor.visit_reset();

  if (task_points[0].Index<0) {
    visitor.visit_null();
    return;
  }
  if (EnableMultipleStartPoints) {
    for (int j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_leg_multistart(task_start_points[j], j, task_points[0]);
    }
  }
  if (task_points[1].Index<0) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  int i=0;
  while ((i+2<MAXTASKPOINTS) && (task_points[i+2].Index != -1)) {
    visitor.visit_leg_intermediate(task_points[i], i,
				   task_points[i+1], i+1);
    i++;
  }
  visitor.visit_leg_final(task_points[i], i,
			  task_points[i+1], i+1);
}


void TaskScan::scan_leg_reverse(AbsoluteTaskLegVisitor &visitor)
{
  ScopeLock protect(mutexTaskData);

  visitor.visit_reset();

  if (task_points[0].Index<0) {
    visitor.visit_null();
    return;
  }
  if (task_points[1].Index<0) {
    visitor.visit_single(task_points[0], 0);
    return;
  }
  int final = getFinalWaypoint();
  int i= final-1;
  while (i>=0) {
    if (i==final-1) {
      visitor.visit_leg_final(task_points[i], i,
			      task_points[i+1], i+1);
    } else {
      visitor.visit_leg_intermediate(task_points[i], i,
				     task_points[i+1], i+1);
    }
    i--;
  }

  if (EnableMultipleStartPoints) {
    for (int j=0; j<MAXSTARTPOINTS; j++) {
      if ((task_start_points[j].Index != -1) && task_start_stats[j].Active)
	visitor.visit_leg_multistart(task_start_points[j], j, task_points[0]);
    }
  }
}


////////////////////////////////////////////////////////////////

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


/////////////////////////

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

////////////////////////////


#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Waypointparser.h"

class TaskSectorsVisitor:
  public AbsoluteTaskPointVisitor {
public:
  void visit_task_point_start(TASK_POINT &point, const unsigned index) { 
    if (StartLine==2) {
      SectorAngle = 45+90;
    } else {
      SectorAngle = 90;
    }
    SectorSize = StartRadius;
    SectorBearing = point.OutBound;
    setStartEnd(point);
    clearAAT(point, index);
  };
  void visit_task_point_intermediate(TASK_POINT &point, const unsigned index) { 
    SectorAngle = 45;
    if (SectorType == 2) {
      SectorSize = 10000; // German DAe 0.5/10
    } else {
      SectorSize = SectorRadius;  // FAI sector
    }
    SectorBearing = point.Bisector;    
    if (!AATEnabled) {
      point.AATStartRadial  = AngleLimit360(SectorBearing - SectorAngle);
      point.AATFinishRadial = AngleLimit360(SectorBearing + SectorAngle);
    }
    setStartEnd(point);
  };
  void visit_task_point_final(TASK_POINT &point, const unsigned index) { 
    // finish line
    if (FinishLine==2) {
      SectorAngle = 45;
    } else {
      SectorAngle = 90;
    }
    SectorSize = FinishRadius;
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
  void clearAAT(const TASK_POINT &point, const unsigned i) {
    task_stats[i].AATTargetOffsetRadius = 0.0;
    task_stats[i].AATTargetOffsetRadial = 0.0;
    task_stats[i].AATTargetLocation = way_points.get(point.Index).Location;
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
    if (true) {
      for (unsigned i = 0; way_points.verify_index(i); i++)
        way_points.set_calc(i).InTask =
          (way_points.get(i).Flags & HOME) == HOME;

      if (ValidWayPoint(settings_computer->HomeWaypoint)) {
        way_points.set_calc(settings_computer->HomeWaypoint).InTask = true;
      }
    }
  }
  void visit_start_point(START_POINT &point, const unsigned i) { 
    way_points.set_calc(task_start_points[i].Index).InTask = true;
  }
  void visit_task_point_start(TASK_POINT &point, const unsigned i) { 
    addTaskPoint(i);
  }
  void visit_task_point_intermediate(TASK_POINT &point, const unsigned i) { 
    addTaskPoint(i);
  }
  void visit_task_point_final(TASK_POINT &point, const unsigned i) { 
    addTaskPoint(i);
  }
private:
  const SETTINGS_COMPUTER *settings_computer;
  void addTaskPoint(const unsigned i) {
    way_points.set_calc(task_points[i].Index).InTask = true;
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

    if (AATEnabled) {
      DistanceBearing(task_stats[index0].AATTargetLocation,
		      task_stats[index1].AATTargetLocation,
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
    task_stats[index0].LengthPercent=1.0;
  };
  void visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
			      TASK_POINT &point1, const unsigned index1) 
  {
    if (index0==0) {
      task_stats[index0].LengthPercent=0;
    } else {
      task_stats[index0].LengthPercent=point0.LegDistance/total_length;
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




void RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer) 
{
  ScopeLock protect(mutexTaskData);
  if ((ActiveTaskPoint<0)&&(task_points[0].Index>=0)) {
    ActiveTaskPoint=0;
  }

  TaskLegGeometryVisitor geom;
  TaskScan::scan_leg_forward(geom);

  TaskLegPercentVisitor legpercent(geom);
  TaskScan::scan_leg_forward(legpercent);

  TaskSectorsVisitor sectors;
  TaskScan::scan_point_forward(sectors);

  WaypointsInTaskVisitor waypoint_in_task(settings_computer);
  TaskScan::scan_point_forward(waypoint_in_task);

}




class AATIsoLineVisitor: public RelativeTaskPointVisitor 
{
public:
  virtual void visit_task_point_current(TASK_POINT &point, const unsigned i) 
  { 
    double stepsize = 25.0;
    if (!ValidTaskPoint(i+1)) {
      // This must be the final waypoint, so it's not an AAT OZ
      return;
    }
    int j;
    for (j=0; j<MAXISOLINES; j++) {
      task_stats[i].IsoLine_valid[j]= false;
    }
    GEOPOINT location = task_stats[i].AATTargetLocation;
    double dist_0, dist_north, dist_east;
    bool in_sector = true;
    
    double max_distance, delta;
    if(point.AATType == SECTOR) {
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
    task_stats[i].IsoLine_Location[j]= location;
    task_stats[i].IsoLine_valid[j]= true;
    j++;
    
    do {
      dist_0 = DoubleLegDistance(i, location);
      
      GEOPOINT loc_north;
      FindLatitudeLongitude(location,
			    0, stepsize,
			    &loc_north);
      dist_north = DoubleLegDistance(i, loc_north);
      
      GEOPOINT loc_east;
      FindLatitudeLongitude(location,
			    90, stepsize,
			    &loc_east);
      dist_east = DoubleLegDistance(i, loc_east);
      
      double angle = AngleLimit360(RAD_TO_DEG*atan2(dist_east-dist_0, dist_north-dist_0)+90);
      if (left) {
	angle += 180;
      }
      
      FindLatitudeLongitude(location,
			    angle, delta,
			    &location);
      
      in_sector = InAATTurnSector(location, i);
      /*
        if (dist_0 < distance_glider) {
	in_sector = false;
        }
      */
      if (in_sector) {
	task_stats[i].IsoLine_Location[j] = location;
	task_stats[i].IsoLine_valid[j] = true;
	j++;
      } else {
	j++;
	if (!left && (j<MAXISOLINES-2))  {
	  left = true;
	  location = task_stats[i].AATTargetLocation;
	  in_sector = true; // cheat to prevent early exit
	  
	  // insert start point (again)
	  task_stats[i].IsoLine_Location[j] = location;
	  task_stats[i].IsoLine_valid[j] = true;
	  j++;
	}
      }
    } while (in_sector && (j<MAXISOLINES));
  };
  virtual void visit_task_point_after(TASK_POINT &point, const unsigned i) { 
    visit_task_point_current(point, i);
  };
};



void CalculateAATIsoLines(void) {
  if(AATEnabled) {
    AATIsoLineVisitor av;
    TaskScan::scan_point_forward(av);
  }  
}
