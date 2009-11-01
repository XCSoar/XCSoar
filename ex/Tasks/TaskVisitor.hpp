#ifndef TASK_VISITOR_HPP
#define TASK_VISITOR_HPP

#include "Util/GenericVisitor.hpp"
class OrderedTask;
class AbortTask;
class GotoTask;

class TaskVisitor:
  public BaseVisitor,
  public Visitor<OrderedTask>,
  public Visitor<AbortTask>,
  public Visitor<GotoTask>
{
};

#endif
