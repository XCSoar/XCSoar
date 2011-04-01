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

#ifndef XCSOAR_NMEA_CHECKSUM_HPP
#define XCSOAR_NMEA_CHECKSUM_HPP

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a NULL terminated string
 */
static inline unsigned char
NMEAChecksum(const char *p)
{
  unsigned char checksum = 0;

  while (*p != 0)
    checksum ^= *p++;

  return checksum;
}

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a string
 * @param length the number of characters in the string
 */
static inline unsigned char
NMEAChecksum(const char *p, unsigned length)
{
  unsigned char checksum = 0;

  for (unsigned i = 0; i < length; ++i)
    checksum ^= *p++;

  return checksum;
}

#endif
