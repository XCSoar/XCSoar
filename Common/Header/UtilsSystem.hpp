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
#if !defined(XCSOAR_UTILS_SYSTEM_H)
#define XCSOAR_UTILS_SYSTEM_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#ifdef PNA
bool SetBacklight(); // VENTA4-ADDON for PNA
bool SetSoundVolume(); // VENTA4-ADDON for PNA
#endif

#if defined(PNA) || defined(FIVV)  // VENTA-ADDON
void SetModelType();
bool SetModelName(DWORD Temp);
#endif

void XCSoarGetOpts(LPTSTR CommandLine);
int MeasureCPULoad();
long CheckFreeRam(void);
void MemCheckPoint();
void MemLeakCheck();
void MyCompactHeaps();
unsigned long FindFreeSpace(const TCHAR *path);
BOOL PlayResource (const TCHAR* lpName);
void CreateDirectoryIfAbsent(const TCHAR *filename);

#ifdef __cplusplus
extern "C"{
#endif

bool FileExistsW(const TCHAR *FileName);
bool FileExistsA(const char *FileName);

#ifdef _UNICODE
#define FileExists FileExistsW
#else
#define FileExists FileExistsA
#endif

#ifdef __cplusplus
}
#endif

bool RotateScreen(void);

#endif
