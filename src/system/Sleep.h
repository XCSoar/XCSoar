// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef XCSOAR_OS_SLEEP_H

#ifdef _WIN32

#include <synchapi.h>

#else /* !_WIN32 */

#include <time.h>

static inline void
Sleep(unsigned ms) noexcept
{
  const struct timespec ts = {
    static_cast<time_t>(ms / 1000),
    static_cast<long>((ms % 1000L) * 1000000L),
  };

  nanosleep(&ts, nullptr);
}

#endif /* !_WIN32 */

#endif
