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

#include "Version.hpp"

#ifndef XCSOAR_VERSION
#error Macro "XCSOAR_VERSION" is not defined.  Check build/version.mk!
#endif

#define VERSION XCSOAR_VERSION

#if defined(ANDROID)
  #define TARGET "Android"
#elif defined(KOBO)
  #define TARGET "Kobo"
#elif defined(__linux__)
  #define TARGET "Linux"
#elif defined(__APPLE__)
  #include <TargetConditionals.h>
  #if TARGET_OS_IPHONE
    #define TARGET "iOS"
  #else
    #define TARGET "OSX"
  #endif
#elif !defined(WIN32)
  #define TARGET "UNIX"
#else
  #define TARGET "PC"
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
