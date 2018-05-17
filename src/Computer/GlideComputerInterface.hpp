#ifndef GLIDE_COMPUTER_INTERFACE_HPP
#define GLIDE_COMPUTER_INTERFACE_HPP

#include "Task/TaskEvents.hpp"

class GlideComputer;

class GlideComputerTaskEvents final : public TaskEvents {
  GlideComputer* computer;

public:
  void SetComputer(GlideComputer &_computer);

  void EnterTransition(const TaskWaypoint& tp);

  void ActiveAdvanced(const TaskWaypoint &tp, const int i);

  void RequestArm(const TaskWaypoint &tp);

  void TaskStart();

  void TaskFinish();
};

#endif
