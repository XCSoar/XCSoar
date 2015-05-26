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

#include "../Session.hpp"
#include "../Request.hpp"

static void CALLBACK
StatusCallback(HINTERNET hInternet,
               DWORD_PTR dwContext,
               DWORD dwInternetStatus,
               LPVOID lpvStatusInformation,
               DWORD dwStatusInformationLength)
{
  if (dwContext != ((DWORD_PTR) 0)) {
    Net::Request *request = (Net::Request *)dwContext;

    request->Callback(dwInternetStatus,
                      lpvStatusInformation, dwStatusInformationLength);
  }
}

Net::Session::Session()
{
  // Get session handle
  handle.Set(::InternetOpenA("XCSoar", INTERNET_OPEN_TYPE_PRECONFIG, NULL,
                             NULL, INTERNET_FLAG_ASYNC));
  if (handle.IsDefined())
    handle.SetStatusCallback(StatusCallback);
}

bool
Net::Session::Error() const
{
  // Error occured if either no handle was retrieved in the constructor
  // or if the callback wasn't setup correctly
  return !handle.IsDefined();
}
