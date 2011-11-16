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

#include "ConfigPanel.hpp"
#include "Form/Edit.hpp"
#include "DataField/FileReader.hpp"
#include "Profile/Profile.hpp"
#include "OS/PathName.hpp"
#include "LocalPath.hpp"


static bool
ProfileStringModified(const TCHAR *key, const TCHAR *new_value)
{
  TCHAR old_value[MAX_PATH];
  Profile::Get(key, old_value, MAX_PATH);
  return _tcscmp(old_value, new_value) != 0;
}


void
ConfigPanel::InitFileField(WndProperty &wp, const TCHAR *profile_key,
                           const TCHAR *filters)
{
  DataFieldFileReader &df = *(DataFieldFileReader *)wp.GetDataField();

  size_t length;
  while ((length = _tcslen(filters)) > 0) {
    df.ScanDirectoryTop(filters);
    filters += length + 1;
  }

  TCHAR path[MAX_PATH];
  if (Profile::GetPath(profile_key, path))
    df.Lookup(path);

  wp.RefreshDisplay();
}


void
ConfigPanel::InitFileField(SubForm &wf, const TCHAR *control_name,
                           const TCHAR *profile_key, const TCHAR *filters)
{
  WndProperty *wp = (WndProperty *)wf.FindByName(control_name);
  assert(wp != NULL);

  InitFileField(*wp, profile_key, filters);
}


bool
ConfigPanel::FinishFileField(const WndProperty &wp, const TCHAR *profile_key)
{
  const DataFieldFileReader *dfe =
    (const DataFieldFileReader *)wp.GetDataField();
  TCHAR new_value[MAX_PATH];
  _tcscpy(new_value, dfe->GetPathFile());
  ContractLocalPath(new_value);

  if (!ProfileStringModified(profile_key, new_value))
    return false;

  Profile::Set(profile_key, new_value);
  return true;
}


bool
ConfigPanel::FinishFileField(SubForm &wf, const TCHAR *control_name,
                             const TCHAR *profile_key)
{
  const WndProperty *wp = (const WndProperty *)wf.FindByName(control_name);
  return wp != NULL && FinishFileField(*wp, profile_key);
}

