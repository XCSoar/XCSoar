// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile.hpp"
#include "Map.hpp"
#include "File.hpp"
#include "Current.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "util/StringUtil.hpp"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"
#include "util/tstring.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <windef.h> /* for MAX_PATH */
#include <cassert>

#define XCSPROFILE "default.prf"
#define OLDXCSPROFILE "xcsoar-registry.prf"

static AllocatedPath startProfileFile = nullptr;

Path
Profile::GetPath() noexcept
{
  return startProfileFile;
}

void
Profile::Load() noexcept
{
  assert(startProfileFile != nullptr);

  LogString("Loading profiles");
  LoadFile(startProfileFile);
  SetModified(false);
}

void
Profile::LoadFile(Path path) noexcept
{
  try {
    LoadFile(map, path);
    LogFormat(_T("Loaded profile from %s"), path.c_str());
  } catch (...) {
    LogError(std::current_exception(), "Failed to load profile");
  }
}

void
Profile::Save() noexcept
{
  if (!IsModified())
    return;

  LogString("Saving profiles");
  if (startProfileFile == nullptr)
    SetFiles(nullptr);

  assert(startProfileFile != nullptr);

  try {
    SaveFile(startProfileFile);
  } catch (...) {
    LogError(std::current_exception(), "Failed to save profile");
  }
}

void
Profile::SaveFile(Path path)
{
  LogFormat(_T("Saving profile to %s"), path.c_str());
  SaveFile(map, path);
}

void
Profile::SetFiles(Path override_path) noexcept
{
  /* set the "modified" flag, because we are potentially saving to a
     new file now */
  SetModified(true);

  if (override_path != nullptr) {
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
Profile::GetPath(const char *key) noexcept
{
  return map.GetPath(key);
}

bool
Profile::GetPathIsEqual(const char *key, Path value) noexcept
{
  return map.GetPathIsEqual(key, value);
}

void
Profile::SetPath(const char *key, Path value) noexcept
{
  map.SetPath(key, value);
}
