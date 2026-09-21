// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile.hpp"
#include "Asset.hpp"
#include "Current.hpp"
#include "File.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Map.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringUtil.hpp"

#include <string>
#include <cassert>
#include <windef.h> /* for MAX_PATH */

#define XCSPROFILE "default.prf"
#define OLDXCSPROFILE "xcsoar-registry.prf"

static AllocatedPath startProfileFile = nullptr;

/** True after Load() has been called for startProfileFile. */
static bool loaded = false;

static AllocatedPath
BuildProfilePath(Path base_name) noexcept
{
  return LocalPath(AllocatedPath::Build(Path("profiles"), base_name));
}

Path
Profile::GetPath() noexcept
{
  return startProfileFile;
}

void
Profile::Clear() noexcept
{
  map.Clear();
  SetModified(false);
  loaded = false;
}

void
Profile::Load() noexcept
{
  assert(startProfileFile != nullptr);

  LogString("Loading profiles");
  LoadFile(startProfileFile);
  loaded = true;
  SetModified(false);
}

void
Profile::LoadFile(Path path) noexcept
{
  try {
    LoadFile(map, path);
    LogFmt("Loaded profile from {}", path);
  } catch (...) {
    LogError(std::current_exception(), "Failed to load profile");
  }
}

void
Profile::Save() noexcept
{
  if (!loaded) {
    if (startProfileFile != nullptr)
      LogString("Skipping profile save: profile was never loaded");
    return;
  }

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
  LogFmt("Saving profile to {}", path);
  SaveFile(map, path);
}

void
Profile::SetFiles(Path override_path) noexcept
{
  /* only dirty the map after Load(): SetFiles() from -profile= runs
     while the map is still empty, and Save() must not persist that */
  if (loaded)
    SetModified(true);

  if (override_path != nullptr) {
    if (override_path.IsBase()) {
      if (StringFind(override_path.c_str(), '.') != nullptr)
        startProfileFile = BuildProfilePath(override_path);
      else {
        std::string t(override_path.c_str());
        t += ".prf";
        startProfileFile = BuildProfilePath(Path(t.c_str()));
      }
    } else
      startProfileFile = Path(override_path);
    return;
  }

  // Set the default profile file
  startProfileFile = BuildProfilePath(Path(XCSPROFILE));
}
