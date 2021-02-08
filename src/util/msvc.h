/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef UTIL_MSVC_COMPILER_H
#define UTIL_MSVC_COMPILER_H

#ifdef _MSC_VER
#if __cplusplus >= 201700L  // with C++17 it's this deprecated
                            // struct not available anymore
#   include "util/xcs_functional.hpp"
#endif
#   include <io.h>
#   include "corecrt_math_defines.h"

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define CLANG_OR_GCC_VERSION(x, y)  0
#define GCC_CHECK_VERSION(x, y)     0
#define GCC_OLDER_THAN(x, y)        1  // This isn't a (new!) GCC
#define CLANG_CHECK_VERSION(x, y)   0

#define DT_UNDERLINE 0            // not avalable in WinUser.h!

#define __attribute__(x)

#define gcc_const
#define gcc_deprecated
#define gcc_may_alias
#define gcc_malloc
#define gcc_noreturn
#define gcc_packed
#define gcc_printf(a,b)
#define gcc_pure
#define gcc_sentinel
#define gcc_unused
#define gcc_warn_unused_result
#define gcc_nonnull(...)
#define gcc_nonnull_all
#define gcc_returns_nonnull
#define gcc_likely(x)  (x)
#define gcc_unlikely(x)  (x)
#define gcc_aligned(n)
#define gcc_visibility_hidden
#define gcc_visibility_default
#define gcc_always_inline

// unuse deprecated functions:
#define strncasecmp(a, b, n)  _strnicmp(a, b, n)
#define strcasecmp(a, b)      _stricmp(a, b)
#define strdup(a)             _strdup(a)
#define wcsdup(a)             _wcsdup(a)

// TODO(August2111): where is this defined (on GCC)?
#ifndef ssize_t
typedef size_t   ssize_t;
#endif  // ssize_t

#endif  // _MSC_VER

#endif  // UTIL_MSVC_COMPILER_H
