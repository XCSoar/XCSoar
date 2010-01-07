#include "harness_airspace.hpp"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceNearestSort.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"


void airspace_random_properties(AbstractAirspace& as) {
  AirspaceClass_t Type = (AirspaceClass_t)(rand()%15);
  AIRSPACE_ALT base;
  AIRSPACE_ALT top;
  base.Altitude = rand()%2000;
  top.Altitude = base.Altitude+rand()%3000;
  as.set_properties(_T("hello"), Type, base, top);
}


bool test_airspace_extra(Airspaces &airspaces) {
  // try adding a null polygon

  AbstractAirspace* as;
  std::vector<GEOPOINT> pts;
  as = new AirspacePolygon(pts);
  airspaces.insert(as);

  // try clearing now (we haven't called optimise())

  airspaces.clear();
  return true;
}

void setup_airspaces(Airspaces& airspaces, const unsigned n) {
#ifdef DO_PRINT
  std::ofstream fin("results/res-bb-in.txt");
#endif
  for (unsigned i=0; i<n; i++) {
    AbstractAirspace* as;
    if (rand()%4!=0) {
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      fixed radius(10000.0*(0.2+(rand()%12)/12.0));
      as = new AirspaceCircle(c,radius);
    } else {

      // just for testing, create a random polygon from a convex hull around
      // random points
      const unsigned num = rand()%10+5;
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      
      std::vector<GEOPOINT> pts;
      for (unsigned j=0; j<num; j++) {
        GEOPOINT p=c;
        p.Longitude += fixed((rand()%200)/1000.0);
        p.Latitude += fixed((rand()%200)/1000.0);
        pts.push_back(p);
      }
      as = new AirspacePolygon(pts,true);
    }
    airspace_random_properties(*as);
    airspaces.insert(as);
#ifdef DO_PRINT
    fin << *as;
#endif
  }

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
#ifdef DO_PRINT
        fout = new std::ofstream(fname);
#endif
      }
    };
  ~AirspaceVisitorPrint() {
#ifdef DO_PRINT
    if (do_report) {
      delete fout;
    }
#endif    
  }
  virtual void visit_general(const AbstractAirspace& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << "# Name: " << as.get_name_text() 
            << " Base: " << as.get_base_text() 
            << " Top: " << as.get_top_text() << "\n";
#endif
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
      visit_general(as);
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
      visit_general(as);
    }
  }
private:
#ifdef DO_PRINT
  std::ofstream *fout;
#endif
  const bool do_report;
};


class AirspaceIntersectionVisitorPrint: 
  public AirspaceIntersectionVisitor {
public:
  AirspaceIntersectionVisitorPrint(const char* fname,
                                   const char* yname,
                                   const char* iname,
                                   const bool _do_report,
                                   const AIRCRAFT_STATE &state,
                                   const AirspaceAircraftPerformance &perf):
    do_report(_do_report),
    m_state(state),
    m_perf(perf)
    {      
      if (do_report) {
#ifdef DO_PRINT
        fout = new std::ofstream(fname);
        iout = new std::ofstream(iname);
        yout = new std::ofstream(yname);
#endif
      }
    };
  ~AirspaceIntersectionVisitorPrint() {
#ifdef DO_PRINT
    if (do_report) {
      delete fout;
      delete iout;
      delete yout;
    }
#endif    
  }
  virtual void intersection(const AbstractAirspace& as) {
#ifdef DO_PRINT
    *fout << "# intersection point\n";
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GEOPOINT start = (it->first);
      const GEOPOINT end = (it->second);
      *fout << start.Longitude << " " << start.Latitude << " " << "\n";
      *fout << end.Longitude << " " << end.Latitude << " " << "\n\n";
    }

    AirspaceInterceptSolution solution = intercept(as, m_state, m_perf);
    if (solution.valid()) {
      *iout << "# intercept " << solution.elapsed_time << " h " << solution.altitude << "\n";
      *iout << solution.location.Longitude << " " << solution.location.Latitude << " " << "\n\n";
    }
#endif
  }
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
#ifdef DO_PRINT
      *yout << as;
#endif
      intersection(as);
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
#ifdef DO_PRINT
      *yout << as;
#endif
      intersection(as);
    }
  }
private:
#ifdef DO_PRINT
  std::ofstream *fout;
  std::ofstream *yout;
  std::ofstream *iout;
#endif
  const bool do_report;
  const AIRCRAFT_STATE m_state;
  const AirspaceAircraftPerformance &m_perf;
};


