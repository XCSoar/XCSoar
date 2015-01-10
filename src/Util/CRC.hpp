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

#ifndef XCSOAR_CRC_HPP
#define XCSOAR_CRC_HPP

#include "Compiler.h"

#include <stdint.h>
#include <stddef.h>

extern const uint16_t crc16ccitt_table[256];

gcc_const
static inline uint16_t
UpdateCRC16CCITT(uint8_t octet, uint16_t crc)
{
  return (crc << 8) ^ crc16ccitt_table[(crc >> 8) ^ octet];
}

gcc_pure
static inline uint16_t
UpdateCRC16CCITT(const uint8_t *data, const uint8_t *end, uint16_t crc)
{
  while (data < end)
    crc = UpdateCRC16CCITT(*data++, crc);
  return crc;
}

gcc_pure
static inline uint16_t
UpdateCRC16CCITT(const void *data, size_t length, uint16_t crc)
{
  const uint8_t *p = (const uint8_t *)data, *end = p + length;
  return UpdateCRC16CCITT(p, end, crc);
}

#endif
