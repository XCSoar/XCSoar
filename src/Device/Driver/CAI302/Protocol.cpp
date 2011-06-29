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

#include "Protocol.hpp"
#include "Device/Port.hpp"

#include <algorithm>

int
CAI302::ReadShortReply(Port &port, void *buffer, unsigned max_size,
                       unsigned timeout_ms)
{
  unsigned char header[3];
  if (!port.FullRead(header, sizeof(header), timeout_ms))
    return -1;

  unsigned size = header[0];
  if (size < sizeof(header))
    return -1;

  size -= sizeof(header);
  if (size > max_size)
    size = max_size;

  if (!port.FullRead(buffer, size, timeout_ms))
    return -1;

  // XXX verify the checksum

  if (size < max_size) {
    /* fill the rest with zeroes */
    char *p = (char *)buffer;
    std::fill(p + size, p + max_size, 0);
  }

  return size;
}
