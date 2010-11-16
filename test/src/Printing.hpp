#ifndef PRINTING_HPP
#define PRINTING_HPP

#include <iostream>

class TaskManager;
class AbortTask;
class GotoTask;
class OrderedTask;
class AbstractTask;
class TaskPoint;
class SampledTaskPoint;
class OrderedTaskPoint;
class ContestManager;
class Trace;
class AATPoint;
struct AIRCRAFT_STATE;
struct GeoPoint;
struct ContestResult;

#ifdef FIXED_MATH
#include "Math/fixed.hpp"
std::ostream& operator<<(std::ostream& os,fixed const& value);
#endif

class PrintHelper {
public:
  static void taskmanager_print(TaskManager& task, const AIRCRAFT_STATE &location);
  static void abstracttask_print(AbstractTask& task, const AIRCRAFT_STATE &location);
  static void aborttask_print(AbortTask& task, const AIRCRAFT_STATE &location);
  static void gototask_print(GotoTask& task, const AIRCRAFT_STATE &location);
  static void orderedtask_print(OrderedTask& task, const AIRCRAFT_STATE &location);
  static void taskpoint_print(std::ostream& f, const TaskPoint& tp, const AIRCRAFT_STATE &state);
  static void sampledtaskpoint_print(std::ostream& f, const SampledTaskPoint& tp, 
                                     const AIRCRAFT_STATE &state);
  static void sampledtaskpoint_print_samples(std::ostream& f, const SampledTaskPoint& tp, 
                                             const AIRCRAFT_STATE &state);
  static void orderedtaskpoint_print(std::ostream& f, const OrderedTaskPoint& tp, 
                                     const AIRCRAFT_STATE &state,
                                     const int item=0);
  static void orderedtaskpoint_print_boundary(std::ostream& f, const OrderedTaskPoint& tp, 
                                              const AIRCRAFT_STATE &state);
  static void aatpoint_print(std::ostream& f, const AATPoint& tp, 
                             const AIRCRAFT_STATE &state,
                             const int item=0);
  static void contestmanager_print(const ContestManager& cm);
  static void trace_print(const Trace& trace, const GeoPoint &loc);
  static void print(const ContestResult& result);
};

#endif
