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

#include "LocalPath.hpp"
#include "Compatibility/path.h"
#include "Util/StringUtil.hpp"
#include "Asset.hpp"

#include "OS/FileUtil.hpp"

#ifdef ANDROID
#include "Android/Environment.hpp"
#endif

#ifdef WIN32
#include "OS/PathName.hpp"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <windef.h> // for MAX_PATH
#ifdef WIN32
#ifdef HAVE_POSIX
#include <windows.h>
#else
#include <shlobj.h>
#endif
#endif

#ifdef _WIN32_WCE
#include "OS/FlashCardEnumerator.hpp"
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
static TCHAR *gcc_restrict data_path;
static size_t data_path_length;

const TCHAR *
GetPrimaryDataPath()
{
  assert(data_path != NULL);

  return data_path;
}

void
SetPrimaryDataPath(const TCHAR *path)
{
  assert(path != NULL);
  assert(!StringIsEmpty(path));

  free(data_path);
  data_path = _tcsdup(path);
  data_path_length = _tcslen(data_path);
}

void
LocalPath(TCHAR *gcc_restrict buffer, const TCHAR *gcc_restrict file)
{
  assert(data_path != NULL);

  memcpy(buffer, data_path, data_path_length * sizeof(data_path[0]));
  buffer[data_path_length] = _T(DIR_SEPARATOR);
  _tcscpy(buffer + data_path_length + 1, file);
}

TCHAR *
LocalPath(TCHAR *gcc_restrict buffer, const TCHAR *gcc_restrict subdir,
          const TCHAR *gcc_restrict name)
{
  assert(data_path != NULL);
  assert(subdir != NULL);
  assert(!StringIsEmpty(subdir));
  assert(name != NULL);
  assert(!StringIsEmpty(name));

  memcpy(buffer, data_path, data_path_length * sizeof(data_path[0]));
  buffer[data_path_length] = _T(DIR_SEPARATOR);
  _tcscpy(buffer + data_path_length + 1, subdir);
  _tcscat(buffer + data_path_length + 1, _T(DIR_SEPARATOR_S));
  _tcscat(buffer + data_path_length + 1, name);

  return buffer;
}

const TCHAR *
RelativePath(const TCHAR *path)
{
  assert(data_path != NULL);

  const TCHAR *p = StringAfterPrefix(path, data_path);
  return p != NULL && IsDirSeparator(*p)
    ? p + 1
    : NULL;
}

/**
 * Convert backslashes to slashes on platforms where it matters.
 * @param p Pointer to the string to normalize
 */
static void
NormalizeBackslashes(TCHAR *p)
{
#if !defined(_WIN32) || defined(__WINE__)
  while ((p = _tcschr(p, '\\')) != NULL)
    *p++ = '/';
#endif
}

static constexpr TCHAR local_path_code[] = _T("%LOCAL_PATH%\\");

void
ExpandLocalPath(TCHAR *dest, const TCHAR *src)
{
  // Get the relative file name and location (ptr)
  const TCHAR *ptr = StringAfterPrefix(src, local_path_code);
  if (ptr == NULL) {
    _tcscpy(dest, src);
    return;
  }

  while (*ptr == _T('/') || *ptr == _T('\\'))
    ++ptr;

  if (StringIsEmpty(ptr))
    return;

  // Replace the code "%LOCAL_PATH%\\" by the full local path (output)
  LocalPath(dest, ptr);

  // Normalize the backslashes (if necessary)
  NormalizeBackslashes(dest);
}

void
ContractLocalPath(TCHAR* filein)
{
  TCHAR output[MAX_PATH];

  // Get the relative file name and location (ptr)
  const TCHAR *relative = RelativePath(filein);
  if (relative == NULL)
    return;

  // Replace the full local path by the code "%LOCAL_PATH%\\" (output)
  _stprintf(output, _T("%s%s"), local_path_code, relative);
  // ... and copy it to the buffer (filein)
  _tcscpy(filein, output);
}

#ifdef WIN32

/**
 * Find a XCSoarData folder in the same location as the executable.
 */
static const TCHAR *
FindDataPathAtModule(HMODULE hModule, TCHAR *buffer)
{
  if (GetModuleFileName(hModule, buffer, MAX_PATH) <= 0)
    return NULL;

  ReplaceBaseName(buffer, _T(XCSDATADIR));
  return Directory::Exists(buffer)
    ? buffer
    : NULL;
}

#endif

#ifdef _WIN32_WCE

static bool
InFlashNamed(const TCHAR *path, const TCHAR *name)
{
  size_t name_length = _tcslen(name);

  return IsDirSeparator(path[0]) &&
    memcmp(path + 1, name, name_length * sizeof(name[0])) == 0 &&
    IsDirSeparator(path[1 + name_length]);
}

/**
 * Determine whether the specified path is on a flash disk.  If yes,
 * it returns the absolute root path of the disk.
 */
