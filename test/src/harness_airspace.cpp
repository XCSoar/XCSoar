/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "AirspacePrinting.hpp"
#include "Printing.hpp"
#include "harness_airspace.hpp"
#include "test_debug.hpp"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceNearestSort.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"
#include "Geo/GeoVector.hpp"
#include "Formatter/AirspaceFormatter.hpp"

static void
airspace_random_properties(AbstractAirspace& as)
{
  AirspaceClass Type = (AirspaceClass)(rand()%14);
  AirspaceAltitude base;
  AirspaceAltitude top;
  base.altitude = fixed(rand()%4000);
  top.altitude = base.altitude+fixed(rand()%3000);
  as.SetProperties(_T("hello"), Type, base, top);
}


bool test_airspace_extra(Airspaces &airspaces) {
  // try adding a null polygon

  AbstractAirspace* as;
  std::vector<GeoPoint> pts;
  as = new AirspacePolygon(pts);
  airspaces.Add(as);

  // try clearing now (we haven't called optimise())

  airspaces.clear();
  return true;
}

void setup_airspaces(Airspaces& airspaces, const GeoPoint& center, const unsigned n) {
  std::ofstream *fin = NULL;

  if (verbose)
    fin = new std::ofstream("results/res-bb-in.txt");

  for (unsigned i=0; i<n; i++) {
    AbstractAirspace* as;
    if (rand()%4!=0) {
      GeoPoint c;
      c.longitude = Angle::Degrees(fixed((rand()%1200-600)/1000.0))+center.longitude;
      c.latitude = Angle::Degrees(fixed((rand()%1200-600)/1000.0))+center.latitude;
      fixed radius(10000.0*(0.2+(rand()%12)/12.0));
      as = new AirspaceCircle(c,radius);
    } else {

      // just for testing, create a random polygon from a convex hull around
      // random points
      const unsigned num = rand()%10+5;
      GeoPoint c;
      c.longitude = Angle::Degrees(fixed((rand()%1200-600)/1000.0))+center.longitude;
      c.latitude = Angle::Degrees(fixed((rand()%1200-600)/1000.0))+center.latitude;

      std::vector<GeoPoint> pts;
      for (unsigned j=0; j<num; j++) {
        GeoPoint p=c;
        p.longitude += Angle::Degrees(fixed((rand()%200)/1000.0));
        p.latitude += Angle::Degrees(fixed((rand()%200)/1000.0));
        pts.push_back(p);
      }
      as = new AirspacePolygon(pts,true);
    }
    airspace_random_properties(*as);
    airspaces.Add(as);
    if (fin)
      *fin << *as;
  }

  delete fin;

  // try inserting nothing
  airspaces.Add(NULL);

  airspaces.Optimise();

}


class AirspaceVisitorPrint final : public AirspaceVisitor {
  std::ofstream *fout;
  const bool do_report;

public:
  AirspaceVisitorPrint(const char *fname, const bool _do_report)
    :do_report(_do_report)
  {
    if (do_report) {
      fout = new std::ofstream(fname);
    }
  }

  ~AirspaceVisitorPrint() {
    if (do_report) {
      delete fout;
    }
  }

  virtual void Visit(const AbstractAirspace &as) override {
    if (do_report) {
      *fout << as;
      *fout << "# Name: " << as.GetName()
            << "Base: " << as.GetBase()
            << " Top: " << as.GetTop()
            << "\n";
    }
  }
};


class AirspaceIntersectionVisitorPrint final
  : public AirspaceIntersectionVisitor {
  std::ofstream *fout;
  std::ofstream *yout;
  std::ofstream *iout;
  const bool do_report;
  const AircraftState m_state;
  const AirspaceAircraftPerformance &m_perf;

public:
  AirspaceIntersectionVisitorPrint(const char* fname,
                                   const char* yname,
                                   const char* iname,
                                   const bool _do_report,
                                   const AircraftState &state,
                                   const AirspaceAircraftPerformance &perf)
    :do_report(_do_report),
     m_state(state),
     m_perf(perf)
  {
    if (do_report) {
      fout = new std::ofstream(fname);
      iout = new std::ofstream(iname);
      yout = new std::ofstream(yname);
    }
  }

  ~AirspaceIntersectionVisitorPrint() {
    if (do_report) {
      delete fout;
      delete iout;
      delete yout;
    }
  }

  void intersection(const AbstractAirspace &as) {
    *fout << "# intersection point\n";
    for (auto it = intersections.begin(); it != intersections.end(); ++it) {
      const GeoPoint start = (it->first);
      const GeoPoint end = (it->second);
      *fout << start.longitude << " " << start.latitude << " " << "\n";
      *fout << end.longitude << " " << end.latitude << " " << "\n\n";
    }

    AirspaceInterceptSolution solution = Intercept(as, m_state, m_perf);
    if (solution.IsValid()) {
      *iout << "# intercept " << solution.elapsed_time << " h " << solution.altitude << "\n";
      *iout << solution.location.longitude << " " << solution.location.latitude << " " << "\n\n";
    }
  }

  virtual void Visit(const AbstractAirspace &as) override {
    if (do_report) {
      *yout << as;
      intersection(as);
    }
  }
};


