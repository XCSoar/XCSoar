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

#ifndef XCSOAR_ELEMENT_STAT_COMPUTER_HPP
#define XCSOAR_ELEMENT_STAT_COMPUTER_HPP

#include "DistanceStatComputer.hpp"
#include "TaskVarioComputer.hpp"

struct ElementStat;

class ElementStatComputer
{
public:
  DistanceStatComputer remaining_effective;
  DistanceStatComputer remaining;
  DistanceStatComputer planned;
  DistanceStatComputer travelled;
  TaskVarioComputer vario;

private:
  bool initialised;

public:
  ElementStatComputer();

  /**
   * Calculate element speeds.  Incremental speeds are
   * held at bulk speeds within first minute of elapsed time.
   *
   * @param time monotonic time of day in seconds
   */
  void CalcSpeeds(ElementStat &data, double time);

  /**
   * Reset to uninitialised state, to supress calculation
   * of incremental speeds.
   */
  void Reset(ElementStat &data);
};

#endif
