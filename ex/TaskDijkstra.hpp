#ifndef TASK_DIJKSTRA_HPP
#define TASK_DIJKSTRA_HPP

#include "Util.h"
#include "Dijkstra.hpp"

class Task;

class TaskDijkstra {
public:
  TaskDijkstra(Task* _task,
               const double _precision=0.1);
  ~TaskDijkstra();

  double distance_opt(ScanTaskPoint start,
                      bool _shortest=false);

  double distance_opt_achieved(const GEOPOINT &currentLocation,
                               bool _shortest=false);
private:

  void add_edges(Dijkstra<ScanTaskPoint> &dijkstra,
                 const ScanTaskPoint &curNode);
  double distance(const ScanTaskPoint &sp,
                  const GEOPOINT &loc);

  unsigned num_taskpoints;
  unsigned *solution;
  double precision;
  bool shortest;
  Task* task;
};

#endif
