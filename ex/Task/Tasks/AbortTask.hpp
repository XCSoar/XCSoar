#ifndef ABORTTASK_H
#define ABORTTASK_H

#include "AbstractTask.hpp"
#include <vector>
#include "Navigation/TaskProjection.hpp"
#include "Navigation/Waypoints.hpp"

class AbortTask : public AbstractTask 
{
public:
  AbortTask(const TaskEvents &te, 
            const TaskBehaviour &tb,
            const TaskProjection &tp,
            TaskAdvance &ta,
            GlidePolar &gp,
            const Waypoints &wps);
  ~AbortTask();

  std::vector<TaskPoint*> tps;

  TaskPoint* getActiveTaskPoint();
  void setActiveTaskPoint(unsigned index);

#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

  virtual bool update_sample(const AIRCRAFT_STATE &, const bool full_update);
protected:
  unsigned active_waypoint;

  const TaskProjection &task_projection;
  virtual bool check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  const Waypoints &waypoints;
  void clear();
  bool task_full() const;

  double abort_range(const AIRCRAFT_STATE &location);

  GlidePolar polar_safety;
private:

  /** @link dependency */
  /*#  Rank lnkRank; */
protected:
  void update_polar();

  void fill_reachable(const AIRCRAFT_STATE &,
                      std::vector < Waypoint > &approx_waypoints,
                      const bool only_airfield);
public:
  void Accept(TaskPointVisitor& visitor) const;
  DEFINE_VISITABLE()
};

#endif //ABORTTASK_H
