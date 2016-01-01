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

#include "XContestFree.hpp"

XContestFree::XContestFree(const Trace &_trace,
                           const bool _is_dhv)
  :ContestDijkstra(_trace, true, 4, 1000),
   is_dhv(_is_dhv) {}

ContestResult
XContestFree::CalculateResult() const
{
  ContestResult result = ContestDijkstra::CalculateResult();
  // DHV-XC: 1.5 points per km
  // XContest: 1.0 points per km
  const auto score_factor = is_dhv ? 0.0015 : 0.0010;
  result.score = ApplyHandicap(result.distance * score_factor);
  return result;
}
