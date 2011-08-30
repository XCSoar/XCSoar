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

#ifndef XCSOAR_IGC_PARSER_HPP
#define XCSOAR_IGC_PARSER_HPP

#include "Util/StaticString.hpp"
#include "Math/fixed.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "DateTime.hpp"

#include <tchar.h>

struct IGCHeader {
  /**
   * 3-letter manufacturer id.
   */
  char manufacturer[4];

  /**
   * 3-letter logger id.
   */
  char id[4];

  /**
   * The flight number on that day.
   */
  unsigned flight;
};

struct IGCFix {
  BrokenTime time;

  GeoPoint location;

  fixed gps_altitude, pressure_altitude;
};

/**
 * Parse an IGC "A" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseHeader(const char *line, IGCHeader &header);

/**
 * Parse an IGC "HFDTE" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseDate(const char *line, BrokenDate &date);

/**
 * Parse an IGC "B" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseFix(const char *buffer, IGCFix &fix);

#endif
