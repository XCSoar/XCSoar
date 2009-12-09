#include "harness_airspace.hpp"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceNearestSort.hpp"
#include "Airspace/AirspaceSoonestSort.hpp"


void airspace_random_properties(AbstractAirspace& as) {
  int Type = rand()%15;
  AIRSPACE_ALT base;
  AIRSPACE_ALT top;
  base.Altitude = rand()%5000;
  top.Altitude = base.Altitude+rand()%3000;
  as.set_properties("hello", Type, base, top);
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
    if (rand()%3==0) {
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      double radius = 10000.0*(0.2+(rand()%12)/12.0);
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
        p.Longitude += (rand()%200)/1000.0;
        p.Latitude += (rand()%200)/1000.0;
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
  virtual void Visit(const AirspaceCircle& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
    }
  }
  virtual void Visit(const AirspacePolygon& as) {
    if (do_report) {
#ifdef DO_PRINT
      *fout << as;
#endif
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
                                   const bool _do_report,
                                   const GEOPOINT &loc):
    do_report(_do_report),
    location(loc)
    {      
      if (do_report) {
#ifdef DO_PRINT
        fout = new std::ofstream(fname);
        yout = new std::ofstream(yname);
#endif
      }
    };
  ~AirspaceIntersectionVisitorPrint() {
#ifdef DO_PRINT
    if (do_report) {
      delete fout;
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
#endif
  const bool do_report;
  const GEOPOINT location;
};


class AirspaceVisitorClosest: public AirspaceVisitor {
public:
  AirspaceVisitorClosest(const char* fname,
                         const AIRCRAFT_STATE &_state):
    state(_state)
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
    fixed time_to_intercept= 1e6;
    fixed intercept_height;

    AirspaceAircraftPerformance perf;
    if (as.intercept_vertical(state, c, perf, time_to_intercept, intercept_height)) {
#ifdef DO_PRINT
      *fout << "# intercept in " << time_to_intercept << " dh " << intercept_height << "\n";
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
};


void scan_airspaces(const AIRCRAFT_STATE state, 
                    const Airspaces& airspaces,
                    bool do_report,
                    const GEOPOINT &target) 
{
  const fixed range = 20000.0;

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
                                    state);
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
                                              do_report,
                                              state.Location);
    GeoVector vec(state.Location, target);
    airspaces.visit_intersecting(state.Location, vec, ivisitor, true);
  }

  {
    AirspaceNearestSort ans(state);
    const AbstractAirspace* as = ans.find_nearest(airspaces, range);
    std::ofstream fout("results/res-bb-sortednearest.txt");
    if (as) {
      fout << *as << "\n";
    } else {
      fout << "# no nearest found\n";
    }
  }

  {
    AirspaceAircraftPerformance perf;
    AirspaceSoonestSort ans(state, perf);
    const AbstractAirspace* as = ans.find_nearest(airspaces, range);
    std::ofstream fout("results/res-bb-sortedsoonest.txt");
    if (as) {
      fout << *as << "\n";
    } else {
      fout << "# no soonest found\n";
    }
  }

}

