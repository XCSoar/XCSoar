/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"
#include "util/StringCompare.hxx"
#include "util/StringFormat.hpp"
#include "util/StringAPI.hxx"
#include "Asset.hpp"

#include "system/FileUtil.hpp"

#ifdef ANDROID
#include "Android/Context.hpp"
#include "Android/Environment.hpp"
#include "Android/Main.hpp"
#endif

#ifdef _WIN32
#include "system/PathName.hpp"
#else
#include "util/tstring.hpp"
#endif

#include <algorithm>
#include <list>

#include <cassert>
#include <stdlib.h>
#ifdef _WIN32
#include <shlobj.h>
#include <windef.h> // for MAX_PATH
#endif

#ifdef ANDROID
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define XCSDATADIR "XCSoarData"

/**
 * This is the partition that the Kobo software mounts on PCs
 */
#define KOBO_USER_DATA "/mnt/onboard"

/**
 * A list of XCSoarData directories.  The first one is the primary
 * one, where "%LOCAL_PATH%\\" refers to.
 */
static std::list<AllocatedPath> data_paths;

static AllocatedPath cache_path;

Path
GetPrimaryDataPath() noexcept
{
  assert(!data_paths.empty());

  return data_paths.front();
}

void
SetPrimaryDataPath(Path path) noexcept
{
  assert(path != nullptr);
  assert(!path.IsEmpty());

  if (auto i = std::find(data_paths.begin(), data_paths.end(), path);
      i != data_paths.end())
    data_paths.erase(i);

  data_paths.emplace_front(path);

#ifndef ANDROID
  cache_path = LocalPath(_T("cache"));
#endif
}

AllocatedPath
LocalPath(Path file) noexcept
{
  assert(file != nullptr);

  return AllocatedPath::Build(GetPrimaryDataPath(), file);
}

AllocatedPath
LocalPath(const TCHAR *file) noexcept
{
  return LocalPath(Path(file));
}

AllocatedPath
MakeLocalPath(const TCHAR *name)
{
  auto path = LocalPath(name);
  Directory::Create(path);
  return path;
}

Path
RelativePath(Path path) noexcept
{
  return path.RelativeTo(GetPrimaryDataPath());
}

static constexpr TCHAR local_path_code[] = _T("%LOCAL_PATH%\\");

[[gnu::pure]]
static const TCHAR *
AfterLocalPathCode(const TCHAR *p) noexcept
{
  p = StringAfterPrefix(p, local_path_code);
  if (p == nullptr)
    return nullptr;

  while (*p == _T('/') || *p == _T('\\'))
    ++p;

  if (StringIsEmpty(p))
    return nullptr;

  return p;
}

AllocatedPath
ExpandLocalPath(Path src) noexcept
{
  // Get the relative file name and location (ptr)
  const TCHAR *ptr = AfterLocalPathCode(src.c_str());
  if (ptr == nullptr)
    return src;

#ifndef _WIN32
  // Convert backslashes to slashes on platforms where it matters
  tstring src2(ptr);
  std::replace(src2.begin(), src2.end(), '\\', '/');
  ptr = src2.c_str();
#endif

  // Replace the code "%LOCAL_PATH%\\" by the full local path (output)
  return LocalPath(ptr);
}

AllocatedPath
ContractLocalPath(Path src) noexcept
{
  // Get the relative file name and location (ptr)
  const Path relative = RelativePath(src);
  if (relative == nullptr)
    return nullptr;

  // Replace the full local path by the code "%LOCAL_PATH%\\" (output)
  return Path(local_path_code) + relative.c_str();
}

#ifdef _WIN32

/**
 * Find a XCSoarData folder in the same location as the executable.
 */
[[gnu::pure]]
static AllocatedPath
FindDataPathAtModule(HMODULE hModule) noexcept
{
  TCHAR buffer[MAX_PATH];
  if (GetModuleFileName(hModule, buffer, MAX_PATH) <= 0)
    return nullptr;

  ReplaceBaseName(buffer, _T(XCSDATADIR));
  return Directory::Exists(Path(buffer))
    ? AllocatedPath(buffer)
    : nullptr;
}

