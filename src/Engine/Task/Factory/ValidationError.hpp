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

#ifndef XCSOAR_TASK_VALIDATION_ERROR_HPP
#define XCSOAR_TASK_VALIDATION_ERROR_HPP

#include "Util/EnumBitSet.hpp"

#include <stdint.h>

/** Task Validation Error Types */
enum class TaskValidationErrorType : uint8_t {
  NO_VALID_START,
  NO_VALID_FINISH,
  TASK_NOT_CLOSED,
  TASK_NOT_HOMOGENEOUS,
  INCORRECT_NUMBER_TURNPOINTS,
  EXCEEDS_MAX_TURNPOINTS,
  UNDER_MIN_TURNPOINTS,
  TURNPOINTS_NOT_UNIQUE,
  EMPTY_TASK,
  NON_FAI_OZS,
  NON_MAT_OZS,

  /**
   * This special value is used to determine the number of items
   * above.
   */
  COUNT
};

typedef EnumBitSet<TaskValidationErrorType> TaskValidationErrorSet;

#endif
