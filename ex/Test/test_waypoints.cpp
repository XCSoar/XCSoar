#include "harness_waypoints.hpp"
#include "test_debug.hpp"

#include "Waypoint/WaypointVisitor.hpp"

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
//    printf("# visiting wp %d\n", wp.id);
    count++;
  }
  unsigned count;
  void reset() {
    count = 0;
  }
};

unsigned test_range(const Waypoints& waypoints, const double range)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    WaypointVisitorPrint v;
    waypoints.visit_within_range(r->Location, range, v);
    return v.count;
  } else {
    return 0;
  }
}

unsigned test_find_range(const Waypoints& waypoints, const double range)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    return (waypoints.find_within_range(r->Location, range).size());
  } else {
    return 0;
  }
}

unsigned test_find_range_circle(const Waypoints& waypoints, const double range)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    return (waypoints.find_within_range_circle(r->Location, range).size());
  } else {
    return 0;
  }
}

unsigned test_nearest(const Waypoints& waypoints)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    Waypoints::WaypointTree::const_iterator it = waypoints.find_nearest(r->Location);
    if (it != waypoints.end()) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

bool test_lookup(const Waypoints& waypoints, unsigned id)
{
  const Waypoint* wp;
  wp = waypoints.lookup_id(id);
  return (wp!= NULL);
}

int main(int argc, char** argv)
{
  ::InitSineTable();

  if (!parse_args(argc,argv)) {
    return 0;
  }

  plan_tests(10);

  Waypoints waypoints;

  ok(setup_waypoints(waypoints),"waypoint setup",0);

  unsigned size = waypoints.size();

  ok(test_lookup(waypoints,3),"waypoint lookup",0);
  ok(!test_lookup(waypoints,5000),"waypoint bad lookup",0);
  ok(test_nearest(waypoints),"waypoint nearest",0);
  ok(test_range(waypoints,100)==1,"waypoint visit range 100m",0);
  ok(test_find_range(waypoints,100)==1,"waypoint find range 100m",0);
  ok(test_find_range_circle(waypoints,100)==1,"waypoint find range 100m",0);
  ok(test_range(waypoints,500000)== waypoints.size(),"waypoint range 500000m",0);

  // test clear
  waypoints.clear();
  ok(waypoints.size()==0,"waypoint clear",0);
  setup_waypoints(waypoints);
  ok(size == waypoints.size(),"waypoint setup after clear",0);

  return exit_status();
}
