/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "FAITriangleTask.hpp"
#include "FAITriangleRules.hpp"
#include "../Ordered/OrderedTask.hpp"
#include "../Ordered/Points/OrderedTaskPoint.hpp"
#include "../Factory/AbstractTaskFactory.hpp"

bool
FAITriangleValidator::Validate(const OrderedTask &task)
{
  if (!task.GetFactory().IsUnique())
    return false;

  if (task.TaskSize() != 4)
    return false;

  const double d1 = task.GetTaskPoint(1).GetVectorPlanned().distance;
  const double d2 = task.GetTaskPoint(2).GetVectorPlanned().distance;
  const double d3 = task.GetTaskPoint(3).GetVectorPlanned().distance;

  return FAITriangleRules::TestDistances(d1, d2, d3,
                                         task.GetOrderedTaskSettings().fai_triangle);
}
