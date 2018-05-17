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
#include "Util/tstring.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Path.hpp"

#include <windef.h> /* for MAX_PATH */
#include <assert.h>

#define XCSPROFILE "default.prf"
#define OLDXCSPROFILE "xcsoar-registry.prf"

static AllocatedPath startProfileFile = nullptr;

Path
Profile::GetPath()
{
  return startProfileFile;
}

void
Profile::Load()
{
  assert(!startProfileFile.IsNull());

  LogFormat("Loading profiles");
  LoadFile(startProfileFile);
  SetModified(false);
}

void
Profile::LoadFile(Path path)
{
  try {
    LoadFile(map, path);
    LogFormat(_T("Loaded profile from %s"), path.c_str());
  } catch (const std::runtime_error &e) {
    LogError("Failed to load profile", e);
  }
}

void
Profile::Save()
{
  if (!IsModified())
    return;

  LogFormat("Saving profiles");
  if (startProfileFile.IsNull())
    SetFiles(nullptr);

  assert(!startProfileFile.IsNull());
  SaveFile(startProfileFile);
}

void
Profile::SaveFile(Path path)
{
  LogFormat(_T("Saving profile to %s"), path.c_str());
  SaveFile(map, path);
}

void
Profile::SetFiles(Path override_path)
{
  /* set the "modified" flag, because we are potentially saving to a
     new file now */
  SetModified(true);

  if (!override_path.IsNull()) {
    if (override_path.IsBase()) {
      if (StringFind(override_path.c_str(), '.') != nullptr)
        startProfileFile = LocalPath(override_path);
      else {
        tstring t(override_path.c_str());
        t += _T(".prf");
        startProfileFile = LocalPath(t.c_str());
      }
    } else
      startProfileFile = Path(override_path);
    return;
  }

  // Set the default profile file
  startProfileFile = LocalPath(_T(XCSPROFILE));
}

AllocatedPath
Profile::GetPath(const char *key)
{
  return map.GetPath(key);
}

bool
Profile::GetPathIsEqual(const char *key, Path value)
{
  return map.GetPathIsEqual(key, value);
}

void
Profile::SetPath(const char *key, Path value)
{
  map.SetPath(key, value);
}
