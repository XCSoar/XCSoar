#ifndef TASKPOINTVISITOR_HPP
#define TASKPOINTVISITOR_HPP

class TaskPoint;
class StartPoint;
class FinishPoint;
class ASTPoint;
class AATPoint;

#include "Util/GenericVisitor.hpp"

/**
 * Generic visitor for task points (for double-dispatch)
 */
class TaskPointVisitor:
  public BaseVisitor,
  public Visitor<TaskPoint>,
  public Visitor<StartPoint>,
  public Visitor<ASTPoint>,
  public Visitor<AATPoint>,
  public Visitor<FinishPoint>
{
};

#endif
