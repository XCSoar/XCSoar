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

#include "Net/Session.hpp"
#include "Net/Context.hpp"
#include "Net/Connection.hpp"
#include "Net/Request.hpp"

static void __stdcall
Callback(HINTERNET hInternet, DWORD dwContext, DWORD dwInternetStatus,
         LPVOID lpStatusInfo, DWORD dwStatusInfoLen)
{
  Net::Context *context = (Net::Context *)dwContext;

  if (context->parent == NULL)
    return;

  if (context->type == Net::Context::CONNECTION)
    ((Net::Connection *)context->parent)->
    Callback(dwInternetStatus, lpStatusInfo, dwStatusInfoLen);
  else if (context->type == Net::Context::REQUEST)
    ((Net::Request *)context->parent)->
    Callback(dwInternetStatus, lpStatusInfo, dwStatusInfoLen);
}

Net::Session::Session()
{
  // Get session handle
  handle.Set(::InternetOpenA("XCSoar", INTERNET_OPEN_TYPE_PRECONFIG, NULL,
                             NULL, INTERNET_FLAG_ASYNC));

  // If handle was retrieved
  if (handle.IsDefined()) {
    // Setup callback function
    INTERNET_STATUS_CALLBACK callback_result =
        handle.SetStatusCallback((INTERNET_STATUS_CALLBACK)&Callback);

    // Save whether callback function was setup correctly
    callback_installed = (callback_result != INTERNET_INVALID_STATUS_CALLBACK);
  }
}

Net::Session::~Session()
{
  if (handle.IsDefined()) {
    // Unregister callback function
    if (callback_installed)
      handle.SetStatusCallback(NULL);
  }
}

bool
Net::Session::Error() const
{
  // Error occured if either no handle was retrieved in the constructor
  // or if the callback wasn't setup correctly
  return !handle.IsDefined() || callback_installed == false;
}
