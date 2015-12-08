/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "../Request.hpp"
#include "../Session.hpp"
#include "Util/ConvertString.hpp"

#include <stdexcept>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

Net::Request::Request(Session &session, const char *url,
                      unsigned timeout_ms)
{
  UTF8ToWideConverter url2(url);
  if (!url2.IsValid())
    return;

  HINTERNET h = session.handle.OpenUrl(url2, NULL, 0,
                                       INTERNET_FLAG_NO_AUTH |
                                       INTERNET_FLAG_NO_AUTO_REDIRECT |
                                       INTERNET_FLAG_NO_CACHE_WRITE |
                                       INTERNET_FLAG_NO_COOKIES |
                                       INTERNET_FLAG_NO_UI,
                                       (DWORD_PTR)this);

  if (h == NULL && GetLastError() == ERROR_IO_PENDING)
    // Wait until we get the Request handle
    opened_event.Wait(timeout_ms);
}

void
Net::Request::AddHeader(const char *name, const char *value)
{
  if (!handle.IsDefined())
    return;

  char buffer[4096];
  unsigned length = snprintf(buffer, sizeof(buffer), "%s: %s\r\n",
                             name, value);
  if (length < sizeof(buffer))
    handle.AddRequestHeaders(buffer, length);
}

void
Net::Request::SetBasicAuth(const char *username, const char *password)
{
  if (handle.IsDefined())
    handle.SetBasicAuth(username, password);
}

void
Net::Request::Send(unsigned timeout_ms)
{
  if (!handle.IsDefined())
    throw std::runtime_error("No INTERNET_STATUS_HANDLE_CREATED received");

  if (!completed_event.Wait(timeout_ms))
    /* response timeout */
    throw std::runtime_error("HTTP timeout");

  if (last_error != ERROR_SUCCESS)
    /* I/O error */
    throw std::runtime_error("HTTP I/O error");

  unsigned status = handle.GetStatusCode();
  if (status < 200 || status >= 300)
    /* unsuccessful HTTP status */
    throw std::runtime_error("Unsuccessful HTTP status");
}

int64_t
Net::Request::GetLength() const
{
  assert(handle.IsDefined());

  return handle.GetContentLength();
}

size_t
Net::Request::Read(void *buffer, size_t buffer_size, unsigned timeout_ms)
{
  INTERNET_BUFFERSA InetBuff;
  FillMemory(&InetBuff, sizeof(InetBuff), 0);
  InetBuff.dwStructSize = sizeof(InetBuff);
  InetBuff.lpvBuffer = buffer;
  InetBuff.dwBufferLength = buffer_size - 1;

  completed_event.Reset();
  if (!handle.Read(&InetBuff, IRF_ASYNC, (DWORD_PTR)this)) {
    if (GetLastError() == ERROR_IO_PENDING) {
      if (!completed_event.Wait(timeout_ms))
        throw std::runtime_error("HTTP timeout");
    } else {
      /* error */
      throw std::runtime_error("HTTP I/O error");
    }
  }

  ((uint8_t *)buffer)[InetBuff.dwBufferLength] = 0;
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
