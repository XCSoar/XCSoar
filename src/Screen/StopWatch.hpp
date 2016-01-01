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

#ifndef XCSOAR_SCREEN_STOP_WATCH_HPP
#define XCSOAR_SCREEN_STOP_WATCH_HPP

#ifdef STOP_WATCH

#include "Util/StaticArray.hxx"
#include "LogFile.hpp"

#ifdef HAVE_POSIX
#include <time.h>
#include <stdint.h>
#else /* !HAVE_POSIX */
#include <windows.h>
#endif /* !HAVE_POSIX */

#endif /* STOP_WATCH */

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/System.hpp"
#endif

/**
 * A stop watch which measures the time needed to perform an
 * operation, and writes it to the log file.  It is a no-op if the
 * macro STOP_WATCH is not defined.
 */
class ScreenStopWatch {
#ifdef STOP_WATCH
  typedef uint64_t clock_stamp_t;
  typedef uint64_t cpu_stamp_t;

  struct Marker {
    const char *text;
    clock_stamp_t clock;
    cpu_stamp_t cpu;

    void Set(const char *_text) {
      text = _text;
      clock = GetCurrentClock();
      cpu = GetCurrentCPU();
    }
  };

  typedef StaticArray<Marker, 256u> MarkerList;
  MarkerList markers;

private:
  static void FlushScreen() {
#ifdef ENABLE_OPENGL
    glFinish();
#endif
  }

  static clock_stamp_t GetCurrentClock() {
#ifdef HAVE_POSIX
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
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

  static cpu_stamp_t GetCurrentCPU() {
#ifdef HAVE_POSIX
    // XXX
    return 0;
#else /* !HAVE_POSIX */
    FILETIME f_kernel_time, f_user_time;

    if (!::GetThreadTimes(::GetCurrentThread(), nullptr, nullptr,
                          &f_kernel_time, &f_user_time))
      return 0;

    uint64_t kernel_time = f_kernel_time.dwLowDateTime / 10
      + (uint64_t)f_kernel_time.dwHighDateTime * 100000;
    uint64_t user_time = f_user_time.dwLowDateTime / 10
      + (uint64_t)f_user_time.dwHighDateTime * 100000;

    return kernel_time + user_time;
#endif /* !HAVE_POSIX */
  }

public:
  void Mark(const char *text) {
    FlushScreen();
    markers.append().Set(text);
  }

  void Finish() {
    if (markers.empty())
      return;

    FlushScreen();
    markers.append().Set(nullptr);

    for (unsigned i = 0; markers[i + 1].text != nullptr; ++i) {
      const Marker &start = markers[i];
      const Marker &end = markers[i + 1];

      LogFormat("StopWatch '%s': clock=%lu cpu=%lu", start.text,
                (unsigned long)(end.clock - start.clock),
                (unsigned long)(end.cpu - start.cpu));
    }

    const Marker &start = markers.front();
    const Marker &end = markers.back();
    LogFormat("StopWatch total: clock=%lu cpu=%lu",
              (unsigned long)(end.clock - start.clock),
              (unsigned long)(end.cpu - start.cpu));

    markers.clear();
  }

#else /* !STOP_WATCH */
public:
  void Mark(const char *text) {}
  void Finish() {}
#endif /* !STOP_WATCH */
};

#endif
