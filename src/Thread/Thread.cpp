/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifdef ANDROID
#include "Java/Global.hpp"
#endif

#include <assert.h>

#ifdef HAVE_POSIX
#include <signal.h>
#endif

#include "LogFile.hpp"

Thread::~Thread()
{
#ifndef HAVE_POSIX
  if (handle != NULL)
    ::CloseHandle(handle);
#endif
}

bool
Thread::start()
{
#ifdef HAVE_POSIX
  assert(!m_defined);

  return m_defined = pthread_create(&handle, NULL, thread_proc, this) == 0;
#else
  assert(handle == NULL);

  handle = ::CreateThread(NULL, 0, thread_proc, this, 0, NULL);

  return handle != NULL;
#endif
}

void
Thread::join()
{
#ifdef HAVE_POSIX
  assert(m_defined);

  pthread_join(handle, NULL);
  m_defined = false;
#else
  assert(handle != NULL);

  ::WaitForSingleObject(handle, INFINITE);
  ::CloseHandle(handle);
  handle = NULL;
#endif
}

bool
Thread::join(unsigned timeout_ms)
{
#ifdef HAVE_POSIX
  // XXX timeout not implemented with pthread
  join();
  return true;
#else
  assert(handle != NULL);

  bool result = ::WaitForSingleObject(handle, timeout_ms) == WAIT_OBJECT_0;
  if (result) {
    ::CloseHandle(handle);
    handle = NULL;
  }

  return result;
#endif
}

#ifdef HAVE_POSIX

void *
Thread::thread_proc(void *p)
{
  Thread *thread = (Thread *)p;

  thread->run();

#ifdef ANDROID
  Java::DetachCurrentThread();
#endif

  return NULL;
}

#else /* !HAVE_POSIX */

DWORD WINAPI
Thread::thread_proc(LPVOID lpParameter)
{
  Thread *thread = (Thread *)lpParameter;

  thread->run();
  return 0;
}

#endif /* !HAVE_POSIX */
