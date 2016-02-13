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

#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "Compatibility/path.h"
#include "Util/StringCompare.hxx"
#include "Util/StringFormat.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringBuilder.hxx"
#include "Asset.hpp"

#include "OS/FileUtil.hpp"

#ifdef ANDROID
#include "Android/Environment.hpp"
#endif

#ifdef WIN32
#include "OS/PathName.hpp"
#else
#include "Util/tstring.hpp"
#endif

#include <algorithm>

#include <assert.h>
#include <stdlib.h>
#include <windef.h> // for MAX_PATH
#ifdef WIN32
#ifdef HAVE_POSIX
#include <windows.h>
#else
#include <shlobj.h>
#endif
#endif

#ifdef ANDROID
#include <android/log.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define XCSDATADIR "XCSoarData"

/**
 * The default mount point of the SD card on Android.
 */
#define ANDROID_SDCARD "/sdcard"

/**
 * On the Samsung Galaxy Tab, the "external" SD card is mounted here.
 * Shame on the Samsung engineers, they didn't implement
 * Environment.getExternalStorageDirectory() properly.
 */
#define ANDROID_SAMSUNG_EXTERNAL_SD "/sdcard/external_sd"

/**
 * This is the partition that the Kobo software mounts on PCs
 */
#define KOBO_USER_DATA "/mnt/onboard"

/**
 * The absolute location of the XCSoarData directory.
 */
static AllocatedPath data_path = AllocatedPath(nullptr);

Path
GetPrimaryDataPath()
{
  assert(!data_path.IsNull());

  return data_path;
}

void
SetPrimaryDataPath(Path path)
{
  assert(!path.IsNull());
  assert(!path.IsEmpty());

  data_path = path;
}

AllocatedPath
LocalPath(Path file)
{
  assert(!data_path.IsNull());
  assert(!file.IsNull());

  return AllocatedPath::Build(data_path, file);
}

AllocatedPath
LocalPath(const TCHAR *file)
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
RelativePath(Path path)
{
  assert(!data_path.IsNull());

  return path.RelativeTo(data_path);
}

static constexpr TCHAR local_path_code[] = _T("%LOCAL_PATH%\\");

gcc_pure
static const TCHAR *
AfterLocalPathCode(const TCHAR *p)
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
ExpandLocalPath(Path src)
{
  // Get the relative file name and location (ptr)
  const TCHAR *ptr = AfterLocalPathCode(src.c_str());
  if (ptr == nullptr)
    return Path(src);

#ifndef WIN32
  // Convert backslashes to slashes on platforms where it matters
  tstring src2(src.c_str());
  std::replace(src2.begin(), src2.end(), '\\', '/');
  src = Path(src2.c_str());
#endif

  // Replace the code "%LOCAL_PATH%\\" by the full local path (output)
  return LocalPath(ptr);
}

AllocatedPath
ContractLocalPath(Path src)
{
  // Get the relative file name and location (ptr)
  const Path relative = RelativePath(src);
  if (relative.IsNull())
    return nullptr;

  // Replace the full local path by the code "%LOCAL_PATH%\\" (output)
  return Path(local_path_code) + relative.c_str();
}

#ifdef WIN32

/**
 * Find a XCSoarData folder in the same location as the executable.
 */
static AllocatedPath
FindDataPathAtModule(HMODULE hModule)
{
  TCHAR buffer[MAX_PATH];
  if (GetModuleFileName(hModule, buffer, MAX_PATH) <= 0)
    return nullptr;

  ReplaceBaseName(buffer, _T(XCSDATADIR));
  return Directory::Exists(Path(buffer))
    ? AllocatedPath(buffer)
    : nullptr;
}

#endif

#ifdef WIN32

static const TCHAR *
ModuleInFlash(HMODULE module, TCHAR *buffer)
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

#endif

#ifdef ANDROID

/**
 * Determine whether a text file contains a given string
 *
 * If two strings are given, the second string is considered
 * as no-match for the given line (i.e. string1 AND !string2).
 */
static bool
fgrep(const char *fname, const char *string, const char *string2 = nullptr)
{
  char line[100];
  FILE *fp;

  if ((fp = fopen(fname, "r")) == nullptr)
    return false;
  while (fgets(line, sizeof(line), fp) != nullptr)
    if (strstr(line, string) != nullptr &&
        (string2 == nullptr || strstr(line, string2) == nullptr)) {
        fclose(fp);
        return true;
    }
  fclose(fp);
  return false;
}

