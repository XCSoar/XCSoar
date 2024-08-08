// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  assert(!path.empty());

  if (auto i = std::find(data_paths.begin(), data_paths.end(), path);
      i != data_paths.end())
    data_paths.erase(i);

  data_paths.emplace_front(path);

#ifndef ANDROID
  cache_path = LocalPath(_T("cache"));
#endif
}

void
SetSingleDataPath(Path path) noexcept
{
  assert(path != nullptr);
  assert(!path.empty());

  data_paths.clear();
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

    for (auto &path : context->GetExternalMediaDirs(env)) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Context.getExternalMediaDirs()='%s'",
                          path.c_str());
      result.emplace_back(std::move(path));
    }

    if (auto path = Environment::GetExternalStoragePublicDirectory(env,
                                                                   "XCSoarData");
        path != nullptr) {
      const bool writable = access(path.c_str(), W_OK) == 0;

      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStoragePublicDirectory()='%s'%s",
                          path.c_str(),
                          writable ? "" : " (not accessible)");

      if (writable)
        /* the "legacy" external storage directory is writable (either
           because this is Android 10 or older, or because the
           "preserveLegacyExternalStorage" is still in effect) - we
           can use it */
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

  /* Windows: use "My Documents\XCSoarData" */
  {
    TCHAR buffer[MAX_PATH];
    if (SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PERSONAL,
                               result.empty()))
      result.emplace_back(AllocatedPath::Build(buffer, _T(XCSDATADIR)));
  }
#endif // _WIN32

#ifdef HAVE_POSIX
  /* on Unix, use ~/.xcsoar */
  if (const char *home = getenv("HOME"); home != nullptr) {
#ifdef __APPLE__
    /* Mac OS X users are not used to dot-files in their home
       directory - make it a little bit easier for them to find the
       files.  If target is an iOS device, use the already existing
       "Documents" folder inside the application's sandbox.  This
       folder can also be accessed via iTunes, if
       UIFileSharingEnabled is set to YES in Info.plist */
#if (TARGET_OS_IPHONE)
    constexpr const char *in_home = "Documents" XCSDATADIR;
#else
    constexpr const char *in_home = XCSDATADIR;
#endif
#else // !APPLE
    constexpr const char *in_home = ".xcsoar";
#endif

    result.emplace_back(AllocatedPath::Build(Path(home), in_home));
  }

#ifndef __APPLE__
  /* Linux (and others): allow global configuration in /etc/xcsoar */
  if (Directory::Exists(Path{"/etc/xcsoar"}))
    result.emplace_back(Path{"/etc/xcsoar"});
#endif // !APPLE
#endif // HAVE_POSIX

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

void
InitialiseDataPath()
{
  data_paths = FindDataPaths();
  if (data_paths.empty())
    throw std::runtime_error("No data path found");

#ifdef ANDROID
  cache_path = context->GetExternalCacheDir(Java::GetEnv());
  if (cache_path == nullptr)
    throw std::runtime_error("No Android cache directory");

  // TODO: delete the old cache directory in XCSoarData?
#else
  cache_path = LocalPath(_T("cache"));
#endif
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
