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

#include "ValidationErrorStrings.hpp"
#include "Language/Language.hpp"

#include <string.h>
#include <windef.h> // for MAX_PATH

gcc_pure
static const TCHAR*
TaskValidationError(TaskValidationErrorType type)
{
  switch (type) {
  case TaskValidationErrorType::NO_VALID_START:
    return _("No valid start.\n");
  case TaskValidationErrorType::NO_VALID_FINISH:
    return _("No valid finish.\n");
  case TaskValidationErrorType::TASK_NOT_CLOSED:
    return _("Task not closed.\n");
  case TaskValidationErrorType::TASK_NOT_HOMOGENEOUS:
    return _("All turnpoints not the same type.\n");
  case TaskValidationErrorType::INCORRECT_NUMBER_TURNPOINTS:
    return _("Incorrect number of turnpoints.\n");
  case TaskValidationErrorType::EXCEEDS_MAX_TURNPOINTS:
    return _("Too many turnpoints.\n");
  case TaskValidationErrorType::UNDER_MIN_TURNPOINTS:
    return _("Not enough turnpoints.\n");
  case TaskValidationErrorType::TURNPOINTS_NOT_UNIQUE:
    return _("Turnpoints not unique.\n");
  case TaskValidationErrorType::INVALID_FAI_TRIANGLE_GEOMETRY:
    return _("Invalid FAI triangle shape.\n");
  case TaskValidationErrorType::EMPTY_TASK:
    return _("Empty task.\n");
  case TaskValidationErrorType::NON_FAI_OZS:
    return _("non-FAI turn points");

  case TaskValidationErrorType::NON_MAT_OZS:
    return _("non-MAT turn points");

  case TaskValidationErrorType::COUNT:
    gcc_unreachable();
  }

  gcc_unreachable();
}

const TCHAR*
getTaskValidationErrors(const TaskValidationErrorSet v)
{
  static TCHAR err[MAX_PATH];
  err[0] = '\0';

  for (unsigned i = 0; i < v.N; i++) {
    const TaskValidationErrorType error = TaskValidationErrorType(i);
    if (v.Contains(error) &&
        _tcslen(err) + _tcslen(TaskValidationError(error)) < MAX_PATH)
      _tcscat(err, TaskValidationError(error));
  }

  return err;
}
