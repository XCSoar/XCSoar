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
 * The absolute location of the XCSoarData directory.
 */
static AllocatedPath data_path;
static AllocatedPath cache_path;

Path
GetPrimaryDataPath() noexcept
{
  assert(data_path != nullptr);

  return data_path;
}

void
SetPrimaryDataPath(Path path) noexcept
{
  assert(path != nullptr);
  assert(!path.IsEmpty());

  data_path = path;

#ifndef ANDROID
  cache_path = LocalPath(_T("cache"));
#endif
}

AllocatedPath
LocalPath(Path file) noexcept
{
  assert(data_path != nullptr);
  assert(file != nullptr);

  return AllocatedPath::Build(data_path, file);
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
  assert(data_path != nullptr);

  return path.RelativeTo(data_path);
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

static const TCHAR *
ModuleInFlash(HMODULE module, TCHAR *buffer) noexcept
{
  if (GetModuleFileName(module, buffer, MAX_PATH) <= 0)
    return nullptr;

  // At least "C:\"
  if (StringLength(buffer) < 3 ||
      buffer[1] != _T(':') ||
      buffer[2] != _T('\\'))
    return nullptr;

  // Trim the module path to the drive letter plus colon
  buffer[2] = _T('\0');
  return buffer;
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
  if (IsAndroid() || IsKobo())
    /* hard-coded path for Android */
    return nullptr;

#ifdef HAVE_POSIX
  /* on Unix, use ~/.xcsoar */
  const TCHAR *home = getenv("HOME");
  if (home != nullptr) {
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
    return Path("/etc/xcsoar");
#else

  TCHAR buffer[MAX_PATH];
  bool success = SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PERSONAL,
                                        create);
  if (!success)
    return nullptr;

  return AllocatedPath::Build(buffer, _T(XCSDATADIR));
#endif
}

static AllocatedPath
FindDataPath() noexcept
{
#ifdef _WIN32
  {
    auto path = FindDataPathAtModule(nullptr);
    if (path != nullptr)
      return path;
  }
#endif

  if (IsKobo())
    return Path(Path(_T(KOBO_USER_DATA DIR_SEPARATOR_S XCSDATADIR)));

  if (IsAndroid()) {
#ifdef ANDROID
    /* try Context.getExternalStoragePublicDirectory() */
    if (auto path = Environment::getExternalStoragePublicDirectory("XCSoarData");
        path != nullptr) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStoragePublicDirectory()='%s'",
                          path.c_str());
      return path;
    }
#endif
  }

#ifdef _WIN32
  /* if XCSoar was started from a flash disk, put the XCSoarData onto
     it, too */
  {
    TCHAR buffer[MAX_PATH];
    if (ModuleInFlash(nullptr, buffer) != nullptr) {
      _tcscat(buffer, _T(DIR_SEPARATOR_S));
      _tcscat(buffer, _T(XCSDATADIR));
      if (Directory::Exists(Path(buffer)))
        return Path(buffer);
    }
  }
#endif

  {
    auto path = GetHomeDataPath(true);
    if (path != nullptr)
      return path;
  }

  return nullptr;
}

void
VisitDataFiles(const TCHAR* filter, File::Visitor &visitor)
{
  const auto data_path = GetPrimaryDataPath();
  Directory::VisitSpecificFiles(data_path, filter, visitor, true);

  if (const auto home_path = GetHomeDataPath();
      home_path != nullptr && data_path != home_path)
    Directory::VisitSpecificFiles(home_path, filter, visitor, true);
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
  data_path = FindDataPath();
  if (data_path == nullptr)
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
  data_path = nullptr;
}

void
CreateDataPath()
{
  Directory::Create(GetPrimaryDataPath());
}
