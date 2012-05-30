/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_FLARM_RECORD_HPP
#define XCSOAR_FLARM_RECORD_HPP

#include "Util/StaticString.hpp"
#include "Compiler.h"

#include <tchar.h>

class FlarmId;

/**
 * FlarmNet.org file entry
 */
struct FlarmRecord {
  /**< FLARM id 6 bytes */
  StaticString<7> id;

  /**< Name 15 bytes */
  StaticString<22> pilot;

  /**< Airfield 4 bytes */
  StaticString<22> airfield;

  /**< Aircraft type 1 byte */
  StaticString<22> plane_type;

  /**< Registration 7 bytes */
  StaticString<8> registration;

  /**< Callsign 3 bytes */
  StaticString<4> callsign;

  /**< Radio frequency 6 bytes */
  StaticString<8> frequency;

  gcc_pure
  FlarmId GetId() const;
};

#endif