class AirspaceVisitorClosest final : public AirspaceVisitor {
  std::ofstream *fout;
  const TaskProjection &projection;
  const AircraftState& state;
  const AirspaceAircraftPerformance &m_perf;

public:
  AirspaceVisitorClosest(const char* fname,
                         const TaskProjection &_projection,
                         const AircraftState &_state,
                         const AirspaceAircraftPerformance &perf):
    fout(NULL),
    projection(_projection),
    state(_state),
    m_perf(perf)
    {
      if (verbose)
        fout = new std::ofstream(fname);
    };
  ~AirspaceVisitorClosest() {
    if (fout)
      delete fout;
  }

  void closest(const AbstractAirspace &as) {
    GeoPoint c = as.ClosestPoint(state.location, projection);
    if (fout) {
      *fout << "# closest point\n";
      *fout << c.longitude << " " << c.latitude << " " << "\n";
      *fout << state.location.longitude << " " << state.location.latitude << " " << "\n\n";
    }
    AirspaceInterceptSolution solution;
    GeoVector vec(state.location, c);
    vec.distance = fixed(20000); // set big distance (for testing)
    if (as.Intercept(state, vec.EndPoint(state.location), projection, m_perf, solution)) {
      if (fout) {
        *fout << "# intercept in " << solution.elapsed_time << " h " << solution.altitude << "\n";
      }
    }
  }

  virtual void Visit(const AbstractAirspace &as) override {
    closest(as);
  }
};

/**
 * Adapter between an AirspaceVisitor and a function class.
 */
template<typename V>
struct CallVisitor {
  V &visitor;

  CallVisitor(V &_visitor):visitor(_visitor) {}

  template<typename T>
  void operator()(const T &t) {
    return visitor.Visit(t);
  }
};

void scan_airspaces(const AircraftState state, 
                    const Airspaces& airspaces,
                    const AirspaceAircraftPerformance& perf,
                    bool do_report,
                    const GeoPoint &target) 
{
  const fixed range(20000.0);

  AirspaceVisitorPrint pvn("results/res-bb-nearest.txt",
                           do_report);
  const Airspace *nearest = airspaces.FindNearest(state.location);
  if (nearest != nullptr) {
    AirspaceVisitor &v = pvn;
    v.Visit(*nearest);
  }

  {
    AirspaceVisitorPrint pvisitor("results/res-bb-range.txt",
                                  do_report);
    airspaces.VisitWithinRange(state.location, range, pvisitor);
  }

  {
    AirspaceVisitorClosest pvisitor("results/res-bb-closest.txt",
                                    airspaces.GetProjection(), state, perf);
    airspaces.VisitWithinRange(state.location, range, pvisitor);
  }

  {
    const std::vector<Airspace> vi = airspaces.FindInside(state);
    AirspaceVisitorPrint pvi("results/res-bb-inside.txt",
                             do_report);
    std::for_each(vi.begin(), vi.end(), CallVisitor<AirspaceVisitor>(pvi));
  }
  
  {
    AirspaceIntersectionVisitorPrint ivisitor("results/res-bb-intersects.txt",
                                              "results/res-bb-intersected.txt",
                                              "results/res-bb-intercepts.txt",
                                              do_report,
                                              state, perf);
    airspaces.VisitIntersecting(state.location, target, ivisitor);
  }

  {
    AirspaceNearestSort ans(state.location);
    const AbstractAirspace* as = ans.find_nearest(airspaces, range);
    if (do_report) {
      std::ofstream fout("results/res-bb-sortednearest.txt");
      if (as) {
        fout << *as << "\n";
      } else {
        fout << "# no nearest found\n";
      }
    }
  }

  {
    AirspaceSoonestSort ans(state, perf);
    const AbstractAirspace* as = ans.find_nearest(airspaces);
    if (do_report) {
      std::ofstream fout("results/res-bb-sortedsoonest.txt");
      if (as) {
        fout << *as << "\n";
      } else {
        fout << "# no soonest found\n";
      }
    }
  }
}

static void
PrintAirspaceWarnings(const char *path,
                      const AirspaceWarningManager &warnings,
                      const AirspaceWarning::State state)
{
  std::ofstream fout(path);

  for (auto i = warnings.begin(), end = warnings.end(); i != end; ++i) {
    const AirspaceWarning &warning = *i;
    if (warning.GetWarningState() == state) {
      fout << warning;
      fout << warning.GetAirspace();
    }
  }
}

void
print_warnings(const AirspaceWarningManager &airspace_warnings)
{
  PrintAirspaceWarnings("results/res-as-warnings-inside.txt",
                        airspace_warnings, AirspaceWarning::WARNING_INSIDE);
  PrintAirspaceWarnings("results/res-as-warnings-glide.txt",
                        airspace_warnings, AirspaceWarning::WARNING_GLIDE);
  PrintAirspaceWarnings("results/res-as-warnings-filter.txt",
                        airspace_warnings, AirspaceWarning::WARNING_FILTER);
  PrintAirspaceWarnings("results/res-as-warnings-task.txt",
                        airspace_warnings, AirspaceWarning::WARNING_TASK);
}
