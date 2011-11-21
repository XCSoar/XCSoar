/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Printing.hpp"
#include "harness_airspace.hpp"
#include "test_debug.hpp"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceNearestSort.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

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
  airspaces.insert(as);

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
    airspaces.insert(as);
    if (fin)
      *fin << *as;
  }

  delete fin;

  // try inserting nothing
  airspaces.insert(NULL);

  airspaces.optimise();

}


class AirspaceVisitorPrint: 
  public AirspaceVisitor {
public:
  AirspaceVisitorPrint(const char* fname,
                       const bool _do_report):
    do_report(_do_report)
    {      
      if (do_report) {
        fout = new std::ofstream(fname);
      }
    };
  ~AirspaceVisitorPrint() {
    if (do_report) {
      delete fout;
    }
  }
  virtual void visit_general(const AbstractAirspace& as) {
    if (do_report) {
      *fout << "# Name: " << as.GetNameText().c_str()
            << " " << as.GetVerticalText().c_str()
            << "\n";
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
      *fout << as;
      visit_general(as);
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
      *fout << as;
      visit_general(as);
    }
  }
private:
  std::ofstream *fout;
  const bool do_report;
};


class AirspaceIntersectionVisitorPrint: 
  public AirspaceIntersectionVisitor {
public:
  AirspaceIntersectionVisitorPrint(const char* fname,
                                   const char* yname,
                                   const char* iname,
                                   const bool _do_report,
                                   const AircraftState &state,
                                   const AirspaceAircraftPerformance &perf):
    do_report(_do_report),
    m_state(state),
    m_perf(perf)
    {      
      if (do_report) {
        fout = new std::ofstream(fname);
        iout = new std::ofstream(iname);
        yout = new std::ofstream(yname);
      }
    };
  ~AirspaceIntersectionVisitorPrint() {
    if (do_report) {
      delete fout;
      delete iout;
      delete yout;
    }
  }
  virtual void intersection(const AbstractAirspace& as) {
    *fout << "# intersection point\n";
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GeoPoint start = (it->first);
      const GeoPoint end = (it->second);
      *fout << start.longitude << " " << start.latitude << " " << "\n";
      *fout << end.longitude << " " << end.latitude << " " << "\n\n";
    }

    AirspaceInterceptSolution solution = intercept(as, m_state, m_perf);
    if (solution.IsValid()) {
      *iout << "# intercept " << solution.elapsed_time << " h " << solution.altitude << "\n";
      *iout << solution.location.longitude << " " << solution.location.latitude << " " << "\n\n";
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
      *yout << as;
      intersection(as);
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
      *yout << as;
      intersection(as);
    }
  }
private:
  std::ofstream *fout;
  std::ofstream *yout;
  std::ofstream *iout;
  const bool do_report;
  const AircraftState m_state;
  const AirspaceAircraftPerformance &m_perf;
};


class AirspaceVisitorClosest: public AirspaceVisitor {
public:
  AirspaceVisitorClosest(const char* fname,
                         const AircraftState &_state,
                         const AirspaceAircraftPerformance &perf):
    fout(NULL),
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
  virtual void closest(const AbstractAirspace& as) {
    GeoPoint c = as.ClosestPoint(state.location);
    if (fout) {
      *fout << "# closest point\n";
      *fout << c.longitude << " " << c.latitude << " " << "\n";
      *fout << state.location.longitude << " " << state.location.latitude << " " << "\n\n";
    }
    AirspaceInterceptSolution solution;
    GeoVector vec(state.location, c);
    vec.distance = fixed(20000); // set big distance (for testing)
    if (as.Intercept(state, vec, m_perf, solution)) {
      if (fout) {
        *fout << "# intercept in " << solution.elapsed_time << " h " << solution.altitude << "\n";
      }
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    closest(as);
  }
  virtual void Visit(const AirspacePolygon& as) {
    closest(as);
  }
private:
  std::ofstream *fout;
  const AircraftState& state;
  const AirspaceAircraftPerformance &m_perf;
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

  const std::vector<Airspace> vn = airspaces.scan_nearest(state.location);
  AirspaceVisitorPrint pvn("results/res-bb-nearest.txt",
                           do_report);
  std::for_each(vn.begin(), vn.end(), CallVisitor<AirspaceVisitor>(pvn));

  {
    AirspaceVisitorPrint pvisitor("results/res-bb-range.txt",
                                  do_report);
    airspaces.visit_within_range(state.location, range, pvisitor);
  }

  {
    AirspaceVisitorClosest pvisitor("results/res-bb-closest.txt",
                                    state, perf);
    airspaces.visit_within_range(state.location, range, pvisitor);
  }

  {
    const std::vector<Airspace> vi = airspaces.find_inside(state);
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
    GeoVector vec(state.location, target);
    airspaces.visit_intersecting(state.location, vec, ivisitor);
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



#include "Airspace/AirspaceWarningVisitor.hpp"

class AirspaceWarningPrint: public AirspaceWarningVisitor
{
public:
  AirspaceWarningPrint(const char* fname,
                       const AirspaceWarning::State state):m_state(state) {
    fout = new std::ofstream(fname);
  }
  ~AirspaceWarningPrint() {
    delete fout;
  }
  virtual void Visit(const AirspaceWarning& as) {
    if (as.get_warning_state() == m_state) {
    *fout << as;
    *fout << as.get_airspace();
    }
  }
private:
  std::ofstream *fout;
  AirspaceWarning::State m_state;
};

void
print_warnings(const AirspaceWarningManager &airspace_warnings)
{
  AirspaceWarningPrint visitor_inside("results/res-as-warnings-inside.txt",
                                      AirspaceWarning::WARNING_INSIDE);
  AirspaceWarningPrint visitor_glide("results/res-as-warnings-glide.txt",
                                     AirspaceWarning::WARNING_GLIDE);
  AirspaceWarningPrint visitor_filter("results/res-as-warnings-filter.txt",
                                      AirspaceWarning::WARNING_FILTER);
  AirspaceWarningPrint visitor_task("results/res-as-warnings-task.txt",
                                    AirspaceWarning::WARNING_TASK);
  airspace_warnings.visit_warnings(visitor_inside);
  airspace_warnings.visit_warnings(visitor_glide);
  airspace_warnings.visit_warnings(visitor_filter);
  airspace_warnings.visit_warnings(visitor_task);
}
