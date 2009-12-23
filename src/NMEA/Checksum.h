/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <tchar.h>

/**
 * Calculates the checksum for the specified line (without the
 * asterisk and the newline character).
 *
 * @param p a NULL terminated string
 */
static inline unsigned char
NMEAChecksum(const TCHAR *p)
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
NMEAChecksum(const TCHAR *p, unsigned length)
{
  unsigned char checksum = 0;

  for (unsigned i = 0; i < length; ++i)
    checksum ^= *p++;

  return checksum;
}

#endif
