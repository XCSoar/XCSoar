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

#include "UtilsSystem.hpp"
#include "CommandLine.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "OS/MemInfo.hpp"
#include "Screen/Point.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <tchar.h>

#ifdef HAVE_POSIX
#ifndef ANDROID
#include <sys/statvfs.h>
#endif
#include <sys/stat.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_VIDEOCORE
#include <bcm_host.h>
#endif

#ifdef _WIN32_WCE
// This is necessary to be called periodically to get rid of
// memory defragmentation, since on pocket pc platforms there is no
// automatic defragmentation.
void MyCompactHeaps() {
#if defined(GNAV) && !defined(__GNUC__)
  HeapCompact(GetProcessHeap(), 0);
#else
  typedef DWORD (_stdcall *CompactAllHeapsFn) (void);
  static CompactAllHeapsFn CompactAllHeaps = NULL;
  static bool init = false;
  if (!init) {
    // get the pointer to the function
    CompactAllHeaps = (CompactAllHeapsFn)GetProcAddress(
        LoadLibrary(_T("coredll.dll")), _T("CompactAllHeaps"));
    init = true;
  }
  if (CompactAllHeaps)
    CompactAllHeaps();
#endif
}
#endif /* _WIN32_WCE */

/**
 * Calculates the free disk space for the given path
 * @param path The path defining the "drive" to look on
 * @return Number of KiB free on the destination drive
 */
unsigned long FindFreeSpace(const TCHAR *path) {
#ifdef HAVE_POSIX
#ifdef ANDROID
  return 64 * 1024 * 1024;
#else
  struct statvfs s;
  if (statvfs(path, &s) < 0)
    return 0;
  return s.f_bsize * s.f_bavail / 1024;
#endif
#else /* !HAVE_POSIX */
  ULARGE_INTEGER FreeBytesAvailableToCaller;
  ULARGE_INTEGER TotalNumberOfBytes;
  ULARGE_INTEGER TotalNumberOfFreeBytes;
  if (GetDiskFreeSpaceEx(path,
                         &FreeBytesAvailableToCaller,
                         &TotalNumberOfBytes,
                         &TotalNumberOfFreeBytes)) {
    return FreeBytesAvailableToCaller.LowPart / 1024;
  } else
    return 0;
#endif /* !HAVE_POSIX */
}

void
StartupLogFreeRamAndStorage()
{
#ifdef HAVE_MEM_INFO
  unsigned long freeram = SystemFreeRAM() / 1024;
  LogFormat("Free ram %lu KB", freeram);
#endif
  unsigned long freestorage = FindFreeSpace(GetPrimaryDataPath());
  LogFormat("Free storage %lu KB", freestorage);
}

/**
 * Returns the screen dimension rect to be used
 * @return The screen dimension rect to be used
 */
PixelSize
SystemWindowSize()
{
#if defined(WIN32) && !defined(_WIN32_WCE)
  unsigned width = CommandLine::width + 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
  unsigned height = CommandLine::height + 2 * GetSystemMetrics(SM_CYFIXEDFRAME)
    + GetSystemMetrics(SM_CYCAPTION);

  return { width, height };
#else
  #ifdef WIN32
  return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
#elif defined(ANDROID)
  return native_view->GetSize();
#elif defined(USE_VIDEOCORE)
  uint32_t width, height;
  return graphics_get_display_size(0, &width, &height) >= 0
    ? PixelSize(width, height)
    : PixelSize(640, 480);
#else
  /// @todo implement this properly for SDL/UNIX
  return { CommandLine::width, CommandLine::height };
  #endif /* !WIN32 */

#endif
}
