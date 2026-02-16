// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef LIBCXX
/* libc++ uses "_T" as template argument names; this conflicts with
   the macro "_T()" defined below.  To work around that problem,
   include all relevant libc++ headers before defining _T() */
#include <iterator>
#endif

#define _T(x) x
