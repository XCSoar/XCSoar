/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "AndroidPort.hpp"
#include "Android/PortBridge.hpp"
#include "OS/Sleep.h"

#include <assert.h>

AndroidPort::AndroidPort(Handler &_handler)
  :Port(_handler), bridge(NULL)
{
}

AndroidPort::~AndroidPort()
{
  Close();
}

void
AndroidPort::SetBridge(PortBridge *_bridge)
{
  assert(bridge == NULL);
  assert(_bridge != NULL);

  bridge = _bridge;
}

void
AndroidPort::Flush()
{
  bridge->flush(Java::GetEnv());
}

void
AndroidPort::Run()
{
  assert(bridge != NULL);

  SetRxTimeout(500);

  JNIEnv *const env = Java::GetEnv();

  while (!CheckStopped()) {
    int ch = bridge->read(env);
    if (ch >= 0) {
      char ch2 = ch;
      handler.DataReceived(&ch2, sizeof(ch2));
    }
  }
}

bool
AndroidPort::Close()
{
  if (bridge == NULL)
    return true;

  StopRxThread();

  delete bridge;
  bridge = NULL;
  return true;
}

size_t
AndroidPort::Write(const void *data, size_t length)
{
  if (bridge == NULL)
    return 0;

  JNIEnv *env = Java::GetEnv();

  size_t nbytes = 0;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < length; ++i) {
    if (!bridge->write(env, bytes[i]))
      break;
    ++nbytes;
  }

  return nbytes;
}

bool
AndroidPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  if (bridge == NULL)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
AndroidPort::StartRxThread()
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (bridge == NULL)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
AndroidPort::SetRxTimeout(unsigned Timeout)
{
  if (bridge == NULL)
    return false;

  bridge->setReadTimeout(Java::GetEnv(), Timeout);
  return true;
}

int
AndroidPort::Read(void *Buffer, size_t Size)
{
  JNIEnv *env = Java::GetEnv();
  int ch = bridge->read(env);
  if (ch < 0)
    return -1;

  *(uint8_t *)Buffer = ch;
  return 1;
}

Port::WaitResult
AndroidPort::WaitRead(unsigned timeout_ms)
{
  return (Port::WaitResult)bridge->waitRead(Java::GetEnv(), timeout_ms);
}
