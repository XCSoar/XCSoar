/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef WINDOWS_H
#define WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

/* GDI */

LRESULT DefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

enum {
  IDCANCEL = 0,
  MB_OKCANCEL = 0,
  MB_OK = 0,
  MB_ICONINFORMATION = 0,
  MB_ICONWARNING = 0,
  MB_ICONEXCLAMATION = 0,
  MB_YESNO = 0,
  MB_ICONQUESTION = 0,
  IDYES = 0,
  MB_ICONERROR = 0,
};

int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);



static inline DWORD
GetTickCount(void)
{
  return time(NULL);
}

int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

/* Debugging */

#include <assert.h>

#define ASSERT assert

/* Memory allocation */

enum {
  LMEM_FIXED = 0,
  LMEM_MOVEABLE = 0,
  LMEM_ZEROINIT = 0x40,
  LPTR = LMEM_FIXED|LMEM_ZEROINIT,
};

typedef void *HLOCAL;

static inline HLOCAL
LocalAlloc(UINT uFlags, size_t uBytes)
{
  void *p = malloc(uBytes);
  if (p == NULL)
    return NULL;

  if (uFlags & LMEM_ZEROINIT)
    memset(p, 0, uBytes);

  return p;
}

static inline HLOCAL
LocalReAlloc(HLOCAL hMem, UINT uFlags, size_t uBytes)
{
  // XXX
  return LocalAlloc(uFlags, uBytes);
}

static inline HLOCAL
LocalFree(HLOCAL h)
{
  free(h);
  return NULL;
}

/* MultiThreading hacks */

/*
#include <pthread.h>

typedef pthread_mutex_t CRITICAL_SECTION;

static inline void
InitializeCriticalSection(CRITICAL_SECTION *mutex)
{
  pthread_mutex_init(mutex, NULL);
}

static inline void
DeleteCriticalSection(CRITICAL_SECTION *mutex)
{
  pthread_mutex_destroy(mutex);
}

static inline void
EnterCriticalSection(CRITICAL_SECTION *mutex)
{
  pthread_mutex_lock(mutex);
}

static inline void
LeaveCriticalSection(CRITICAL_SECTION *mutex)
{
  pthread_mutex_unlock(mutex);
}
*/

/* Debugging */

#include <assert.h>

#define ASSERT assert

/* File I/O */

#define DeleteFile unlink

enum {
  GENERIC_READ = 0,
  GENERIC_WRITE = 0,
  CREATE_ALWAYS = 0,
  OPEN_EXISTING = 0,
  FILE_ATTRIBUTE_NORMAL = 0,
};

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME, *PFILETIME;

static inline BOOL
ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
         LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
  int fd = (int)(size_t)hFile;
  ssize_t nbytes;

  nbytes = read(fd, lpBuffer, nNumberOfBytesToRead);
  if (nbytes < 0) {
    *lpNumberOfBytesRead = 0;
    return FALSE;
  }

  *lpNumberOfBytesRead = (DWORD)nbytes;
  return TRUE;
}

static inline void MessageBeep(UINT uType)
{
}

static inline void Sleep(unsigned ms)
{
  const struct timespec ts = { ms / 1000, (long)ms % 1000000L };
  nanosleep(&ts, NULL);
}

typedef HANDLE HMENU;

enum {
  MF_CHECKED,
  MF_UNCHECKED,
  MF_BYCOMMAND,
};

static inline void CheckMenuItem(HMENU h, int id, unsigned flags) {}

static inline void SystemIdleTimerReset() {}

#endif
