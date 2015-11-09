/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Profile.hpp"
#include "Map.hpp"
#include "File.hpp"
#include "Current.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StringCompare.hxx"
#include "Util/StringAPI.hxx"
#include "Util/Error.hxx"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"

#include <windef.h> /* for MAX_PATH */

#define XCSPROFILE "default.prf"
#define OLDXCSPROFILE "xcsoar-registry.prf"

static TCHAR startProfileFile[MAX_PATH];

const TCHAR *
Profile::GetPath()
{
  return startProfileFile;
}

void
Profile::Load()
{
  LogFormat("Loading profiles");
  LoadFile(startProfileFile);
  SetModified(false);
}

void
Profile::LoadFile(const TCHAR *szFile)
{
  Error error;
  if (LoadFile(map, szFile, error))
    LogFormat(_T("Loaded profile from %s"), szFile);
  else
    LogError("Failed to load profile", error);
}

void
Profile::Save()
{
  if (!IsModified())
    return;

  LogFormat("Saving profiles");
  if (StringIsEmpty(startProfileFile))
    SetFiles(nullptr);
  SaveFile(startProfileFile);
}

void
Profile::SaveFile(const TCHAR *szFile)
{
  LogFormat(_T("Saving profile to %s"), szFile);
  SaveFile(map, szFile);
}

void
Profile::SetFiles(const TCHAR *override_path)
{
  /* set the "modified" flag, because we are potentially saving to a
     new file now */
  SetModified(true);

  if (override_path != nullptr) {
    if (IsBaseName(override_path)) {
      LocalPath(startProfileFile, override_path);

      if (StringFind(override_path, '.') == nullptr)
        _tcscat(startProfileFile, _T(".prf"));
    } else
      CopyString(startProfileFile, override_path, MAX_PATH);
    return;
  }

  // Set the default profile file
  LocalPath(startProfileFile, _T(XCSPROFILE));
}

bool
Profile::GetPath(const char *key, TCHAR *value)
{
  return map.GetPath(key, value);
}

bool
Profile::GetPathIsEqual(const char *key, const TCHAR *value)
{
  return map.GetPathIsEqual(key, value);
}

void
Profile::SetPath(const char *key, const TCHAR *value)
{
  map.SetPath(key, value);
}
