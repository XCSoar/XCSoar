/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef NET_HTTP_RESPONSE_HANDLER_HPP
#define NET_HTTP_RESPONSE_HANDLER_HPP

#include "IO/DataHandler.hpp"

#include <stdint.h>

namespace Net {
  class ResponseHandler : public DataHandler {
  public:
    /**
     * Response metadata (status and headers) has been received.
     *
     * @param content_length the total length of the response body or -1 if
     * the server did not announce a length
     */
    virtual void ResponseReceived(int64_t content_length) = 0;
  };
}

#endif
