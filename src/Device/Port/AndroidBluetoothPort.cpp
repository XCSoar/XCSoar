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

#include "AndroidBluetoothPort.hpp"
#include "Android/BluetoothHelper.hpp"
#include "OS/Sleep.h"

#include <assert.h>

AndroidBluetoothPort::AndroidBluetoothPort(const TCHAR *_address,
                                           Handler &_handler)
  :Port(_handler), address(_address), helper(NULL)
{
}

AndroidBluetoothPort::~AndroidBluetoothPort()
{
  Close();
}

bool
AndroidBluetoothPort::Open()
{
  helper = BluetoothHelper::connect(Java::GetEnv(), address);
  if (helper == NULL)
    return false;

  return true;
}

void
AndroidBluetoothPort::Flush(void)
{
  helper->flush(Java::GetEnv());
}

void
AndroidBluetoothPort::Run()
{
  assert(helper != NULL);

  SetRxTimeout(500);

  JNIEnv *const env = Java::GetEnv();

  while (!CheckStopped()) {
    int ch = helper->read(env);
    if (ch >= 0)
      ProcessChar(ch);
  }
}

bool
AndroidBluetoothPort::Close()
{
  if (helper == NULL)
    return true;

  StopRxThread();

  delete helper;
  helper = NULL;
  return true;
}

size_t
AndroidBluetoothPort::Write(const void *data, size_t length)
{
  if (helper == NULL)
    return 0;

  JNIEnv *env = Java::GetEnv();

  size_t nbytes = 0;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < length; ++i) {
    if (!helper->write(env, bytes[i]))
      break;
    ++nbytes;
  }

  return nbytes;
}

bool
AndroidBluetoothPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  if (helper == NULL)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
AndroidBluetoothPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (helper == NULL)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
AndroidBluetoothPort::SetRxTimeout(unsigned Timeout)
{
  if (helper == NULL)
    return false;

  helper->setReadTimeout(Java::GetEnv(), Timeout);
  return true;
}

unsigned
AndroidBluetoothPort::GetBaudrate() const
{
  // XXX implement K6-Bt commands?
  return 19200;
}

unsigned
AndroidBluetoothPort::SetBaudrate(unsigned BaudRate)
{
  // XXX implement K6-Bt commands?
  return BaudRate;
}

int
AndroidBluetoothPort::Read(void *Buffer, size_t Size)
{
  JNIEnv *env = Java::GetEnv();
  int ch = helper->read(env);
  if (ch < 0)
    return -1;

  *(uint8_t *)Buffer = ch;
  return 1;
}

void
AndroidBluetoothPort::ProcessChar(char c)
{
  Buffer::Range range = buffer.write();
  if (range.second == 0) {
    // overflow, so reset buffer
    buffer.clear();
    return;
  }

  if (c == '\n') {
    range.first[0] = _T('\0');
    buffer.append(1);

    range = buffer.read();
    handler.LineReceived(range.first);
    buffer.clear();
  } else if (c != '\r') {
    range.first[0] = c;
    buffer.append(1);
  }
}