static const TCHAR *
InFlash(const TCHAR *path, TCHAR *buffer)
{
  assert(path != NULL);
  assert(buffer != NULL);

  FlashCardEnumerator enumerator;
  const TCHAR *name;
  while ((name = enumerator.Next()) != NULL) {
    if (InFlashNamed(path, name)) {
      buffer[0] = DIR_SEPARATOR;
      _stprintf(buffer, _T(DIR_SEPARATOR_S"%s"), name);
      return buffer;
    }
  }

  return NULL;
}

static const TCHAR *
ModuleInFlash(HMODULE hModule, TCHAR *buffer)
{
  if (GetModuleFileName(hModule, buffer, MAX_PATH) <= 0)
    return NULL;

  return InFlash(buffer, buffer);
}

/**
 * Looks for a directory called "XCSoarData" on all flash disks.
 */
static const TCHAR *
ExistingDataOnFlash(TCHAR *buffer)
{
  assert(buffer != NULL);

  FlashCardEnumerator enumerator;
  const TCHAR *name;
  while ((name = enumerator.Next()) != NULL) {
    _stprintf(buffer, _T(DIR_SEPARATOR_S "%s" DIR_SEPARATOR_S XCSDATADIR), name);
    if (Directory::Exists(buffer))
      return buffer;
  }

  return NULL;
}

#elif defined(WIN32)

static const TCHAR *
ModuleInFlash(HMODULE module, TCHAR *buffer)
{
  if (GetModuleFileName(module, buffer, MAX_PATH) <= 0)
    return NULL;

  // At least "C:\"
  if (_tcslen(buffer) < 3 ||
      buffer[1] != _T(':') ||
      buffer[2] != _T('\\'))
    return NULL;

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
fgrep(const char *fname, const char *string, const char *string2 = NULL)
{
  char line[100];
  FILE *fp;

  if ((fp = fopen(fname, "r")) == NULL)
    return false;
  while (fgets(line, sizeof(line), fp) != NULL)
    if (strstr(line, string) != NULL &&
        (string2 == NULL || strstr(line, string2) == NULL)) {
        fclose(fp);
        return true;
    }
  fclose(fp);
  return false;
}

#endif /* ANDROID */

/**
 * Returns the location of XCSoarData in the user's home directory.
 *
 * @param create true creates the path if it does not exist
 * @return a buffer which may be used to build the path
 */
static const TCHAR *
GetHomeDataPath(TCHAR *gcc_restrict buffer, bool create=false)
{
  if (IsAndroid() || IsKobo())
    /* hard-coded path for Android */
    return NULL;

#ifdef HAVE_POSIX
  /* on Unix or WINE, use ~/.xcsoar */
  const TCHAR *home = getenv("HOME");
  if (home != NULL) {
    _tcscpy(buffer, home);
#ifdef __APPLE__
    /* Mac OS X users are not used to dot-files in their home
       directory - make it a little bit easier for them to find the
       files */
    _tcscat(buffer, _T("/XCSoarData"));
#else
    _tcscat(buffer, _T("/.xcsoar"));
#endif
    return buffer;
  } else
    return _T("/etc/xcsoar");
#else
  if (IsWindowsCE())
    /* clear the buffer, just in case we evaluate it after
       SHGetSpecialFolderPath() failure, see below */
    buffer[0] = _T('\0');

  bool success = SHGetSpecialFolderPath(NULL, buffer, CSIDL_PERSONAL, create);
  if (IsWindowsCE() && !success && !StringIsEmpty(buffer))
    /* MSDN: "If you are using the AYGShell extensions, then this
       function returns FALSE even if successful. If the folder
       represented by the CSIDL does not exist and is not created, a
       NULL string is returned indicating that the directory does not
       exist." */
    success = true;
  if (!success)
    return NULL;

  _tcscat(buffer, _T(DIR_SEPARATOR_S));
  _tcscat(buffer, _T(XCSDATADIR));
  return buffer;
#endif
}

static TCHAR *
FindDataPath()
{
  if (IsAltair() && IsEmbedded()) {
    /* if XCSoarData exists on USB drive, use that, because the
       internal memory is extremely small */
    const TCHAR *usb = _T("\\USB HD\\" XCSDATADIR);
    if (Directory::Exists(usb))
      return _tcsdup(usb);

    /* hard-coded path for Altair */
    return _tcsdup(_T("\\NOR Flash"));
  }

#ifdef WIN32
  {
    TCHAR buffer[MAX_PATH];
    const TCHAR *path = FindDataPathAtModule(NULL, buffer);
    if (path != NULL)
      return _tcsdup(path);
  }
#endif

  if (IsKobo())
    return _tcsdup(_T(KOBO_USER_DATA DIR_SEPARATOR_S XCSDATADIR));

  if (IsAndroid()) {
#ifdef ANDROID
    /* hack for Samsung Galaxy S and Samsung Galaxy Tab (which has a
       build-in and an external SD card) */
    struct stat st;
    if (stat(ANDROID_SAMSUNG_EXTERNAL_SD, &st) == 0 &&
        (st.st_mode & S_IFDIR) != 0 &&
        fgrep("/proc/mounts", ANDROID_SAMSUNG_EXTERNAL_SD " ", "tmpfs ")) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Enable Samsung hack, " XCSDATADIR " in "
                          ANDROID_SAMSUNG_EXTERNAL_SD);
      return strdup(ANDROID_SAMSUNG_EXTERNAL_SD "/" XCSDATADIR);
    }

    /* try Context.getExternalStoragePublicDirectory() */
    char buffer[MAX_PATH];
    if (Environment::getExternalStoragePublicDirectory(buffer, sizeof(buffer),
                                                       "XCSoarData") != NULL) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStoragePublicDirectory()='%s'",
                          buffer);
      return strdup(buffer);
    }

    /* now try Context.getExternalStorageDirectory(), because
       getExternalStoragePublicDirectory() needs API level 8 */
    if (Environment::getExternalStorageDirectory(buffer,
                                                 sizeof(buffer) - 32) != NULL) {
      __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                          "Environment.getExternalStorageDirectory()='%s'",
                          buffer);

      strcat(buffer, "/" XCSDATADIR);
      return strdup(buffer);
    }

    /* hard-coded path for Android */
    __android_log_print(ANDROID_LOG_DEBUG, "XCSoar",
                        "Fallback " XCSDATADIR " in " ANDROID_SDCARD);
