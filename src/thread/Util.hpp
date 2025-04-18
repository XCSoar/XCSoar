// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#if defined(__linux__) && !defined(ANDROID)
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <processthreadsapi.h>
#endif

#if defined(__linux__) && !defined(ANDROID)

static int
ioprio_set(int which, int who, int ioprio)
{
  return syscall(__NR_ioprio_set, which, who, ioprio);
}

static void
ioprio_set_idle()
{
  static constexpr int _IOPRIO_WHO_PROCESS = 1;
  static constexpr int _IOPRIO_CLASS_IDLE = 3;
  static constexpr int _IOPRIO_CLASS_SHIFT = 13;
  static constexpr int _IOPRIO_IDLE =
    (_IOPRIO_CLASS_IDLE << _IOPRIO_CLASS_SHIFT) | 7;

  ioprio_set(_IOPRIO_WHO_PROCESS, 0, _IOPRIO_IDLE);
}

#endif

/**
 * Lower the current thread's priority to "idle" (very low).
 */
static inline void
SetThreadIdlePriority()
{
#ifdef __linux__

#ifdef SCHED_IDLE
  static struct sched_param sched_param;
  sched_setscheduler(0, SCHED_IDLE, &sched_param);
#endif

#ifndef ANDROID
  /* this system call is forbidden via seccomp on Android 8 and leads
   * to crash (SIGSYS) */
  ioprio_set_idle();
#endif

#elif defined(_WIN32)
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
#endif
};

/**
 * Raise the current thread's priority to "real-time" (very high).
 */
static inline void
SetThreadRealtime()
{
#ifdef __linux__
  struct sched_param sched_param;
  sched_param.sched_priority = 50;

  int policy = SCHED_FIFO;
#ifdef SCHED_RESET_ON_FORK
  policy |= SCHED_RESET_ON_FORK;
#endif

  sched_setscheduler(0, policy, &sched_param);
#endif
};
