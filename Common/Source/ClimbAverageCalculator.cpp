/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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


#include "ClimbAverageCalculator.h"

ClimbAverageCalculator::ClimbAverageCalculator(void)
{
	newestValIndex = -1;

	for(int i=0; i<MAX_HISTORY; i++)
	{
		history[i].altitude = -99999;
		history[i].time = -99999;
	}
}

ClimbAverageCalculator::~ClimbAverageCalculator(void)
{
}


double ClimbAverageCalculator::GetAverage(double curTime, double curAltitude, int averageTime)
{
  double result = 0;
  int i;
  int bestHistory;
  int oldestValIndex = 0;


  newestValIndex = newestValIndex < MAX_HISTORY-1 ? newestValIndex+1 : 0;
  oldestValIndex = bestHistory =  newestValIndex < MAX_HISTORY-1 ? newestValIndex+1 : 0;

  // add the new sample
  history[newestValIndex].time = curTime;
  history[newestValIndex].altitude = curAltitude;

  // initialy bestHistory is the current...
  bestHistory = newestValIndex;

  // now run through the history and find the best sample for average period within the average time period
  for(i=0; i<MAX_HISTORY; i++)
    {
      if (history[i].time != -99999)
	{
	  if (history[i].time + averageTime >= curTime) // inside the period ?
	    {
	      if (history[i].time < history[bestHistory].time) // is the sample older (and therefore better) than the current found ?
		{
		  bestHistory = i;
		}
	    }
	}
    }

  // calculate the average !
  if (bestHistory != newestValIndex)
    {
      result = (curAltitude - history[bestHistory].altitude) / (curTime - history[bestHistory].time);
    }

  return result;
}
