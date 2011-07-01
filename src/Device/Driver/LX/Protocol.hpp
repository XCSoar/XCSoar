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

#ifndef XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP

#include <stddef.h>

#define LX_ACK_STRING "\x06"

static const unsigned int NUMTPS = 12;

enum LX_command {
  LX_PREFIX = 0x02,
  LX_ACK = 0x06,
  LX_SYN = 0x16,
  LX_WRITE_FLIGHT_INFO = 0xCA,
  LX_WRITE_CONTEST_CLASS = 0xD0,
};

char
calc_crc_char(char d, char crc);

char
filser_calc_crc(const char *p0, size_t len, char crc);

#endif
