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

#include "FAITaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Util/Macros.hpp"

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t fai_start_types[] = {
  AbstractTaskFactory::START_SECTOR,
  AbstractTaskFactory::START_LINE,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t fai_im_types[] = {
  AbstractTaskFactory::FAI_SECTOR,
  AbstractTaskFactory::AST_CYLINDER,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t fai_finish_types[] = {
  AbstractTaskFactory::FINISH_SECTOR,
  AbstractTaskFactory::FINISH_LINE,
};

FAITaskFactory::FAITaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(_task, tb,
                       LegalPointConstArray(fai_start_types,
                                            ARRAY_SIZE(fai_start_types)),
                       LegalPointConstArray(fai_im_types,
                                            ARRAY_SIZE(fai_im_types)),
                       LegalPointConstArray(fai_finish_types,
                                            ARRAY_SIZE(fai_finish_types)))
{
}

bool
FAITaskFactory::validate()
{
  bool valid = AbstractTaskFactory::validate();

  if (!is_unique()) {
    addValidationError(TURNPOINTS_NOT_UNIQUE);
    // warning only
  }
  return valid;
}

void 
FAITaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  to.task_scored = true;
  to.fai_finish = true;  
  to.homogeneous_tps = false;
  to.is_closed = false;
  to.min_points = 2;
  to.max_points = 13;

  to.start_max_speed = fixed_zero;
  to.start_max_height = 0;
  to.start_max_height_ref = hrAGL;
  to.finish_min_height = 0;
  to.start_requires_arm = false;
}

AbstractTaskFactory::LegalPointType_t
FAITaskFactory::getMutatedPointType(const OrderedTaskPoint &tp) const
{
  const LegalPointType_t oldtype = getType(tp);
  LegalPointType_t newtype = oldtype;

  switch (oldtype) {

  case START_SECTOR:
  case START_LINE:
    break;

  case START_CYLINDER:
  case START_BGA:
    newtype = START_SECTOR;
    break;

  case KEYHOLE_SECTOR:
  case BGAFIXEDCOURSE_SECTOR:
  case BGAENHANCEDOPTION_SECTOR:
  case AAT_ANNULAR_SECTOR:
  case AAT_SEGMENT:
    newtype = FAI_SECTOR;
    break;

  case FINISH_SECTOR:
  case FINISH_LINE:
    break;

  case FINISH_CYLINDER:
    newtype = FINISH_SECTOR;
    break;

  case FAI_SECTOR:
  case AST_CYLINDER:
    break;

  case AAT_CYLINDER:
    newtype = AST_CYLINDER;
    break;
  }
  return newtype;
}

void
FAITaskFactory::getPointDefaultSizes(const LegalPointType_t type,
                                          fixed &start_radius,
                                          fixed &turnpoint_radius,
                                          fixed &finish_radius) const
{
  start_radius = finish_radius = fixed(1000);
  turnpoint_radius = fixed(500);

  switch (type) {
  case START_LINE:
  case FINISH_LINE:
    start_radius = finish_radius = fixed(2000);
    break;

  case START_SECTOR:
  case START_CYLINDER:
  case FAI_SECTOR:
  case KEYHOLE_SECTOR:
  case BGAFIXEDCOURSE_SECTOR:
  case BGAENHANCEDOPTION_SECTOR:
  case AST_CYLINDER:
  case AAT_CYLINDER:
  case AAT_SEGMENT:
  case AAT_ANNULAR_SECTOR:
  case FINISH_SECTOR:
  case FINISH_CYLINDER:
  case START_BGA:
    break;
  }
}