/**
 * See if the given mount point contains a writable directory called
 * XCSoarData.  If so, it returns an allocated absolute path to that
 * XCSoarData directory.
 */
static AllocatedPath
TryMountPoint(const TCHAR *mnt)
{
  auto path = AllocatedPath::Build(mnt, _T(XCSDATADIR));

  __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                      "Try '%s' exists=%d access=%d",
                      path.c_str(), Directory::Exists(path),
                      access(path.c_str(), W_OK));

  if (Directory::Exists(path) && access(path.c_str(), W_OK) == 0)
    return path;

  return nullptr;
}

#endif /* ANDROID */

/**
 * Returns the location of XCSoarData in the user's home directory.
 *
 * @param create true creates the path if it does not exist
 * @return a buffer which may be used to build the path
 */
static AllocatedPath
GetHomeDataPath(bool create=false)
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
                                /* Mac OS X users are not used to
                                   dot-files in their home directory -
                                   make it a little bit easier for
                                   them to find the files */
                                _T(XCSDATADIR)
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
FindDataPath()
{
#ifdef WIN32
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
    /* on Samsung Galaxy S4 (and others), the "external" SD card is
       mounted here */
    auto result = TryMountPoint("/mnt/extSdCard");
    if (result != nullptr)
      /* found writable XCSoarData: use this SD card */
      return result;

    /* hack for Samsung Galaxy S and Samsung Galaxy Tab (which has a
       built-in and an external SD card) */
    struct stat st;
    if (stat(ANDROID_SAMSUNG_EXTERNAL_SD, &st) == 0 &&
        S_ISDIR(st.st_mode) &&
        fgrep("/proc/mounts", ANDROID_SAMSUNG_EXTERNAL_SD " ", "tmpfs ")) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Enable Samsung hack, " XCSDATADIR " in "
                          ANDROID_SAMSUNG_EXTERNAL_SD);
      return Path(ANDROID_SAMSUNG_EXTERNAL_SD "/" XCSDATADIR);
    }

    /* try Context.getExternalStoragePublicDirectory() */
    char buffer[MAX_PATH];
    if (Environment::getExternalStoragePublicDirectory(buffer, sizeof(buffer),
                                                       "XCSoarData") != nullptr) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStoragePublicDirectory()='%s'",
                          buffer);
      return Path(buffer);
    }

    /* now try Context.getExternalStorageDirectory(), because
       getExternalStoragePublicDirectory() needs API level 8 */
    if (Environment::getExternalStorageDirectory(buffer,
                                                 sizeof(buffer) - 32) != nullptr) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStorageDirectory()='%s'",
                          buffer);

      return AllocatedPath::Build(buffer, XCSDATADIR);
    }

    /* hard-coded path for Android */
    __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                        "Fallback " XCSDATADIR " in " ANDROID_SDCARD);
#endif
    return Path(_T(ANDROID_SDCARD "/" XCSDATADIR));
  }

#ifdef WIN32
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

  {
    const auto home_path = GetHomeDataPath();
    if (home_path != nullptr && data_path != home_path)
      Directory::VisitSpecificFiles(home_path, filter, visitor, true);
  }
}

#ifdef ANDROID
/**
 * Resolve all symlinks in the specified (allocated) string, and
 * returns a newly allocated string.  The specified string is freed by
 * this function.
 */
static AllocatedPath
RealPath(Path path)
{
  char buffer[4096];
  char *result = realpath(path.c_str(), buffer);
  return AllocatedPath(result);
}
#endif

bool
InitialiseDataPath()
{
  data_path = FindDataPath();
  if (data_path == nullptr)
    return false;

#ifdef ANDROID
  /* on some Android devices, /sdcard or /sdcard/external_sd are
     symlinks, and on some devices (Samsung phones), the Android
     DownloadManager does not allow destination paths pointing inside
     these symlinks; to avoid problems with this restriction, all
     symlinks on the way must be resolved by RealPath(): */
  auto rp = RealPath(data_path);
  if (rp != nullptr)
    data_path = std::move(rp);
#endif

  return true;
}

void
DeinitialiseDataPath()
{
  data_path = nullptr;
}

void
CreateDataPath()
{
  Directory::Create(GetPrimaryDataPath());
}