#endif /* _WIN32 */

/**
 * Returns the location of XCSoarData in the user's home directory.
 *
 * @param create true creates the path if it does not exist
 * @return a buffer which may be used to build the path
 */
static AllocatedPath
GetHomeDataPath(bool create=false) noexcept
{
  if constexpr (IsAndroid() || IsKobo())
    /* hard-coded path for Android */
    return nullptr;

#ifdef HAVE_POSIX
  /* on Unix, use ~/.xcsoar */
  if (const TCHAR *home = getenv("HOME"); home != nullptr) {
    return AllocatedPath::Build(Path(home),
#ifdef __APPLE__
    /* Mac OS X users are not used to dot-files in their home
       directory - make it a little bit easier for them to find the
       files.
       If target is an iOS device, use the already existing "Documents" folder
       inside the application's sandbox.
       This folder can also be accessed via iTunes, if UIFileSharingEnabled is set
       to YES in Info.plist
    */
#if (TARGET_OS_IPHONE)
    _T("Documents")
#else
    _T(XCSDATADIR)
#endif
#else
                                _T("/.xcsoar")
#endif
                                );
  } else
    return nullptr;
#else

  TCHAR buffer[MAX_PATH];
  bool success = SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PERSONAL,
                                        create);
  if (!success)
    return nullptr;

  return AllocatedPath::Build(buffer, _T(XCSDATADIR));
#endif
}

static std::list<AllocatedPath>
FindDataPaths() noexcept
{
  std::list<AllocatedPath> result;

  /* Kobo: hard-coded XCSoarData path */
  if constexpr (IsKobo()) {
    result.emplace_back(_T(KOBO_USER_DATA DIR_SEPARATOR_S XCSDATADIR));
    return result;
  }

  /* Android: ask the Android API */
  if constexpr (IsAndroid()) {
#ifdef ANDROID
    const auto env = Java::GetEnv();

    /* try Context.getExternalStoragePublicDirectory() */
    if (auto path = Environment::GetExternalStoragePublicDirectory(env,
                                                                   "XCSoarData");
        path != nullptr) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStoragePublicDirectory()='%s'",
                          path.c_str());
      result.emplace_back(std::move(path));
    }
#endif

    return result;
  }

#ifdef _WIN32
  /* look for a XCSoarData directory in the same directory as
     XCSoar.exe */
  if (auto path = FindDataPathAtModule(nullptr); path != nullptr)
    result.emplace_back(std::move(path));
#endif

  if (auto path = GetHomeDataPath(result.empty());
      path != nullptr && path != result.front())
    result.emplace_back(std::move(path));

  /* Linux (and others): allow global configuration in /etc/xcsoar */
#ifdef HAVE_POSIX
  if (!IsEmbedded() && Directory::Exists(Path{"/etc/xcsoar"}))
    data_paths.emplace_back(Path{"/etc/xcsoar"});
#endif

  return result;
}

void
VisitDataFiles(const TCHAR* filter, File::Visitor &visitor)
{
  for (const auto &i : data_paths)
    Directory::VisitSpecificFiles(i, filter, visitor, true);
}

Path
GetCachePath() noexcept
{
  return cache_path;
}

AllocatedPath
MakeCacheDirectory(const TCHAR *name) noexcept
{
  Directory::Create(cache_path);
  auto path = AllocatedPath::Build(cache_path, Path(name));
  Directory::Create(path);
  return path;
}

bool
InitialiseDataPath()
{
  data_paths = FindDataPaths();
  if (data_paths.empty())
    return false;

#ifdef ANDROID
  cache_path = context->GetExternalCacheDir(Java::GetEnv());
  if (cache_path == nullptr)
    throw std::runtime_error("No Android cache directory");

  // TODO: delete the old cache directory in XCSoarData?
#else
  cache_path = LocalPath(_T("cache"));
#endif

  return true;
}

void
DeinitialiseDataPath() noexcept
{
  data_paths.clear();
}

void
CreateDataPath()
{
  Directory::Create(GetPrimaryDataPath());
}
