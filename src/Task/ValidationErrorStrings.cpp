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

#include "ValidationErrorStrings.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"

#include <windef.h> // for MAX_PATH
#include <string.h>

static const TCHAR *const validation_error_strings[] = {
  N_("No valid start"),
  N_("No valid finish"),
  N_("Task not closed"),
  N_("All turnpoints not the same type"),
  N_("Incorrect number of turnpoints"),
  N_("Too many turnpoints"),
  N_("Not enough turnpoints"),
  N_("Turnpoints not unique"),
  N_("Empty task"),
  N_("non-FAI turn points"),
  N_("non-MAT turn points"),
};

static_assert(ARRAY_SIZE(validation_error_strings) == unsigned(TaskValidationErrorType::COUNT),
              "Wrong array size");

const TCHAR*
getTaskValidationErrors(const TaskValidationErrorSet v)
{
  static TCHAR err[MAX_PATH];
  err[0] = '\0';

  for (unsigned i = 0; i < v.N; i++) {
    const TaskValidationErrorType error = TaskValidationErrorType(i);
    if (!v.Contains(error))
      continue;

    const TCHAR *current = gettext(validation_error_strings[i]);
    if (_tcslen(err) + _tcslen(current) + 1 < MAX_PATH) {
      _tcscat(err, current);
      _tcscat(err, _T("\n"));
    }
  }

  return err;
}
