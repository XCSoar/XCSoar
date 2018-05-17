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

#include "Thread/Thread.hpp"
#include "Name.hpp"
#include "Util.hpp"

#ifdef ANDROID
#include "Java/Global.hxx"
#endif

#include <assert.h>

#ifdef HAVE_POSIX
#include <signal.h>
#endif

#ifndef NDEBUG
#include "FastMutex.hpp"
static FastMutex all_threads_mutex;

/**
 * This list keeps track of all active threads.  It is used to assert
 * that all threads have been cleaned up on shutdown.
 */
static boost::intrusive::list<Thread,
                              boost::intrusive::member_hook<Thread, Thread::SiblingsHook, &Thread::siblings>,
                              boost::intrusive::constant_time_size<false>> all_threads;
#endif

void
Thread::SetIdlePriority()
{
  ::SetThreadIdlePriority();
}

bool
Thread::Start()
{
  assert(!IsDefined());

#ifdef HAVE_POSIX
#ifndef NDEBUG
  creating = true;
#endif

#if defined(__GLIBC__) || defined(__BIONIC__) || defined(__APPLE__)
  defined = pthread_create(&handle, nullptr, ThreadProc, this) == 0;
#else
  /* In other libc implementations, the default stack size for created threads
     might not be large enough (e. g. 80 KB on musl libc).
     640 KB ought to be enough for anybody. */
  pthread_attr_t attr;
  defined = (pthread_attr_init(&attr) == 0) &&
            (pthread_attr_setstacksize(&attr, 640 * 1024) == 0) &&
            (pthread_create(&handle, &attr, ThreadProc, this) == 0);
#endif

#ifndef NDEBUG
  creating = false;
#endif

  bool success = defined;
#else
  handle = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &id);

  bool success = handle != nullptr;
#endif

#ifndef NDEBUG
  if (success) {
    all_threads_mutex.lock();
    all_threads.push_back(*this);
    all_threads_mutex.unlock();
  }
#endif

  return success;
}

void
Thread::Join()
{
  assert(IsDefined());
  assert(!IsInside());

#ifdef HAVE_POSIX
  pthread_join(handle, nullptr);
  defined = false;
#else
  ::WaitForSingleObject(handle, INFINITE);
  ::CloseHandle(handle);
  handle = nullptr;
#endif

#ifndef NDEBUG
  all_threads_mutex.lock();
  all_threads.erase(all_threads.iterator_to(*this));
  all_threads_mutex.unlock();
#endif
}

#ifndef HAVE_POSIX
bool
Thread::Join(unsigned timeout_ms)
{
  assert(IsDefined());
  assert(!IsInside());

  bool result = ::WaitForSingleObject(handle, timeout_ms) == WAIT_OBJECT_0;
  if (result) {
    ::CloseHandle(handle);
    handle = nullptr;

#ifndef NDEBUG
    {
      all_threads_mutex.lock();
      all_threads.erase(all_threads.iterator_to(*this));
      all_threads_mutex.unlock();
    }
#endif
  }

  return result;
}
#endif

#ifdef HAVE_POSIX

void *
Thread::ThreadProc(void *p)
{
  Thread *thread = (Thread *)p;

#ifndef NDEBUG
  /* this works around a race condition that causes an assertion
     failure due to IsInside() spuriously returning false right after
     the thread has been created, and the calling thread hasn't
     initialised "defined" yet */
  thread->defined = true;
#endif

  if (thread->name != nullptr)
    SetThreadName(thread->name);

  thread->Run();

#ifdef ANDROID
  Java::DetachCurrentThread();
#endif

  return nullptr;
}

#else /* !HAVE_POSIX */

DWORD WINAPI
Thread::ThreadProc(LPVOID lpParameter)
{
  Thread *thread = (Thread *)lpParameter;

  thread->Run();
  return 0;
}

#endif /* !HAVE_POSIX */

#ifndef NDEBUG

bool
ExistsAnyThread()
{
  all_threads_mutex.lock();
  bool result = !all_threads.empty();
  all_threads_mutex.unlock();
  return result;
}

#endif
