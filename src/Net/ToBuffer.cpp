/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Net/ToBuffer.hpp"
#include "Net/Features.hpp"
#include "Net/Request.hpp"
#include "Operation/Operation.hpp"

#include <stdint.h>

int
Net::DownloadToBuffer(Session &session, const char *url,
                      void *_buffer, size_t max_length,
                      OperationEnvironment &env)
{
  Request request(session, url, 10000);
  if (!request.Send(10000))
    return -1;

  int64_t total = request.GetLength();
  if (total >= 0)
    env.SetProgressRange(total);
  total = 0;

  uint8_t *buffer = (uint8_t *)_buffer, *p = buffer, *end = buffer + max_length;
  while (p != end) {
    if (env.IsCancelled())
      return -1;

    ssize_t nbytes = request.Read(p, end - p, 5000);
    if (nbytes < 0)
      return -1;
    if (nbytes == 0)
      break;

    p += nbytes;

    total += nbytes;
    env.SetProgressPosition(total);
  }

  return p - buffer;
}

void
Net::DownloadToBufferJob::Run(OperationEnvironment &env)
{
  length = DownloadToBuffer(session, url, buffer, max_length, env);
}
