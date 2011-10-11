#ifndef GLIDE_COMPUTER_INTERFACE_HPP
#define GLIDE_COMPUTER_INTERFACE_HPP

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents:
  public TaskEvents
{
  GlideComputer* computer;

public:
  void SetComputer(GlideComputer &_computer);

  void EnterTransition(const TaskWaypoint& tp);

  void AlternateTransition();

  void ActiveAdvanced(const TaskWaypoint &tp, const int i);

  void RequestArm(const TaskWaypoint &tp);

  void TaskStart();

  void TaskFinish();

  void FlightModeTransition(const bool is_final);
};

#endif
