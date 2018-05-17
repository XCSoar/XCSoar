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

#ifndef XCSOAR_TASK_POINT_FACTORY_TYPE_HPP
#define XCSOAR_TASK_POINT_FACTORY_TYPE_HPP

#include <stdint.h>

/**
 * Legal types of points with observation zones.
 */
enum class TaskPointFactoryType : uint8_t {
  START_SECTOR = 0,
  START_LINE,
  START_CYLINDER,
  FAI_SECTOR,
  KEYHOLE_SECTOR,
  BGAFIXEDCOURSE_SECTOR,
  BGAENHANCEDOPTION_SECTOR,
  AST_CYLINDER,
  MAT_CYLINDER,
  AAT_CYLINDER,
  AAT_SEGMENT,
  FINISH_SECTOR,
  FINISH_LINE,
  FINISH_CYLINDER,
  START_BGA,
  AAT_ANNULAR_SECTOR,
  SYMMETRIC_QUADRANT,
  AAT_KEYHOLE,

  /**
   * This special value is used to determine the number of types
   * above.
   */
  COUNT
};

#endif
