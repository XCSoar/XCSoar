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

#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include <io.h>
#include "corecrt_math_defines.h"
#include <BaseTsd.h>

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define CLANG_OR_GCC_VERSION(x, y)  0
#define GCC_CHECK_VERSION(x, y)     0
// #define GCC_OLDER_THAN(x, y)        1  // This isn't a (new!) GCC
#define CLANG_CHECK_VERSION(x, y)   0

#define DT_UNDERLINE 0  // gibt es in WinUser.h nicht!

#define __attribute__(x)
// #define __attribute__(x) __declspec(x)

#define gcc_const
// not used: #define gcc_deprecated
// not used: #define gcc_may_alias
#define gcc_malloc
#define gcc_noreturn [[noreturn]]  // C17++
#if 0
// Unfortunately the PACKED structures definition are differently completely
// in MSVC yoe have to work with #pragma, on gcc/clang the definition of this
// is on the end of the structue, maybe this:
// https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
#ifdef _MSC_VER
#define PACKED_STRUCT(name)                                                    \
  __pragma(pack(push, 1)) struct name __pragma(pack(pop))
#elif defined(__GNUC__)
#define PACKED_STRUCT(name) struct __attribute__((packed)) name
#endif
#else
#define gcc_packed
#endif


#define gcc_printf(a,b)  // TODO(August2111): _Printf_format_string_ p
#define gcc_pure  //  TODO(August2111): is used!
// not used: #define gcc_sentinel    //  aug: sentinel
#define gcc_unused [[maybe_unused]] // C17++  //  aug: unused???
// not used: #define gcc_warn_unused_result  //  aug: warn_unused_result

// not used: #define gcc_nonnull(...)  // __assume(__VA_ARGS__ != nullptr)
#define gcc_nonnull_all   // __assume(!nullptr)
// not used: #define gcc_returns_nonnull // returns_nonnull

#define gcc_likely(x)  (x)
//  aug: __builtin_expect (!!(x), 1)
#define gcc_unlikely(x)  (x)
//  aug: __builtin_expect (!!(x), 0)

// not used: #define gcc_aligned(n)  //  aug: aligned(n)

// not used: #define gcc_visibility_hidden  //  aug: visibility("hidden")
// not used: #define gcc_visibility_default  //  aug: visibility("default")

#define gcc_always_inline
// inline  // wird dann mehrfach verwendet... //  aug: always_inline


// unuse deprecated functions:
#define strdup(a)             _strdup(a)
#define wcsdup(a)             _wcsdup(a)

typedef SSIZE_T ssize_t;

#define gcc_hot
  // not used: #define gcc_cold
#define gcc_flatten
#define gcc_fallthrough [[fallthrough]]  // C17++
#define gcc_restrict

#ifndef __has_feature
  // define dummy macro for non-clang compilers
  #define __has_feature(x) 0
#endif

#define gcc_unused_field  [[maybe_unused]]  // C17++

#define gcc_unreachable() __assume(0)   // GCC: __builtin_unreachable()

#endif COMPILER_H
