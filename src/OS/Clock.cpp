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

#include "OS/Clock.hpp"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#elif defined(HAVE_POSIX) && !defined(__CYGWIN__)
#include <time.h>
#ifndef __GLIBC__
#include <sys/time.h>
#endif
#else /* !HAVE_POSIX */
#include <windows.h>
#endif /* !HAVE_POSIX */

unsigned
MonotonicClockMS()
{
#if defined(HAVE_POSIX) && !defined(__CYGWIN__)
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#elif defined(__APPLE__) /* OS X does not define CLOCK_MONOTONIC */
  static mach_timebase_info_data_t base;
  if (base.denom == 0)
    (void)mach_timebase_info(&base);

  return (unsigned)((mach_absolute_time() * base.numer)
                    / (1000000 * base.denom));
#else
  /* we have no monotonic clock, fall back to gettimeofday() */
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
#else /* !HAVE_POSIX */
  return ::GetTickCount();
#endif /* !HAVE_POSIX */
}

uint64_t
MonotonicClockUS()
{
#if defined(HAVE_POSIX) && !defined(__CYGWIN__)
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return uint64_t(ts.tv_sec) * 1000000 + uint64_t(ts.tv_nsec) / 1000;
#elif defined(__APPLE__) /* OS X does not define CLOCK_MONOTONIC */
  static mach_timebase_info_data_t base;
  if (base.denom == 0)
    (void)mach_timebase_info(&base);

  return (uint64_t(mach_absolute_time()) * uint64_t(base.numer))
    / (1000 * uint64_t(base.denom));
#else
  /* we have no monotonic clock, fall back to gettimeofday() */
  struct timeval tv;
  gettimeofday(&tv, 0);
  return uint64_t(tv.tv_sec) * 1000 + uint64_t(tv.tv_usec) / 1000;
#endif
#else /* !HAVE_POSIX */
  LARGE_INTEGER l_value, l_frequency;

  if (!::QueryPerformanceCounter(&l_value) ||
      !::QueryPerformanceFrequency(&l_frequency))
    return 0;

  uint64_t value = l_value.QuadPart;
  uint64_t frequency = l_frequency.QuadPart;

  if (frequency > 1000000) {
    value *= 10000;
    value /= frequency / 100;
  } else if (frequency < 1000000) {
    value *= 10000;
    value /= frequency;
    value *= 100;
  }

  return value;
#endif /* !HAVE_POSIX */
}

double
MonotonicClockFloat()
{
#if defined(HAVE_POSIX) && !defined(__CYGWIN__)
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + double(ts.tv_nsec) / 1000000000.;
#elif defined(__APPLE__) /* OS X does not define CLOCK_MONOTONIC */
  static mach_timebase_info_data_t base;
  if (base.denom == 0)
    (void)mach_timebase_info(&base);

  return mach_absolute_time() * double(base.numer) / double(base.denom);
#else
  /* we have no monotonic clock, fall back to gettimeofday() */
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + double(tv.tv_usec) / 1000000.;
#endif
#else /* !HAVE_POSIX */
  LARGE_INTEGER l_value, l_frequency;

  if (!::QueryPerformanceCounter(&l_value) ||
      !::QueryPerformanceFrequency(&l_frequency))
    return 0;

  uint64_t value = l_value.QuadPart;
  uint64_t frequency = l_frequency.QuadPart;

  return double(value) / double(frequency);
#endif /* !HAVE_POSIX */
}

int
GetSystemUTCOffset()
{
#ifdef HAVE_POSIX
  // XXX implement
  return 0;
#else
  TIME_ZONE_INFORMATION TimeZoneInformation;
  DWORD tzi = GetTimeZoneInformation(&TimeZoneInformation);

  int offset = -TimeZoneInformation.Bias * 60;
  if (tzi == TIME_ZONE_ID_STANDARD)
    offset -= TimeZoneInformation.StandardBias * 60;

  if (tzi == TIME_ZONE_ID_DAYLIGHT)
    offset -= TimeZoneInformation.DaylightBias * 60;

  return offset;
#endif
}
