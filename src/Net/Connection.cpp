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

#include "Net/Connection.hpp"
#include "Net/Session.hpp"

Net::Connection::Connection(Session &session, const char *server,
                            unsigned long timeout)
  :context(Context::CONNECTION, this),
   event(false)
{
  handle = InternetConnectA(session.handle, server,
                            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                            INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)&context);

  if (handle == NULL && GetLastError() == ERROR_IO_PENDING)
    // Wait until we get the connection handle
    event.Wait(timeout);
}

Net::Connection::~Connection()
{
  InternetCloseHandle(handle);
}

bool
Net::Connection::Connected() const
{
  return handle != NULL;
}

void
Net::Connection::Callback(DWORD status, LPVOID info, DWORD info_length)
{
  if (status == INTERNET_STATUS_HANDLE_CREATED) {
    INTERNET_ASYNC_RESULT *res = (INTERNET_ASYNC_RESULT *)info;
    handle = (HINTERNET)res->dwResult;
    event.Signal();
  }
}
