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

#ifndef XCSOAR_IGC_DECLARATION_HPP
#define XCSOAR_IGC_DECLARATION_HPP

#include "Time/BrokenDateTime.hpp"
#include "Util/StaticString.hxx"
#include "Geo/GeoPoint.hpp"

struct IGCDeclarationHeader {
  /** Date and time of the declaration */
  BrokenDateTime datetime;

  /** Date of the flight */
  BrokenDate flight_date;

  /** Task number on the day */
  char task_id[4];

  /** Number of task turnpoints, excluding start and finish */
  unsigned num_turnpoints;

  /** Optional name of the task */
  NarrowString<256> task_name;
};

struct IGCDeclarationTurnpoint {
  /** Location of the turnpoint */
  GeoPoint location;

  /** Optional name of the turnpoint */
  NarrowString<256> name;
};

#endif
