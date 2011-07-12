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

#include "FLARM/FlarmCalculations.hpp"
#include "FLARM/State.hpp"

FlarmCalculations::~FlarmCalculations()
{
  for (AverageCalculatorMap::iterator i = averageCalculatorMap.begin();
       i != averageCalculatorMap.end(); ++i)
    delete i->second;
}

fixed
FlarmCalculations::Average30s(FlarmId flarmId, fixed curTime, fixed curAltitude)
{
  ClimbAverageCalculator *itemTemp = NULL;
  AverageCalculatorMap::iterator iterFind = averageCalculatorMap.find(flarmId);

  if (iterFind != averageCalculatorMap.end()) {
    itemTemp = (ClimbAverageCalculator*)iterFind->second;
  } else {
    itemTemp = new ClimbAverageCalculator();
    averageCalculatorMap[flarmId] = itemTemp;
  }

  return itemTemp->GetAverage(curTime, curAltitude, fixed(30));
}
