// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Printing.hpp"
#include "Trace/Trace.hpp"
#include "system/FileUtil.hpp"
#include "Waypoint/Waypoint.hpp"
#include "util/ConvertString.hpp"

#include <fstream>

std::ostream &
operator<<(std::ostream &f, Path path)
{
  f << WideToUTF8Converter(path.c_str());
  return f;
}

std::ostream &
operator<< (std::ostream& f, const Waypoint& wp)
{
  f << wp.location.longitude << " " << wp.location.latitude << "\n";
  return f;
}

/*
#include "Geo/Flat/FlatBoundingBox.hpp"

void 
FlatBoundingBox::print(std::ostream &f, const TaskProjection &task_projection) const {
  FlatGeoPoint ll(bb_ll.longitude,bb_ll.latitude);
  FlatGeoPoint lr(bb_ur.longitude,bb_ll.latitude);
  FlatGeoPoint ur(bb_ur.longitude,bb_ur.latitude);
  FlatGeoPoint ul(bb_ll.longitude,bb_ur.latitude);
  GeoPoint gll = task_projection.unproject(ll);
  GeoPoint glr = task_projection.unproject(lr);
  GeoPoint gur = task_projection.unproject(ur);
  GeoPoint gul = task_projection.unproject(ul);
  
  f << gll.longitude << " " << gll.latitude << "\n";
  f << glr.longitude << " " << glr.latitude << "\n";
  f << gur.longitude << " " << gur.latitude << "\n";
  f << gul.longitude << " " << gul.latitude << "\n";
  f << gll.longitude << " " << gll.latitude << "\n";
  f << "\n";
}
*/

/*
void
TaskMacCready::print(std::ostream &f, const AIRCRAFT_STATE &aircraft) const
{
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);
  AIRCRAFT_STATE aircraft_predict = aircraft;
  aircraft_predict.Altitude = aircraft_start.Altitude;
  f << "#  i alt  min  elev\n";
  f << start-0.5 << " " << aircraft_start.Altitude << " " <<
    minHs[start] << " " <<
    task_points[start]->get_elevation() << "\n";
  for (int i=start; i<=end; i++) {
    aircraft_predict.Altitude -= gs[i].HeightGlide;
    f << i << " " << aircraft_predict.Altitude << " " << minHs[i]
      << " " << task_points[i]->get_elevation() << "\n";
  }
  f << "\n";
}
*/

static void
PrintTracePoint(const TracePoint &point, std::ofstream& fs)
{
  fs << point.GetTime().count()
     << " " << point.GetLocation().longitude
     << " " << point.GetLocation().latitude
     << " " << point.GetAltitude()
     << " " << point.GetVario()
     << "\n";
}

void
PrintHelper::trace_print([[maybe_unused]] const Trace& trace,
			 [[maybe_unused]] const GeoPoint &loc)
{
  Directory::Create(Path("output/results"));
  std::ofstream fs("output/results/res-trace.txt");

  for (auto it = trace.begin(); it != trace.end(); ++it)
    PrintTracePoint(*it, fs);
}


#include "Math/Angle.hpp"

std::ostream& operator<< (std::ostream& o, Angle a)
{
  o << a.Degrees();
  return o;
} 

#include "Route/AirspaceRoute.hpp"

void PrintHelper::print_route(RoutePlanner& r)
{
  for (auto i = r.solution_route.begin(); i != r.solution_route.end(); ++i) {
    printf("%.6g %.6g %d # solution\n",
           (double)i->longitude.Degrees(),
           (double)i->latitude.Degrees(),
           0);
  }
  printf("# solution\n");
  for (auto i = r.solution_route.begin(); i != r.solution_route.end(); ++i) {
    printf("%.6g %.6g %d # solution\n",
           (double)i->longitude.Degrees(),
           (double)i->latitude.Degrees(),
           (int)i->altitude);
  }
}

#include "Route/ReachFan.hpp"

void
PrintHelper::print(const ReachFan& r)
{
  print(r.root);
}

void
PrintHelper::print(const FlatTriangleFanTree& r) {
  print((const FlatTriangleFan&)r, r.depth);

  for (auto it = r.children.begin(); it != r.children.end(); ++it) {
    print(*it);
  }
};

void
PrintHelper::print(const FlatTriangleFan& r, const unsigned depth) {
  if (r.vs.size()<3)
    return;

  if (depth) {
    printf("%d %d # fcorner\n", r.vs[0].x, r.vs[0].y);
  }

  for (auto it = r.vs.begin(); it != r.vs.end(); ++it) {
    const FlatGeoPoint p = (*it);
    printf("%d %d # ftri\n", p.x, p.y);
  }
  printf("%d %d # ftri\n", r.vs[0].x, r.vs[0].y);
  printf("# ftri\n");
}

