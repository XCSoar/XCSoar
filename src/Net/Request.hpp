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

#ifndef NET_REQUEST_HPP
#define NET_REQUEST_HPP

#include "Thread/Trigger.hpp"
#include "Net/Context.hpp"

#include <windows.h>
#include <wininet.h>

namespace Net {
  class Connection;

  class Request {
  public:
    /** Internal connection handle */
    HINTERNET handle;
    /** Context for the callback function (holds a pointer to this) */
    Context context;
    /** Event handles that are triggered in certain situations */
    Trigger opened_event, completed_event;
    /** The last error code that was retrieved by the Callback() function */
    DWORD last_error;

  public:
    /**
     * Creates a Request that can be used to get data from a webserver.
     * @param connection Connection instance that is used for creating this Request
     * @param file The file to request (e.g. /downloads/index.htm)
     * @param timeout Timeout used for creating this request
     */
    Request(Connection &connection, const char *file,
            unsigned long timeout = INFINITE);

    /** Destroys the Request and unregisters the event handles */
    ~Request();

    /**
     * Returns whether the Request has been created successfully.
     * Note that this has nothing to do with a physical connection yet!
     */
    bool Created() const;

    /**
     * Send the request to the server. If this function fails the server
     * can't be reached. If the file doesn't exists the webserver usually
     * returns a valid 404 page.
     * @param timeout Timeout used for sending the request
     * @return True if the connection was established successfully and
     * the request was sent
     */
    bool Send(unsigned long timeout = INFINITE);

    /**
     * Reads a number of bytes from the server.
     * This function must not be called before Send() !
     * @param timeout Timeout used for retrieving the data chunk
     * @return Number of bytes that were read from the server. 0 means EOF.
     */
    size_t Read(char *buffer, size_t buffer_size, unsigned long timeout = INFINITE);

    /** Internal callback function. Don't use this manually! */
    void Callback(DWORD status, LPVOID info, DWORD info_length);
  };
}

#endif
