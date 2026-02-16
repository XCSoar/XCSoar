// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ValidationErrorStrings.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"

#include <windef.h> // for MAX_PATH
#include <string.h>

static const char *const validation_error_strings[] = {
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

const char*
getTaskValidationErrors(const TaskValidationErrorSet v)
{
  static char err[MAX_PATH];
  err[0] = '\0';

  for (unsigned i = 0; i < v.N; i++) {
    const TaskValidationErrorType error = TaskValidationErrorType(i);
    if (!v.Contains(error))
      continue;

    const char *current = gettext(validation_error_strings[i]);
    if (strlen(err) + strlen(current) + 1 < MAX_PATH) {
      strcat(err, current);
      strcat(err, _T("\n"));
    }
  }

  return err;
}
