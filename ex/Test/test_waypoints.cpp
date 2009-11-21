#include "harness_waypoints.hpp"
#include "test_debug.hpp"

#include "Waypoint/WaypointVisitor.hpp"

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
    printf("# visiting wp %d\n", wp.id);
    count++;
  }
  void summary() {
    printf("# there are %d wps found\n",count);
  }
  unsigned count;
};

bool test_range(const Waypoints& waypoints)
{
  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    WaypointVisitorPrint v;
    waypoints.visit_within_range(r->Location, 100, v);
    v.summary();
    return (v.count==2);
  } else {
    return false;
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

  plan_tests(3);

  Waypoints waypoints;

  ok(setup_waypoints(waypoints),"waypoint setup",0);
  ok(test_lookup(waypoints),"waypoint lookup",0);
  ok(test_range(waypoints),"waypoint range",0);
  return exit_status();
}
