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

#include "Net/Request.hpp"
#include "Net/Connection.hpp"

#include <assert.h>

Net::Request::Request(Connection &connection, const char *file,
                      unsigned long timeout)
  :context(Context::REQUEST, this), last_error(0)
{
  opened_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  completed_event = CreateEvent(NULL, FALSE, FALSE, NULL);

  handle = HttpOpenRequestA(connection.handle, "GET", file, NULL, NULL, NULL,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
                            (DWORD_PTR)&context);

  if (handle == NULL && GetLastError() == ERROR_IO_PENDING)
    // Wait until we get the Request handle
    WaitForSingleObject(opened_event, timeout);
}

Net::Request::~Request()
{
  CloseHandle(opened_event);
  CloseHandle(completed_event);
  InternetCloseHandle(handle);
}

bool
Net::Request::Created() const
{
  return handle != NULL;
}

bool
Net::Request::Send(unsigned long timeout)
{
  assert(handle != NULL);

  // Send request
  if (HttpSendRequestA(handle, NULL, 0, NULL, 0))
    return true;

  // If HttpSendRequestA() failed or timeout occured in WaitForSingleObject()
  if (GetLastError() != ERROR_IO_PENDING ||
      WaitForSingleObject(completed_event, timeout) != WAIT_OBJECT_0)
    return false;
  else
    return last_error == 0;
}

size_t
Net::Request::Read(char *buffer, size_t buffer_size, unsigned long timeout)
{
  INTERNET_BUFFERSA InetBuff;
  FillMemory(&InetBuff, sizeof(InetBuff), 0);
  InetBuff.dwStructSize = sizeof(InetBuff);
  InetBuff.lpvBuffer = buffer;
  InetBuff.dwBufferLength = buffer_size - 1;

  // If InternetReadFileExA() failed or timeout occured in WaitForSingleObject()
  if (!InternetReadFileExA(handle, &InetBuff, IRF_ASYNC, (DWORD_PTR)&context) &&
      (GetLastError() != ERROR_IO_PENDING ||
       WaitForSingleObject(completed_event, timeout) != WAIT_OBJECT_0))
    // return "0 bytes read"
    return 0;

  buffer[InetBuff.dwBufferLength] = 0;
  return InetBuff.dwBufferLength;
}

void
Net::Request::Callback(DWORD status, LPVOID info, DWORD info_length)
{
  // Request handle
  switch (status) {
  case INTERNET_STATUS_HANDLE_CREATED: {
    INTERNET_ASYNC_RESULT *res = (INTERNET_ASYNC_RESULT *)info;
    handle = (HINTERNET)res->dwResult;
    SetEvent(opened_event);
    break;
  }
  case INTERNET_STATUS_REQUEST_COMPLETE: {
    INTERNET_ASYNC_RESULT *res = (INTERNET_ASYNC_RESULT *)info;
    last_error = res->dwError;
    SetEvent(completed_event);
    break;
  }
  }
}
