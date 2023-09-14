// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define GCC_MAKE_VERSION(major, minor, patchlevel) ((major) * 10000 + (minor) * 100 + patchlevel)

#ifdef __GNUC__
#define GCC_VERSION GCC_MAKE_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#define GCC_VERSION 0
#endif

#ifdef __clang__
#  define CLANG_VERSION GCC_MAKE_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
#  define CLANG_VERSION 0
#endif

/**
 * Are we building with the specified version of gcc (not clang or any
 * other compiler) or newer?
 */
#define GCC_CHECK_VERSION(major, minor) \
  (!CLANG_VERSION && \
   GCC_VERSION >= GCC_MAKE_VERSION(major, minor, 0))

/**
 * Are we building with clang (any version) or at least the specified
 * gcc version?
 */
#define CLANG_OR_GCC_VERSION(major, minor) \
  (CLANG_VERSION || GCC_CHECK_VERSION(major, minor))

/**
 * Are we building with gcc (not clang or any other compiler) and a
 * version older than the specified one?
 */
#define GCC_OLDER_THAN(major, minor) \
  (GCC_VERSION && !CLANG_VERSION && \
   GCC_VERSION < GCC_MAKE_VERSION(major, minor, 0))

#ifdef __clang__
#  if CLANG_VERSION < GCC_MAKE_VERSION(10,0,0)
#    error Sorry, your clang version is too old.  You need at least version 10.
#  endif
#elif defined(__GNUC__)
#  if GCC_OLDER_THAN(10,0)
#    error Sorry, your gcc version is too old.  You need at least version 10.
#  endif
#else
#  warning Untested compiler.  Use at your own risk!
#endif

/**
 * Are we building with the specified version of clang or newer?
 */
#define CLANG_CHECK_VERSION(major, minor) \
  (CLANG_VERSION >= GCC_MAKE_VERSION(major, minor, 0))

#ifdef __GNUC__

/* GCC 4.x */

#define gcc_const __attribute__((const))
#define gcc_malloc __attribute__((malloc))
#define gcc_packed __attribute__((packed))
#define gcc_printf(a,b) __attribute__((format(printf, a, b)))
#define gcc_pure __attribute__((pure))
#define gcc_unused __attribute__((unused))

#define gcc_visibility_default __attribute__((visibility("default")))

#else

/* generic C compiler */

#define gcc_const
#define gcc_malloc
#define gcc_packed
#define gcc_printf(a,b)
#define gcc_pure
#define gcc_unused

#define gcc_visibility_default

#endif

#ifndef __cplusplus
/* plain C99 has "restrict" */
#define gcc_restrict restrict
#elif defined(__GNUC__)
/* "__restrict__" is a GCC extension for C++ */
#define gcc_restrict __restrict__
#else
/* disable it on other compilers */
#define gcc_restrict
#endif

#ifndef __has_feature
  // define dummy macro for non-clang compilers
  #define __has_feature(x) 0
#endif

#if __has_feature(attribute_unused_on_fields)
#define gcc_unused_field gcc_unused
#else
#define gcc_unused_field
#endif

#if defined(__GNUC__) || defined(__clang__)
#define gcc_unreachable() __builtin_unreachable()
#else
#define gcc_unreachable()
#endif
