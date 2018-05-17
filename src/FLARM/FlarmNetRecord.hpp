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

#ifndef XCSOAR_FLARM_NET_RECORD_HPP
#define XCSOAR_FLARM_NET_RECORD_HPP

#include "Util/StaticString.hxx"
#include "Compiler.h"

class FlarmId;

constexpr
static inline size_t
LatinBufferSize(size_t size)
{
#ifdef _UNICODE
/* with wide characters, the exact size of the FLARMNet database field
   (plus one for the terminator) is just right, ... */
  return size;
#else
/* ..., but when we convert Latin-1 to UTF-8, we need a little bit
   more buffer */
  return size * 3 / 2 + 1;
#endif
}

/**
 * FlarmNet.org file entry
 */
struct FlarmNetRecord {
  /**< FLARM id 6 bytes */
  StaticString<LatinBufferSize(7)> id;

  /**< Name 15 bytes */
  StaticString<LatinBufferSize(22)> pilot;

  /**< Airfield 4 bytes */
  StaticString<LatinBufferSize(22)> airfield;

  /**< Aircraft type 1 byte */
  StaticString<LatinBufferSize(22)> plane_type;

  /**< Registration 7 bytes */
  StaticString<LatinBufferSize(8)> registration;

  /**< Callsign 3 bytes */
  StaticString<LatinBufferSize(4)> callsign;

  /**< Radio frequency 6 bytes */
  StaticString<LatinBufferSize(8)> frequency;

  gcc_pure
  FlarmId GetId() const;
};

#endif
