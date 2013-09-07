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

#include "RowFormWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "Math/Angle.hpp"
#include "Util/ConvertString.hpp"

#include <windef.h> /* for MAX_PATH */
#include <assert.h>

WndProperty *
RowFormWidget::AddFileReader(const TCHAR *label, const TCHAR *help,
                             const char *registry_key, const TCHAR *filters,
                             bool nullable)
{
  WndProperty *edit = Add(label, help);
  DataFieldFileReader *df = new DataFieldFileReader();
  edit->SetDataField(df);

  if (nullable)
    df->AddNull();

  df->ScanMultiplePatterns(filters);

  if (registry_key != nullptr) {
    TCHAR path[MAX_PATH];
    if (Profile::GetPath(registry_key, path))
      df->Lookup(path);
  }

  edit->RefreshDisplay();

  return edit;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         TCHAR *string, size_t max_size) const
{
  if (!SaveValue(i, string, max_size))
    return false;

  Profile::Set(registry_key, string);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         bool &value, bool negated) const
{
  if (!SaveValue(i, value, negated))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         int &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         uint8_t &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         uint16_t &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, const char *registry_key,
                         fixed &value) const
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValueFileReader(unsigned i, const char *registry_key)
{
  const DataFieldFileReader *dfe =
    (const DataFieldFileReader *)GetControl(i).GetDataField();
  TCHAR new_value[MAX_PATH];
  _tcscpy(new_value, dfe->GetPathFile());
  ContractLocalPath(new_value);

  const WideToUTF8Converter new_value2(new_value);
  if (!new_value2.IsValid())
    return false;

  const char *old_value = Profile::Get(registry_key, "");
  if (StringIsEqual(old_value, new_value2))
    return false;

  Profile::Set(registry_key, new_value2);
  return true;
}
