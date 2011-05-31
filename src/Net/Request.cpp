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
#include "Net/Session.hpp"
#include "Net/Connection.hpp"

#include <assert.h>

Net::Request::Request(Session &session, const TCHAR *url,
                      unsigned long timeout)
  :context(Context::REQUEST, this),
   opened_event(false), completed_event(false),
   last_error(ERROR_SUCCESS)
{
  HINTERNET h = session.handle.OpenUrl(url, NULL, 0,
                                       INTERNET_FLAG_NO_AUTH |
                                       INTERNET_FLAG_NO_AUTO_REDIRECT |
                                       INTERNET_FLAG_NO_CACHE_WRITE |
                                       INTERNET_FLAG_NO_COOKIES |
                                       INTERNET_FLAG_NO_UI,
                                       (DWORD_PTR)&context);

  if (h == NULL && GetLastError() == ERROR_IO_PENDING)
    // Wait until we get the Request handle
    completed_event.Wait(timeout);
}

Net::Request::Request(Connection &connection, const char *file,
                      unsigned long timeout)
  :context(Context::REQUEST, this),
   opened_event(false), completed_event(false),
   last_error(ERROR_SUCCESS)
{
  HINTERNET h = connection.handle.OpenRequest("GET", file, NULL, NULL, NULL,
                                              INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
                                              (DWORD_PTR)&context);

  if (h == NULL && GetLastError() == ERROR_IO_PENDING)
    // Wait until we get the Request handle
    opened_event.Wait(timeout);
}

bool
Net::Request::Created() const
{
  return handle.IsDefined();
}

bool
Net::Request::Send(unsigned long timeout)
{
  assert(handle.IsDefined());

  // Send request
  if (handle.SendRequest(NULL, 0, NULL, 0))
    return true;

  // If HttpSendRequestA() failed or timeout occured in WaitForSingleObject()
  if (GetLastError() != ERROR_IO_PENDING ||
      !completed_event.Wait(timeout))
    return false;
  else
    return last_error == ERROR_SUCCESS;
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
  if (!handle.Read(&InetBuff, IRF_ASYNC, (DWORD_PTR)&context) &&
      (GetLastError() != ERROR_IO_PENDING ||
       !completed_event.Wait(timeout)))
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
    handle.Set((HINTERNET)res->dwResult);
    opened_event.Signal();
    break;
  }
  case INTERNET_STATUS_REQUEST_COMPLETE: {
    INTERNET_ASYNC_RESULT *res = (INTERNET_ASYNC_RESULT *)info;
    last_error = res->dwError;
    completed_event.Signal();
    break;
  }
  }
}
