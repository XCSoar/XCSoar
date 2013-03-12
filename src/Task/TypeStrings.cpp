/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Engine/Task/Points/Type.hpp"
#include "Language/Language.hpp"

#include <assert.h>

const TCHAR*
OrderedTaskFactoryName(TaskFactoryType type)
{
  switch(type) {
  case TaskFactoryType::RACING:
    return _("Racing");
  case TaskFactoryType::FAI_GENERAL:
    return _("FAI badges/records");
  case TaskFactoryType::FAI_TRIANGLE:
    return _("FAI triangle");
  case TaskFactoryType::FAI_OR:
    return _("FAI out and return");
  case TaskFactoryType::FAI_GOAL:
    return _("FAI goal");
  case TaskFactoryType::AAT:
    return _("AAT");
  case TaskFactoryType::MAT:
    return _("Modified area task (MAT)");
  case TaskFactoryType::MIXED:
    return _("Mixed");
  case TaskFactoryType::TOURING:
    return _("Touring");
  }

  gcc_unreachable();
}

const TCHAR*
OrderedTaskFactoryDescription(TaskFactoryType type)
{
  switch(type) {
  case TaskFactoryType::RACING:
    return _("Racing task around turn points.  Can also be used for FAI badge and record tasks.  "
        "Allows all shapes of observation zones.");
  case TaskFactoryType::FAI_GENERAL:
    return _("FAI rules, allows only FAI start, finish and turn point types, for badges and "
        "records.  Enables FAI finish height for final glide calculation.");
  case TaskFactoryType::FAI_TRIANGLE:
    return _("FAI rules, path from a start to two turn points and return.");
  case TaskFactoryType::FAI_OR:
    return _("FAI rules, path from start to a single turn point and return.");
  case TaskFactoryType::FAI_GOAL:
    return _("FAI rules, path from start to a goal destination.");
  case TaskFactoryType::AAT:
    return _("Task through assigned areas, minimum task time applies.  Restricted to cylinder "
        "and sector observation zones.");
  case TaskFactoryType::MAT:
    return _("Modified area task.  Task with start, finish and at least one predefined 1-mile cylinder.  Pilot can add additional points as needed.  Minimum task time applies.");
  case TaskFactoryType::MIXED:
    return _("Racing task with a mix of assigned areas and turn points, minimum task time applies.");
  case TaskFactoryType::TOURING:
    return _("Casual touring task, uses start and finish cylinders and FAI sector turn points.");
  }

  gcc_unreachable();
}

const TCHAR*
OrderedTaskPointDescription(TaskPointFactoryType type)
{
  switch (type) {
  case TaskPointFactoryType::START_SECTOR:
    return _("A 90 degree sector with 1km radius. Cross corner edge from inside area to start.");
  case TaskPointFactoryType::START_LINE:
    return _("A straight line start gate.  Cross start gate from inside area to start.");
  case TaskPointFactoryType::START_CYLINDER:
    return _("A cylinder.  Exit area to start.");
  case TaskPointFactoryType::START_BGA:
    return _("A 180 degree sector with 5km radius.  Exit area in any direction to start.");
  case TaskPointFactoryType::FAI_SECTOR:
    return _("A 90 degree sector with 'infinite' length sides.  Cross any edge, scored from "
        "corner point.");
  case TaskPointFactoryType::AST_CYLINDER:
    return _("A cylinder.  Any point within area scored from center.");
  case TaskPointFactoryType::KEYHOLE_SECTOR:
    return _("(German rules) Any point within 1/2 km of center or 10km of a 90 degree sector.  "
        "Scored from center.");
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
    return _("(British rules) Any point within 1/2 km of center or 20km of a 90 degree sector.  "
        "Scored from center.");
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    return _("(British rules) Any point within 1/2 km of center or 10km of a 180 degree sector.  "
        "Scored from center.");
  case TaskPointFactoryType::AAT_CYLINDER:
    return _("A cylinder.  Scored by farthest point reached in area.");

  case TaskPointFactoryType::MAT_CYLINDER:
    return _("A 1 mile cylinder.  Scored by farthest point reached in area.");

  case TaskPointFactoryType::AAT_SEGMENT:
    return _("A sector that can vary in angle and radius.  Scored by farthest point reached "
        "inside area.");
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
    return _("A sector that can vary in angle, inner and outer radius.  Scored by farthest point "
        "reached inside area.");
  case TaskPointFactoryType::FINISH_SECTOR:
    return _("A 90 degree sector with 1km radius.  Cross edge to finish.");
  case TaskPointFactoryType::FINISH_LINE:
    return _("Cross finish gate line into area to finish.");
  case TaskPointFactoryType::FINISH_CYLINDER:
    return _("Enter cylinder to finish.");
  }

  gcc_unreachable();
}

const TCHAR*
OrderedTaskPointName(TaskPointFactoryType type)
{
  switch (type) {
  case TaskPointFactoryType::START_SECTOR:
    return _("FAI start quadrant");
  case TaskPointFactoryType::START_LINE:
    return _("Start line");
  case TaskPointFactoryType::START_CYLINDER:
    return _("Start cylinder");
  case TaskPointFactoryType::START_BGA:
    return _("BGA start sector");
  case TaskPointFactoryType::FAI_SECTOR:
    return _("FAI quadrant");
  case TaskPointFactoryType::KEYHOLE_SECTOR:
    return _("Keyhole sector (DAeC)");
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
    return _("BGA Fixed Course sector");
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    return _("BGA Enhanced Option Fixed Course sector");
  case TaskPointFactoryType::AST_CYLINDER:
    return _("Turn point cylinder");
  case TaskPointFactoryType::AAT_CYLINDER:
    return _("Area cylinder");

  case TaskPointFactoryType::MAT_CYLINDER:
    return _("Cylinder with 1 mile radius.");

  case TaskPointFactoryType::AAT_SEGMENT:
    return _("Area sector");
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
    return _("Area sector with inner radius");
  case TaskPointFactoryType::FINISH_SECTOR:
    return _("FAI finish quadrant");
  case TaskPointFactoryType::FINISH_LINE:
    return _("Finish line");
  case TaskPointFactoryType::FINISH_CYLINDER:
    return _("Finish cylinder");
  }

  gcc_unreachable();
}
