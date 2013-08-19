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

#ifndef XCSOAR_FORM_UTIL_HPP
#define XCSOAR_FORM_UTIL_HPP

#include "Util/StaticString.hpp"
#include "Units/Group.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

#include <tchar.h>

struct StaticEnumChoice;
class SubForm;

/**
 * Show or hide a named form control.
 */
void
ShowFormControl(SubForm &form, const TCHAR *control_name, bool visible);

/**
 * Show or hide a named form control that may not exist in some layouts.
 */
void
ShowOptionalFormControl(SubForm &form, const TCHAR *control_name,
                        bool visible);

void SetFormControlEnabled(SubForm &form, const TCHAR *control_name,
                           bool enabled);

/**
 * Set a form control to the specified value (without a DataField).
 */
void
SetFormValue(SubForm &form, const TCHAR *control_name, const TCHAR *value);

/**
 * Loads the specified value into the form.
 *
 * @param form the form
 * @param control_name the name of the control in the form
 * @param value the new value
 */
void
LoadFormProperty(SubForm &form, const TCHAR *control_name, bool value);
void
LoadFormProperty(SubForm &form, const TCHAR *control_name, unsigned int value);

void
LoadFormPropertyEnum(SubForm &form, const TCHAR *control_name, int value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 const StaticEnumChoice *list, unsigned value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name, fixed value);

void
LoadOptionalFormProperty(SubForm &form, const TCHAR *control_name,
                         fixed value);

void
LoadOptionalFormProperty(SubForm &form, const TCHAR *control_name,
                         UnitGroup unit_group, fixed value);

static inline void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, unsigned value)
{
  LoadFormProperty(form, control_name, unit_group, (int)value);
}

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 const TCHAR *value);

gcc_pure
int
GetFormValueInteger(const SubForm &form, const TCHAR *control_name);

gcc_pure
fixed
GetFormValueFixed(const SubForm &form, const TCHAR *control_name);

gcc_pure
const TCHAR *
GetFormValueString(const SubForm &form, const TCHAR *control_name);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, bool &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, unsigned int &value);

bool
SaveFormProperty(SubForm &form, const TCHAR *control_name, fixed &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed &value);

/**
 * Saves a form value into a variable and into the registry.
 *
 * @param form the form
 * @param control_name the name of the control in the form
 * @param value the new value
 * @param registry_name the name of the registry key
 * @return true if the value has been modified
 */
bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 bool &value, const char *profile_key);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field,
                 const char *profile_key,
                 bool &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 TCHAR *buffer, size_t max_size);

template<size_t N>
bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 StaticString<N> &value)
{
  return SaveFormProperty(form, control_name, value.buffer(), value.MAX_SIZE);
}

#endif
