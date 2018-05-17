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

#include "TouringTaskFactory.hpp"
#include "Constraints.hpp"
#include "Task/Ordered/Settings.hpp"

static constexpr TaskFactoryConstraints touring_constraints = {
  false,
  false,
  true,
  false,
  false,
  2, 10,
};

static constexpr LegalPointSet touring_start_types{
  TaskPointFactoryType::START_CYLINDER,
};

static constexpr LegalPointSet touring_im_types{
  TaskPointFactoryType::FAI_SECTOR,
};

static constexpr LegalPointSet touring_finish_types{
  TaskPointFactoryType::FINISH_CYLINDER,
};

TouringTaskFactory::TouringTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(touring_constraints, _task, tb,
                       touring_start_types, touring_im_types,
                       touring_finish_types)
{
}

void 
TouringTaskFactory::UpdateOrderedTaskSettings(OrderedTaskSettings& to)
{
  AbstractTaskFactory::UpdateOrderedTaskSettings(to);

  to.aat_min_time = 0;

  to.start_constraints.max_speed = 0;
  to.start_constraints.max_height = 0;
  to.start_constraints.max_height_ref = AltitudeReference::AGL;
  to.finish_constraints.min_height = 0;
}
