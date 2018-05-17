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
#ifndef XCSOAR_UTILS_FLARM_HPP
#define XCSOAR_UTILS_FLARM_HPP

#include <tchar.h>

class FlarmId;
struct FlarmNetRecord;

namespace FlarmDetails
{
  /**
   * Looks up the FLARM id in the FLARMNet Database
   * and returns the FLARMNet Record
   * @param id FLARM id
   * @return The corresponding FLARMNet Record if found, otherwise NULL
   */
  const FlarmNetRecord *
  LookupRecord(FlarmId id);

  /**
   * Looks up the FLARM id in the FLARM details array
   * and the FLARMnet file and returns the callsign
   * @param id FLARM id
   * @return The corresponding callsign if found, otherwise NULL
   */
  const TCHAR *
  LookupCallsign(FlarmId id);

  /**
   * Looks up the callsign in the FLARM details array
   * and the FLARMnet file and returns the FLARM id
   * @param cn Callsign
   * @return The corresponding FLARM id if found, otherwise 0
   */
  FlarmId
  LookupId(const TCHAR *cn);

  /**
   * Adds a FLARM details couple (callsign + FLARM id)
   * to the FLARM details array
   * @param id FLARM id
   * @param name Callsign
   * @return True if successfully added, False otherwise
   */
  bool
  AddSecondaryItem(FlarmId id, const TCHAR *name);

  unsigned
  FindIdsByCallSign(const TCHAR *cn, FlarmId array[], unsigned size);
}

#endif
