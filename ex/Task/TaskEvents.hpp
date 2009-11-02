#ifndef TASKEVENTS_HPP
#define TASKEVENTS_HPP

class TaskPoint;

class TaskEvents 
{
public:

  virtual void transition_enter(const TaskPoint& tp) const;
  virtual void transition_exit(const TaskPoint &tp) const;
  virtual void active_advanced(const TaskPoint &tp, const int i) const;
  virtual void active_changed(const TaskPoint &tp) const;
  virtual void construction_error(const char* error) const;

};

#endif
