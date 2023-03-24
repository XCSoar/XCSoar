// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ValidationErrorStrings.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"

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
  N_("Wrong shape"),
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
