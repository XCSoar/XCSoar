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

bool test_lookup(const Waypoints& waypoints)
{
  const Waypoint* wp;
  wp = waypoints.lookup_id(2);
  return (wp!= NULL);
}

int main()
{
  interactive = false;

  plan_tests(4);

  Waypoints waypoints;

  ok(setup_waypoints(waypoints),"waypoint setup",0);
  ok(test_lookup(waypoints),"waypoint lookup",0);
  ok(test_range(waypoints,100)==2,"waypoint range 100m",0);
  ok(test_range(waypoints,500000)== waypoints.size(),"waypoint range 500000m",0);
  return exit_status();
}