#endif
    return _tcsdup(_T(ANDROID_SDCARD "/" XCSDATADIR));
  }

#ifdef WIN32
  /* if XCSoar was started from a flash disk, put the XCSoarData onto
     it, too */
  {
    TCHAR buffer[MAX_PATH];
    if (ModuleInFlash(NULL, buffer) != NULL) {
      _tcscat(buffer, _T(DIR_SEPARATOR_S));
      _tcscat(buffer, _T(XCSDATADIR));
      if (Directory::Exists(buffer))
        return _tcsdup(buffer);
    }

#ifdef _WIN32_WCE
    /* if a flash disk with XCSoarData exists, use it */
    if (ExistingDataOnFlash(buffer) != NULL)
      return _tcsdup(buffer);
#endif
  }
#endif

  {
    TCHAR buffer[MAX_PATH];
    const TCHAR *path = GetHomeDataPath(buffer, true);
    if (path != NULL)
      return _tcsdup(path);
  }

  return NULL;
}

void
VisitDataFiles(const TCHAR* filter, File::Visitor &visitor)
{
  const TCHAR *data_path = GetPrimaryDataPath();
  Directory::VisitSpecificFiles(data_path, filter, visitor, true);

  {
    TCHAR buffer[MAX_PATH];
    const TCHAR *home_path = GetHomeDataPath(buffer);
    if (home_path != NULL && _tcscmp(data_path, home_path) != 0)
      Directory::VisitSpecificFiles(home_path, filter, visitor, true);
  }

#if defined(_WIN32_WCE) && !defined(GNAV)
  TCHAR flash_path[MAX_PATH];
  FlashCardEnumerator enumerator;
  const TCHAR *flash_name;
  while ((flash_name = enumerator.Next()) != NULL) {
    _stprintf(flash_path, _T(DIR_SEPARATOR_S "%s" DIR_SEPARATOR_S XCSDATADIR),
              flash_name);
    if (_tcscmp(data_path, flash_path) == 0)
      /* don't scan primary data path twice */
      continue;

    Directory::VisitSpecificFiles(flash_path, filter, visitor, true);
  }
#endif /* _WIN32_WCE && !GNAV*/
}

#ifdef ANDROID
/**
 * Resolve all symlinks in the specified (allocated) string, and
 * returns a newly allocated string.  The specified string is freed by
 * this function.
 */
static char *
RealPath(char *path)
{
  char buffer[4096];
  char *result = realpath(path, buffer);
  if (result == NULL)
    return path;

  free(path);
  return strdup(result);
}
#endif

bool
InitialiseDataPath()
{
  data_path = FindDataPath();
  if (data_path == NULL)
    return false;

#ifdef ANDROID
  /* on some Android devices, /sdcard or /sdcard/external_sd are
     symlinks, and on some devices (Samsung phones), the Android
     DownloadManager does not allow destination paths pointing inside
     these symlinks; to avoid problems with this restriction, all
     symlinks on the way must be resolved by RealPath(): */
  data_path = RealPath(data_path);
#endif

  data_path_length = _tcslen(data_path);
  return true;
}

void
DeinitialiseDataPath()
{
  free(data_path);
}

void
CreateDataPath()
{
  Directory::Create(GetPrimaryDataPath());
}
