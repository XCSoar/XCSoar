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

#include "Checksum.hpp"

IMI::IMIWORD IMI::CRC16Checksum(const void *message, unsigned bytes)
{
  const IMIBYTE *pData = (const IMIBYTE *)message;

  IMIWORD crc = 0xFFFF;
  for(;bytes; bytes--) {
    crc  = (IMIBYTE)(crc >> 8) | (crc << 8);
    crc ^= *pData++;
    crc ^= (IMIBYTE)(crc & 0xff) >> 4;
    crc ^= (crc << 8) << 4;
    crc ^= ((crc & 0xff) << 4) << 1;
  }

  if (crc == 0xFFFF)
    crc = 0xAAAA;

  return crc;
}

IMI::IMIBYTE IMI::FixChecksum(const void *message, unsigned bytes)
{
  const IMIBYTE *p = (const IMIBYTE*)message;

  IMIBYTE checksum = 0;
  for (;bytes; bytes--)
    checksum ^= *p++;

  if (checksum == 0xFF) checksum = 0xAA;

  return checksum;
}
