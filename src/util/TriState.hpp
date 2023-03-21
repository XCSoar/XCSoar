// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <cstdint>

#ifdef _WIN32
#include <minwindef.h>
#elif defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

/* TRUE and FALSE are macros on some platforms (at least on Win32, iOS and
 * macOS) - undefine */
#ifdef TRUE
#undef TRUE
static constexpr bool TRUE = true;
#endif
#ifdef FALSE
#undef FALSE
static constexpr bool FALSE = false;
#endif

/**
 * A boolean type that has a third state called "unknown".
 */
enum class TriState : uint8_t {
  FALSE, TRUE, UNKNOWN
};
