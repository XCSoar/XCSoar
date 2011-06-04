#ifndef GLIDE_COMPUTER_INTERFACE_HPP
#define GLIDE_COMPUTER_INTERFACE_HPP

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents:
  public TaskEvents
{
  GlideComputer* m_computer;

public:
  void set_computer(GlideComputer& computer);

  void transition_enter(const TaskWayPoint& tp);

  void transition_alternate();

  void active_advanced(const TaskWayPoint &tp, const int i);

  void request_arm(const TaskWayPoint &tp);

  void task_start();

  void task_finish();

  void transition_flight_mode(const bool is_final);
};

#endif
