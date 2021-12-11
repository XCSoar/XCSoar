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

#include "thread/Thread.hpp"
#include "Name.hpp"
#include "Util.hpp"
#include "system/Error.hxx"

#ifdef ANDROID
#include "java/Global.hxx"
#endif

#include <cassert>

void
Thread::SetIdlePriority() noexcept
{
  ::SetThreadIdlePriority();
}

void
Thread::Start()
{
  assert(!IsDefined());

#ifdef HAVE_POSIX
#ifndef NDEBUG
  creating = true;
#endif

#if defined(__GLIBC__) || defined(__BIONIC__) || defined(__APPLE__)
  constexpr pthread_attr_t *attr_pointer = nullptr;

  int error;
#else
  /* In other libc implementations, the default stack size for created threads
     might not be large enough (e. g. 80 KB on musl libc).
     640 KB ought to be enough for anybody. */
  pthread_attr_t attr;
  int error = pthread_attr_init(&attr);
  if (error != 0)
    throw MakeErrno(error, "pthread_attr_init() failed");

  error = pthread_attr_setstacksize(&attr, 640 * 1024);
  if (error != 0)
    throw MakeErrno(error, "pthread_attr_setstacksize() failed");

  pthread_attr_t *attr_pointer = &attr;
#endif

  error = pthread_create(&handle, attr_pointer, ThreadProc, this);
  if (error != 0)
    throw MakeErrno(error, "pthread_create() failed");

  defined = true;

#ifndef NDEBUG
  creating = false;
#endif
#else
  handle = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &id);
  if (handle == nullptr)
    throw MakeLastError("CreateThread() failed");
#endif
}

void
Thread::Join() noexcept
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
}

#ifndef HAVE_POSIX
bool
Thread::Join(unsigned timeout_ms) noexcept
{
  assert(IsDefined());
  assert(!IsInside());

  bool result = ::WaitForSingleObject(handle, timeout_ms) == WAIT_OBJECT_0;
  if (result) {
    ::CloseHandle(handle);
    handle = nullptr;
  }

  return result;
}
#endif

#ifdef HAVE_POSIX

void *
Thread::ThreadProc(void *p) noexcept
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
Thread::ThreadProc(LPVOID lpParameter) noexcept
{
  Thread *thread = (Thread *)lpParameter;

  thread->Run();
  return 0;
}

#endif /* !HAVE_POSIX */
