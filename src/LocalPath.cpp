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

#include "LocalPath.hpp"
#include "Compatibility/path.h"
#include "StringUtil.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h> // for MAX_PATH

#ifdef WIN32
#include <shlobj.h>
#endif

#ifdef _WIN32_WCE
#include "OS/FlashCardEnumerator.hpp"
#include "OS/FileUtil.hpp"
#endif

#ifdef ANDROID
#include <sys/stat.h>
#include <unistd.h>
#endif

/**
 * The absolute location of the XCSoarData directory.
 */
static TCHAR *data_path;
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
  assert(!string_is_empty(path));

  free(data_path);
  data_path = _tcsdup(path);
}

void
LocalPath(TCHAR *buffer, const TCHAR *file)
{
  assert(data_path != NULL);

  memcpy(buffer, data_path, data_path_length * sizeof(data_path[0]));
  buffer[data_path_length] = _T(DIR_SEPARATOR);
  _tcscpy(buffer + data_path_length + 1, file);
}

/**
 * Convert backslashes to slashes on platforms where it matters.
 * @param p Pointer to the string to normalize
 */
static void
normalize_backslashes(TCHAR *p)
{
#if !defined(_WIN32) || defined(__WINE__)
  while ((p = _tcschr(p, '\\')) != NULL)
    *p++ = '/';
#endif
}

static const TCHAR local_path_code[] = _T("%LOCAL_PATH%\\");

void
ExpandLocalPath(TCHAR* filein)
{
  TCHAR output[MAX_PATH];

  // Get the relative file name and location (ptr)
  const TCHAR *ptr = string_after_prefix(filein, local_path_code);
  if (ptr == NULL)
    return;

  while (*ptr == _T('/') || *ptr == _T('\\'))
    ++ptr;

  if (string_is_empty(ptr))
    return;

  // Replace the code "%LOCAL_PATH%\\" by the full local path (output)
  LocalPath(output, ptr);
  // ... and copy it to the buffer (filein)
  _tcscpy(filein, output);

  // Normalize the backslashes (if necessary)
  normalize_backslashes(filein);
}

void
ContractLocalPath(TCHAR* filein)
{
  TCHAR output[MAX_PATH];

  // Get the relative file name and location (ptr)
  const TCHAR *ptr = string_after_prefix(filein, data_path);
  if (ptr == NULL || !is_dir_separator(*ptr))
    return;

  // Replace the full local path by the code "%LOCAL_PATH%\\" (output)
  _stprintf(output, _T("%s%s"), local_path_code, ptr + 1);
  // ... and copy it to the buffer (filein)
  _tcscpy(filein, output);
}

#ifdef _WIN32_WCE

static bool
InFlashNamed(const TCHAR *path, const TCHAR *name)
{
  size_t name_length = _tcslen(name);

  return is_dir_separator(path[0]) &&
    memcmp(path + 1, name, name_length * sizeof(name[0])) == 0 &&
    is_dir_separator(path[1 + name_length]);
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
  while ((name = enumerator.next()) != NULL) {
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
  while ((name = enumerator.next()) != NULL) {
    _stprintf(buffer, _T(DIR_SEPARATOR_S"%s"DIR_SEPARATOR_S) XCSDATADIR, name);
    if (Directory::Exists(buffer))
      return buffer;
  }

  return NULL;
}

#endif /* _WIN32_WCE */

#ifdef ANDROID

/**
 * Determine whether a text file contains a given string
 */
static bool
fgrep(const char *fname, const char *string)
{
  char line[100];
  FILE *fp;

  if ((fp = fopen(fname, "r")) == NULL)
    return false;
  while (fgets(line, sizeof(line), fp) != NULL)
    if (strstr(line, string) != NULL) {
      fclose(fp);
      return true;
    }
  fclose(fp);
  return false;
}

#endif /* ANDROID */

static TCHAR *
FindDataPath()
{
  if (is_altair() && is_embedded())
    /* hard-coded path for Altair */
    return _tcsdup(_T("\\NOR Flash"));

  if (is_android()) {
    /* XXX use Environment.getExternalStorageDirectory() */

#ifdef ANDROID
    /* hack for Samsung Galaxy S and Samsung Galaxy Tab (which has a
       build-in and an external SD card) */
    struct stat st;
    if (stat("/sdcard/external_sd", &st) == 0 &&
        (st.st_mode & S_IFDIR) != 0 &&
        fgrep("/proc/mounts", "/sdcard/external_sd "))
      return strdup("/sdcard/external_sd/XCSoarData");
#endif

    /* hard-coded path for Android */
    return _tcsdup(_T("/sdcard/XCSoarData"));
  }

#ifdef _WIN32_WCE
  /* if XCSoar was started from a flash disk, put the XCSoarData onto
     it, too */
  {
    TCHAR buffer[MAX_PATH];
    if (ModuleInFlash(NULL, buffer) != NULL) {
      _tcscat(buffer, _T(DIR_SEPARATOR_S));
      _tcscat(buffer, XCSDATADIR);
      if (Directory::Exists(buffer))
        return _tcsdup(buffer);
    }

    /* if a flash disk with XCSoarData exists, use it */
    if (ExistingDataOnFlash(buffer) != NULL)
      return _tcsdup(buffer);
  }
#endif

  {
    TCHAR buffer[MAX_PATH];
    const TCHAR *path = GetHomeDataPath(buffer);
    if (path != NULL)
      return _tcsdup(path);
  }

  return NULL;
}

const TCHAR *
GetHomeDataPath(TCHAR *buffer)
{
  if (is_android())
    /* hard-coded path for Android */
    return NULL;

#ifdef HAVE_POSIX
  /* on Unix or WINE, use ~/.xcsoar */
  const TCHAR *home = getenv("HOME");
  if (home != NULL) {
    _tcscpy(buffer, home);
    _tcscat(buffer, _T("/.xcsoar"));
    return buffer;
  } else
    return _T("/etc/xcsoar");
#else
  if (!SHGetSpecialFolderPath(NULL, buffer, CSIDL_PERSONAL, false))
    return NULL;

  _tcscat(buffer, _T(DIR_SEPARATOR_S));
  _tcscat(buffer, XCSDATADIR);
  return buffer;
#endif
}

struct ScopePathGlobalInit {
  ScopePathGlobalInit() {
    data_path = FindDataPath();
    assert(data_path != NULL);

    data_path_length = _tcslen(data_path);
  }

  ~ScopePathGlobalInit() {
    free(data_path);
  }
};

static const ScopePathGlobalInit path_global_init;
