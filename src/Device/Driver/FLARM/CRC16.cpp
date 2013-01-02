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

#include "CRC16.hpp"

#include <assert.h>

/*
 * Source: FLARM_BINCOMM.pdf
 */
gcc_const
static uint16_t
crc_update(uint16_t crc, uint8_t data)
{
  crc = crc ^ ((uint16_t)data << 8);
  for (unsigned i = 0; i < 8; ++i) {
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
  }
  return crc;
}

uint16_t
FLARM::CalculateCRC(const FrameHeader &header,
                    const void *data, size_t length)
{
  assert((data != NULL && length > 0) || (data == NULL && length == 0));

  uint16_t crc = 0x00;

  const uint8_t *_header = (const uint8_t *)&header;
  for (unsigned i = 0; i < 6; ++i, ++_header)
    crc = crc_update(crc, *_header);

  if (length == 0 || data == NULL)
    return crc;

  const uint8_t *_data = (const uint8_t *)data;
  for (unsigned i = 0; i < length; ++i, ++_data)
    crc = crc_update(crc, *_data);

  return crc;
}
