/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "LogFile.hpp"
#include "Thread/Mutex.hpp"
#include "LocalPath.hpp"
#include "Interface.hpp"
#include "Asset.hpp"

#include <stdio.h>
#include <stdarg.h>

static Mutex mutexLogFile;

#if !defined(NDEBUG) && !defined(GNAV)
/**
 * Saves the given string (Str) to the debug logfile
 * @param Str String to be logged
 */
void DebugStore(const char *Str, ...)
{
  char buf[MAX_PATH];
  va_list ap;
  int len;

  va_start(ap, Str);
  len = vsprintf(buf, Str, ap);
  va_end(ap);

  mutexLogFile.Lock();
  FILE *stream;
  TCHAR szFileName[] = TEXT("xcsoar-debug.log");
  static bool initialised = false;
  if (!initialised) {
    initialised = true;
    stream = _tfopen(szFileName,TEXT("w"));
  } else {
    stream = _tfopen(szFileName,TEXT("a+"));
  }

  fwrite(buf,len,1,stream);

  fclose(stream);
  mutexLogFile.Unlock();
}
#endif /* !NDEBUG */

/**
 * Saves the given string (Str) to the logfile
 * @param Str String to be logged
 */
void StartupStore(const TCHAR *Str, ...)
{
  TCHAR buf[MAX_PATH];
  va_list ap;

  va_start(ap, Str);
  _vstprintf(buf, Str, ap);
  va_end(ap);

  mutexLogFile.Lock();
  FILE *startupStoreFile = NULL;
  static TCHAR szFileName[MAX_PATH];
  static bool initialised = false;
  if (!initialised) {
    if (is_altair())
      LocalPath(szFileName, TEXT("persist/xcsoar-startup.log"));
    else
      LocalPath(szFileName, TEXT("xcsoar-startup.log"));
    startupStoreFile = _tfopen(szFileName, TEXT("wb"));
    if (startupStoreFile) {
      fclose(startupStoreFile);
    }
    initialised = true;
  }
  startupStoreFile = _tfopen(szFileName, TEXT("ab+"));
  if (startupStoreFile != NULL) {
    fprintf(startupStoreFile, "%S", buf);
    fclose(startupStoreFile);
  }
  mutexLogFile.Unlock();
}
