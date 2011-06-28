/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#include "TaskPointVisitor.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include <assert.h>

void
TaskPointConstVisitor::Visit(const TaskPoint &tp)
{
  switch (tp.GetType()) {

  case TaskPoint::ROUTE:
    assert(1); // not supported yet
    break;

  case TaskPoint::UNORDERED:
    Visit((const UnorderedTaskPoint &)tp);
    break;

  case TaskPoint::START:
    Visit((const StartPoint &)tp);
    break;

  case TaskPoint::AST:
    Visit((const ASTPoint &)tp);
    break;

  case TaskPoint::AAT:
    Visit((const AATPoint &)tp);
    break;

  case TaskPoint::FINISH:
    Visit((const FinishPoint &)tp);
    break;
  }
}

void
TaskPointVisitor::Visit(TaskPoint &tp)
{
  switch (tp.GetType()) {

  case TaskPoint::ROUTE:
    assert(1); // not supported yet
    break;

  case TaskPoint::UNORDERED:
    Visit((UnorderedTaskPoint &)tp);
    break;

  case TaskPoint::START:
    Visit((StartPoint &)tp);
    break;

  case TaskPoint::AST:
    Visit((ASTPoint &)tp);
    break;

  case TaskPoint::AAT:
    Visit((AATPoint &)tp);
    break;

  case TaskPoint::FINISH:
    Visit((FinishPoint &)tp);
    break;
  }
}
