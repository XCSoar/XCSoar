#ifndef TASK_VISITOR_HPP
#define TASK_VISITOR_HPP

#include "Util/GenericVisitor.hpp"
class OrderedTask;
class AbortTask;
class GotoTask;

/**
 * Generic visitor to achieve double-dispatch of an AbstractTask
 * (e.g. the active task in the TaskManager)
 */
class TaskVisitor:
  public BaseVisitor,
  public Visitor<OrderedTask>,
  public Visitor<AbortTask>,
  public Visitor<GotoTask>
{
};

#endif
