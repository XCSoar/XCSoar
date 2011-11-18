/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Units/Units.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <stdint.h>

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

void
SetFormMultiLineValue(SubForm &form, const TCHAR *control_name,
                      const TCHAR *value);

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
LoadFormProperty(SubForm &form, const TCHAR *control_name, int value);
void
LoadFormProperty(SubForm &form, const TCHAR *control_name, unsigned int value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 const StaticEnumChoice *list, unsigned value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name, fixed value);

void
LoadOptionalFormProperty(SubForm &form, const TCHAR *control_name,
                         fixed value);

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, int value);

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

void
LoadFormPropertyFromProfile(SubForm &form, const TCHAR *control_name,
                            const TCHAR *profile_key);

gcc_pure
int
GetFormValueInteger(const SubForm &form, const TCHAR *control_name);

gcc_pure
bool
GetFormValueBoolean(const SubForm &form, const TCHAR *control_name);

gcc_pure
fixed
GetFormValueFixed(const SubForm &form, const TCHAR *control_name);

gcc_pure
const TCHAR *
GetFormValueString(const SubForm &form, const TCHAR *control_name);

template<typename T>
static inline void
GetFormValueEnum(const SubForm &form, const TCHAR *control_name,
                 T &value)
{
  value = (T)GetFormValueInteger(form, control_name);
}

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, bool &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, unsigned int &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, int &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, short &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, uint8_t &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR* field, uint16_t &value);

bool
SaveFormProperty(SubForm &form, const TCHAR *control_name, fixed &value);

#ifdef FIXED_MATH
bool
SaveFormProperty(SubForm &form, const TCHAR *control_name, double &value);
#endif

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
                 bool &value, const TCHAR *registry_name);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 bool &value);

/**
 * Same as SaveFormProperty(), but negates the input value.
 */
bool
SaveFormPropertyNegated(const SubForm &form, const TCHAR *field,
                        const TCHAR *profile_key, bool &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 unsigned int &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 int &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 short &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 uint8_t &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                 uint16_t &value);

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, int &value,
                 const TCHAR *registry_name);

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, unsigned &value,
                 const TCHAR *registry_name);

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed &value,
                 const TCHAR *registry_name);

template<typename T>
static inline bool
SaveFormPropertyEnum(const SubForm &form, const TCHAR *field,
                     T &value)
{
  int value2 = (int)value;
  if (!SaveFormProperty(form, field, value2))
    return false;

  value = (T)value2;
  return true;
}

template<typename T>
static inline bool
SaveFormPropertyEnum(const SubForm &form, const TCHAR *field, const TCHAR *reg,
                     T &value)
{
  int value2 = (int)value;
  if (!SaveFormProperty(form, field, reg, value2))
    return false;

  value = (T)value2;
  return true;
}

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 TCHAR *buffer, size_t max_size);

template<unsigned N>
bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 StaticString<N> &value)
{
  return SaveFormProperty(form, control_name, value.buffer(), value.MAX_SIZE);
}

bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 TCHAR *buffer, size_t max_size,
                 const TCHAR *profile_key);

template<unsigned N>
bool
SaveFormProperty(const SubForm &form, const TCHAR *control_name,
                 StaticString<N> &value,
                 const TCHAR *profile_key)
{
  return SaveFormProperty(form, control_name, value.buffer(), value.MAX_SIZE,
                          profile_key);
}

bool
SaveFormPropertyToProfile(const SubForm &form, const TCHAR *control_name,
                          const TCHAR *profile_key);

#endif
