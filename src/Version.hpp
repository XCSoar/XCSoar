// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/** 5.2.5 */
extern const char XCSoar_Version[];
/** 5.2.5F */
extern const char XCSoar_VersionLong[];
/** 5.2.5F-PC */
extern const char XCSoar_VersionString[];
/** PC 5.2.5F 7. Oct 09 */
extern const char XCSoar_VersionStringOld[];
/** XCSoar v5.2.5F-PC */
extern const char XCSoar_ProductToken[];

/**
 * Four-digit calendar year from the compiler's \c __DATE__ macro (format
 * "MMM DD YYYY").  The value is fixed when each translation unit is compiled.
 */
[[nodiscard]] constexpr int
CompileDateYear() noexcept
{
  return (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 +
         (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
}
