/* Copyright_License {

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
#ifndef XCSOAR_FLIGHT_PHASE_JSON_HPP
#define XCSOAR_FLIGHT_PHASE_JSON_HPP

#include "FlightPhaseDetector.hpp"

class BufferedOutputStream;

/**
 * Write JSON code for flight preformance statistics to the writer
 *
 * @param writer JSON writer instance
 * @param totals Flight statistics
 *
 * @see FlightPhaseDetector
 */
void
WritePerformanceStats(BufferedOutputStream &writer, const PhaseTotals &totals);

/**
 * Write JSON code for list of flight phases to the writer
 *
 * @param writer JSON writer instance
 * @param phases List of flight phases
 *
 * @see FlightPhaseDetector
 */
void
WritePhaseList(BufferedOutputStream &writer, const PhaseList &phases);

#endif
