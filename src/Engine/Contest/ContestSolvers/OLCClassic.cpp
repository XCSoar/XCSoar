/* Copyright_License {

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

#include "OLCClassic.hpp"

OLCClassic::OLCClassic(const Trace &_trace):
  ContestDijkstra(_trace, 6, 1000) {}

void 
OLCClassic::set_weightings()
{
  m_weightings[0] = 5;
  m_weightings[1] = 5;
  m_weightings[2] = 5;
  m_weightings[3] = 5;
  m_weightings[4] = 5; // was 4, pre 2010 rules
  m_weightings[5] = 5; // was 3, pre 2010 rules
}