class AirspaceVisitorClosest: public AirspaceVisitor {
public:
  AirspaceVisitorClosest(const char* fname,
                         const AIRCRAFT_STATE &_state,
                         const AirspaceAircraftPerformance &perf):
    state(_state),
    m_perf(perf)
    {      
#ifdef DO_PRINT
      fout = new std::ofstream(fname);
#endif
    };
  ~AirspaceVisitorClosest() {
#ifdef DO_PRINT
    delete fout;
#endif    
  }
  virtual void closest(const AbstractAirspace& as) {
    GEOPOINT c = as.closest_point(state.Location);
#ifdef DO_PRINT
    *fout << "# closest point\n";
    *fout << c.Longitude << " " << c.Latitude << " " << "\n";
    *fout << state.Location.Longitude << " " << state.Location.Latitude << " " << "\n\n";
#endif
    AirspaceInterceptSolution solution;
    GeoVector vec(state.Location, c);
    vec.Distance = 20000; // set big distance (for testing)
    if (as.intercept(state, vec, m_perf, solution)) {
#ifdef DO_PRINT
      *fout << "# intercept in " << solution.elapsed_time << " h " << solution.altitude << "\n";
#endif
    }
  }
  virtual void Visit(const AirspaceCircle& as) {
    closest(as);
  }
  virtual void Visit(const AirspacePolygon& as) {
    closest(as);
  }
private:
#ifdef DO_PRINT
  std::ofstream *fout;
#endif
  const AIRCRAFT_STATE& state;
  const AirspaceAircraftPerformance &m_perf;
};


void scan_airspaces(const AIRCRAFT_STATE state, 
                    const Airspaces& airspaces,
                    const AirspaceAircraftPerformance& perf,
                    bool do_report,
                    const GEOPOINT &target) 
{
  const fixed range(20000.0);

  const std::vector<Airspace> vn = airspaces.scan_nearest(state);
  AirspaceVisitorPrint pvn("results/res-bb-nearest.txt",
                           do_report);
  pvn.for_each(vn);

//  std::for_each(vn.begin(), vn.end(), pvn);
// (will work for simple cases where visitor is stateless)

  {
    AirspaceVisitorPrint pvisitor("results/res-bb-range.txt",
                                  do_report);
    airspaces.visit_within_range(state.Location, range, pvisitor);
  }

  {
    AirspaceVisitorClosest pvisitor("results/res-bb-closest.txt",
                                    state, perf);
    airspaces.visit_within_range(state.Location, range, pvisitor);
  }

  {
    const std::vector<Airspace> vi = airspaces.find_inside(state);
    AirspaceVisitorPrint pvi("results/res-bb-inside.txt",
                             do_report);
    pvi.for_each(vi);
  }
  
  {
    AirspaceIntersectionVisitorPrint ivisitor("results/res-bb-intersects.txt",
                                              "results/res-bb-intersected.txt",
                                              "results/res-bb-intercepts.txt",
                                              do_report,
                                              state, perf);
    GeoVector vec(state.Location, target);
    airspaces.visit_intersecting(state.Location, vec, ivisitor);
  }

  {
    AirspaceNearestSort ans(state);
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
                       const AirspaceWarning::AirspaceWarningState state):m_state(state) {
#ifdef DO_PRINT
    fout = new std::ofstream(fname);
#endif
  }
  ~AirspaceWarningPrint() {
#ifdef DO_PRINT
    delete fout;
#endif
  }
  virtual void Visit(const AirspaceWarning& as) {
    if (as.get_warning_state() == m_state) {
#ifdef DO_PRINT
    *fout << as;
    *fout << as.get_airspace();
#endif
    }
  }
private:
#ifdef DO_PRINT
  std::ofstream *fout;
#endif
  AirspaceWarning::AirspaceWarningState m_state;
};

void print_warnings() {
  if (airspace_warnings) {
    {
      AirspaceWarningPrint visitor_inside("results/res-as-warnings-inside.txt", 
                                          AirspaceWarning::WARNING_INSIDE);
      AirspaceWarningPrint visitor_glide("results/res-as-warnings-glide.txt", 
                                          AirspaceWarning::WARNING_GLIDE);
      AirspaceWarningPrint visitor_filter("results/res-as-warnings-filter.txt", 
                                          AirspaceWarning::WARNING_FILTER);
      AirspaceWarningPrint visitor_task("results/res-as-warnings-task.txt", 
                                          AirspaceWarning::WARNING_TASK);
      airspace_warnings->visit_warnings(visitor_inside);
      airspace_warnings->visit_warnings(visitor_glide);
      airspace_warnings->visit_warnings(visitor_filter);
      airspace_warnings->visit_warnings(visitor_task);
    }
  }
}

