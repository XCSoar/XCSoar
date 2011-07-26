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
Thread::Start()
{
  assert(!IsDefined());

#ifdef HAVE_POSIX
  return defined = pthread_create(&handle, NULL, ThreadProc, this) == 0;
#else
  handle = ::CreateThread(NULL, 0, ThreadProc, this, 0, &id);

  return handle != NULL;
#endif
}

void
Thread::Join()
{
  assert(IsDefined());
  assert(!IsInside());

#ifdef HAVE_POSIX
  pthread_join(handle, NULL);
  defined = false;
#else
  ::WaitForSingleObject(handle, INFINITE);
  ::CloseHandle(handle);
  handle = NULL;
#endif
}

bool
Thread::Join(unsigned timeout_ms)
{
  assert(IsDefined());
  assert(!IsInside());

#ifdef HAVE_POSIX
  // XXX timeout not implemented with pthread
  Join();
  return true;
#else
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
Thread::ThreadProc(void *p)
{
  Thread *thread = (Thread *)p;

  thread->defined = true;
  thread->Run();

#ifdef ANDROID
  Java::DetachCurrentThread();
#endif

  return NULL;
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
