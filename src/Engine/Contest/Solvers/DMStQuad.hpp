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

#ifndef XCSOAR_CONTEST_DMST_QUAD_HPP
#define XCSOAR_CONTEST_DMST_QUAD_HPP

#include "ContestDijkstra.hpp"

/**
 * A quadrilateral task for the DMSt (Deutsche Meisterschaft im
 * Streckensegelflug).
 *
 * @see http://www.daec.de/fileadmin/user_upload/files/2012/sportarten/segelflug/sport/dmst/DMSt-WO_2012.pdf
 */
class DMStQuad : public ContestDijkstra {
public:
  DMStQuad(const Trace &_trace);
};

#endif
