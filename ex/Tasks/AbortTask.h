#ifndef ABORTTASK_H
#define ABORTTASK_H

#include "AbstractTask.h"
#include <vector>
#include "BaseTask/TaskProjection.h"
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

  virtual void print(const AIRCRAFT_STATE &location);

  virtual bool update_sample(const AIRCRAFT_STATE &, const bool full_update);
protected:
  unsigned active_waypoint;

  const TaskProjection &task_projection;
  virtual bool check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  const Waypoints &waypoints;
  void clear();
  bool task_full() const;

  int abort_range(const AIRCRAFT_STATE &location);

  GlidePolar polar_safety;
private:

  /** @link dependency */
  /*#  Rank lnkRank; */
protected:
  void update_polar();

  void fill_reachable(const AIRCRAFT_STATE &,
                      std::vector < WAYPOINT > &approx_waypoints,
                      const bool only_airfield);
};

#endif //ABORTTASK_H
