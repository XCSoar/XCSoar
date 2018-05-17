/*
Copyright_License {

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

#include "TypeStrings.hpp"
#include "Engine/Task/Factory/TaskFactoryType.hpp"
#include "Engine/Task/Factory/TaskPointFactoryType.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"

static const TCHAR *const task_factory_names[] = {
  N_("FAI badges/records"),
  N_("FAI triangle"),
  N_("FAI out and return"),
  N_("FAI goal"),
  N_("Racing"),
  N_("AAT"),
  N_("Modified area task (MAT)"),
  N_("Mixed"),
  N_("Touring"),
};

static_assert(ARRAY_SIZE(task_factory_names) == unsigned(TaskFactoryType::COUNT),
              "Wrong array size");

const TCHAR*
OrderedTaskFactoryName(TaskFactoryType type)
{
  return gettext(task_factory_names[unsigned(type)]);
}

static const TCHAR *const task_factory_descriptions[] = {
  N_("FAI rules, allows only FAI start, finish and turn point types, for badges and "
     "records.  Enables FAI finish height for final glide calculation."),
  N_("FAI rules, path from a start to two turn points and return."),
  N_("FAI rules, path from start to a single turn point and return."),
  N_("FAI rules, path from start to a goal destination."),
  N_("Racing task around turn points.  Can also be used for FAI badge and record tasks.  "
     "Allows all shapes of observation zones."),
  N_("Task through assigned areas, minimum task time applies.  Restricted to cylinder "
     "and sector observation zones."),
  N_("Modified area task.  Task with start, finish and at least one predefined 1-mile cylinder.  Pilot can add additional points as needed.  Minimum task time applies."),
  N_("Racing task with a mix of assigned areas and turn points, minimum task time applies."),
  N_("Casual touring task, uses start and finish cylinders and FAI sector turn points."),
};

static_assert(ARRAY_SIZE(task_factory_descriptions) == unsigned(TaskFactoryType::COUNT),
              "Wrong array size");

const TCHAR*
OrderedTaskFactoryDescription(TaskFactoryType type)
{
  return gettext(task_factory_descriptions[unsigned(type)]);
}

static const TCHAR *const tp_factory_descriptions[] = {
  N_("A 90 degree sector with 1km radius. Cross corner edge from inside area to start."),
  N_("A straight line start gate.  Cross start gate from inside area to start."),
  N_("A cylinder.  Exit area to start."),
  N_("A 90 degree sector with 'infinite' length sides.  Cross any edge, scored from "
     "corner point."),
  N_("(German rules) Any point within 1/2 km of center or 10km of a 90 degree sector.  "
     "Scored from center."),
  N_("(British rules) Any point within 1/2 km of center or 20km of a 90 degree sector.  "
     "Scored from center."),
  N_("(British rules) Any point within 1/2 km of center or 10km of a 180 degree sector.  "
     "Scored from center."),
  N_("A cylinder.  Any point within area scored from center."),
  N_("A 1 mile cylinder.  Scored by farthest point reached in area."),
  N_("A cylinder.  Scored by farthest point reached in area."),
  N_("A sector that can vary in angle and radius.  Scored by farthest point reached "
     "inside area."),
  N_("A 90 degree sector with 1km radius.  Cross edge to finish."),
  N_("Cross finish gate line into area to finish."),
  N_("Enter cylinder to finish."),
  N_("A 180 degree sector with 5km radius.  Exit area in any direction to start."),
  N_("A sector that can vary in angle, inner and outer radius.  Scored by farthest point "
     "reached inside area."),
  N_("A symmetric quadrant with a custom radius."),
  N_("A keyhole.  Scored by farthest point reached in area."),
};

static_assert(ARRAY_SIZE(tp_factory_descriptions) == unsigned(TaskPointFactoryType::COUNT),
              "Wrong array size");

const TCHAR*
OrderedTaskPointDescription(TaskPointFactoryType type)
{
  return tp_factory_descriptions[unsigned(type)];
}

static const TCHAR *const tp_factory_names[] = {
  N_("FAI start quadrant"),
  N_("Start line"),
  N_("Start cylinder"),
  N_("FAI quadrant"),
  N_("Keyhole sector (DAeC)"),
  N_("BGA Fixed Course sector"),
  N_("BGA Enhanced Option Fixed Course sector"),
  N_("Turn point cylinder"),
  N_("Cylinder with 1 mile radius."),
  N_("Area cylinder"),
  N_("Area sector"),
  N_("FAI finish quadrant"),
  N_("Finish line"),
  N_("Finish cylinder"),
  N_("BGA start sector"),
  N_("Area sector with inner radius"),
  N_("Symmetric quadrant"),
  N_("Area keyhole"),
};

static_assert(ARRAY_SIZE(tp_factory_names) == unsigned(TaskPointFactoryType::COUNT),
              "Wrong array size");

const TCHAR*
OrderedTaskPointName(TaskPointFactoryType type)
{
  return tp_factory_names[unsigned(type)];
}
