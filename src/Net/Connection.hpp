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

#ifndef NET_CONNECTION_HPP
#define NET_CONNECTION_HPP

#include "Net/Context.hpp"

#include <windows.h>
#include <wininet.h>

namespace Net {
  class Session;

  class Connection {
    /** Internal connection handle */
    HINTERNET handle;
    /** Context for the callback function (holds a pointer to this) */
    Context context;
    /**
     * Event handle that is triggered when the Connection
     * is established asynchronously
     */
    HANDLE event;

  public:
    friend class Request;

    /**
     * Opens a connection that can be used to create requests.
     * @param session A Session instance used for creating this Connection
     * @param server The server to connect to (e.g. www.xcsoar.org)
     * @param timeout Timeout used for creating the connection handle
     */
    Connection(Session &session, const char *server, unsigned long timeout = INFINITE);

    /** Destroys the Connection and unregisters the event handle */
    ~Connection();

    /**
     * Returns whether the Connection has been created successfully.
     * Note that this has nothing to do with a physical connection yet!
     */
    bool Connected() const;

    /** Internal callback function. Don't use this manually! */
    void Callback(DWORD status, LPVOID info, DWORD info_length);
  };
}

#endif
