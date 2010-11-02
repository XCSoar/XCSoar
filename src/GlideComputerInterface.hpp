#ifndef GLIDE_COMPUTER_INTERFACE_HPP
#define GLIDE_COMPUTER_INTERFACE_HPP

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents:
  public TaskEvents
{
public:
  void set_computer(GlideComputer& computer);

  void transition_enter(const TaskPoint& tp);

  void transition_alternate();

  void active_advanced(const TaskPoint &tp, const int i);

  void request_arm(const TaskPoint &tp);

  void task_start();

  void task_finish();

  void transition_flight_mode(const bool is_final);
private:
  GlideComputer* m_computer;
};

#endif
