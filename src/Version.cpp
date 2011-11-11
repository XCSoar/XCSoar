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

#include "Version.hpp"

#ifndef XCSOAR_VERSION
#error Macro "XCSOAR_VERSION" is not defined.  Check build/version.mk!
#endif

#define VERSION XCSOAR_VERSION

#ifdef GNAV
  #define TARGET "Altair"
#elif defined(ANDROID)
  #define TARGET "Android"
#elif defined(__linux__)
  #define TARGET "Linux"
#elif defined(__APPLE__)
  #define TARGET "MacOSX"
#elif !defined(WIN32)
  #define TARGET "UNIX"
#elif !defined(_WIN32_WCE)
  #define TARGET "PC"
#else
  #if _WIN32_WCE >= 0x0500
    #define TARGET "WM5"
  #elif _WIN32_WCE >= 0x0400
    #define TARGET "PPC2003"
  #else
    #define TARGET "PPC2000"
  #endif
#endif

#define VERSION_SUFFIX ""

#ifdef GIT_COMMIT_ID
#define GIT_SUFFIX "~git#" GIT_COMMIT_ID
#else
#define GIT_SUFFIX
#endif

const TCHAR XCSoar_Version[] = _T(VERSION);
const TCHAR XCSoar_VersionLong[] = _T(VERSION VERSION_SUFFIX);
const TCHAR XCSoar_VersionString[] = _T(VERSION VERSION_SUFFIX "-" TARGET);
const TCHAR XCSoar_VersionStringOld[] = _T(TARGET " " VERSION VERSION_SUFFIX " " __DATE__);
const TCHAR XCSoar_ProductToken[] = _T("XCSoar v" VERSION VERSION_SUFFIX "-" TARGET GIT_SUFFIX);
