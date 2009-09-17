#ifndef TASK_VISITOR_HPP
#define TASK_VISITOR_HPP

#include "Task.h"


class RelativeTaskPointVisitor {
public:
  virtual void visit_reset() { };
  virtual void visit_null() { };
  virtual void visit_start_point(START_POINT &point, const unsigned index) { };
  virtual void visit_task_point_before(TASK_POINT &point, const unsigned index) { };
  virtual void visit_task_point_current(TASK_POINT &point, const unsigned index) { };
  virtual void visit_task_point_after(TASK_POINT &point, const unsigned index) { };
};

class AbsoluteTaskPointVisitor {
public:
  virtual void visit_reset() { };
  virtual void visit_null() { };
  virtual void visit_start_point(START_POINT &point, const unsigned index) { };
  virtual void visit_task_point_start(TASK_POINT &point, const unsigned index) { };
  virtual void visit_task_point_intermediate(TASK_POINT &point, const unsigned index) { };
  virtual void visit_task_point_final(TASK_POINT &point, const unsigned index) { };
};

class RelativeTaskLegVisitor {
public:
  virtual void visit_reset() { };
  virtual void visit_null() { };
  virtual void visit_single(TASK_POINT &point0, const unsigned index0) { };
  virtual void visit_leg_before(TASK_POINT &point0, const unsigned index0,
				TASK_POINT &point1, const unsigned index1) {};
  virtual void visit_leg_current(TASK_POINT &point0, const unsigned index0,
				 TASK_POINT &point1, const unsigned index1) {};
  virtual void visit_leg_after(TASK_POINT &point0, const unsigned index0,
			       TASK_POINT &point1, const unsigned index1) {};
};

class AbsoluteTaskLegVisitor {
public:
  virtual void visit_reset() { };
  virtual void visit_null() { };
  virtual void visit_single(TASK_POINT &point0, const unsigned index0) { };
  virtual void visit_leg_multistart(START_POINT &start, const unsigned index0, TASK_POINT &point) {};
  virtual void visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
				 TASK_POINT &point1, const unsigned index1) {};
  virtual void visit_leg_final(TASK_POINT &point0, const unsigned index0,
			       TASK_POINT &point1, const unsigned index1) {};
};

#endif
