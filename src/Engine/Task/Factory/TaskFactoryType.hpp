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

#ifndef FACTORY_TYPE_HPP
#define FACTORY_TYPE_HPP

#include <stdint.h>

/**
 * Enumeration of factory types.  This is the set of
 * types of ordered task that can be created.
 */
enum class TaskFactoryType: uint8_t {
  FAI_GENERAL = 0,
  FAI_TRIANGLE,
  FAI_OR,
  FAI_GOAL,
  RACING,
  AAT,

  /**
   * Modified Area Task.
   */
  MAT,

  MIXED,
  TOURING,

  /**
   * This special value is used to determine the number of items
   * above.
   */
  COUNT
};

/**
 * returns true if task is an FAI type
 * @param ftype. task type being checked
 */
constexpr
static bool
IsFai(TaskFactoryType ftype)
{
  return ftype == TaskFactoryType::FAI_GENERAL ||
    ftype == TaskFactoryType::FAI_GOAL ||
    ftype == TaskFactoryType::FAI_OR ||
    ftype == TaskFactoryType::FAI_TRIANGLE;
}

#endif
