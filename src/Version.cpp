// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
    #define TARGET "macOS"
  #endif
#elif !defined(_WIN32)
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

const char XCSoar_Version[] = VERSION;
const TCHAR XCSoar_VersionLong[] = _T(VERSION VERSION_SUFFIX);
const TCHAR XCSoar_VersionString[] = _T(VERSION VERSION_SUFFIX "-" TARGET);
const TCHAR XCSoar_VersionStringOld[] = _T(TARGET " " VERSION VERSION_SUFFIX);
const TCHAR XCSoar_ProductToken[] = _T("XCSoar v" VERSION VERSION_SUFFIX "-" TARGET GIT_SUFFIX);
