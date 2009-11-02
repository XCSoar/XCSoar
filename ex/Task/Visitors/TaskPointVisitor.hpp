#ifndef TASKPOINTVISITOR_HPP
#define TASKPOINTVISITOR_HPP

class TaskPoint;
class OrderedTaskPoint;
class StartPoint;
class FinishPoint;

#include "Util/GenericVisitor.hpp"

class TaskPointVisitor:
  public BaseVisitor,
  public Visitor<TaskPoint>,
  public Visitor<OrderedTaskPoint>,
  public Visitor<StartPoint>,
  public Visitor<FinishPoint>
{
};

#endif
