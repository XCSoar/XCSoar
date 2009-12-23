/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifdef FLARM_AVERAGE
#include "FlarmCalculations.h"
#include "NMEA/Info.h"

FlarmCalculations::FlarmCalculations(void)
{
}

FlarmCalculations::~FlarmCalculations(void)
{
  // TODO code: delete on exit
}


double FlarmCalculations::Average30s(long flarmId, double curTime, double curAltitude)
{
  ClimbAverageCalculator *itemTemp = NULL;
  AverageCalculatorMap::iterator iterFind = averageCalculatorMap.find(flarmId);
  if( iterFind != averageCalculatorMap.end() )
    {
      itemTemp = averageCalculatorMap[flarmId];
    }
  else
    {
      itemTemp = new ClimbAverageCalculator();
      averageCalculatorMap[flarmId] = itemTemp;
    }
  return itemTemp->GetAverage(curTime, curAltitude, 30);
}

#endif

int FindFlarmSlot(const NMEA_INFO &GPS_INFO, int flarmId)
{
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++)
    {
      if (GPS_INFO.FLARM_Traffic[z].ID == flarmId)
	{
	  return z;
	}
    }
  return -1;
}

int FindFlarmSlot(const NMEA_INFO &GPS_INFO, TCHAR *flarmCN)
{
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++)
    {
      if (_tcscmp(GPS_INFO.FLARM_Traffic[z].Name, flarmCN) == 0)
	{
	  return z;
	}
    }
  return -1;
}

bool IsFlarmTargetCNInRange(const NMEA_INFO &GPS_INFO, const long target_id)
{
  bool FlarmTargetContact = false;
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++) {
    if (GPS_INFO.FLARM_Traffic[z].ID != 0) {
      if (GPS_INFO.FLARM_Traffic[z].ID == target_id) {
        FlarmTargetContact = true;
        break;
      }
    }
  }
  return FlarmTargetContact;
}
