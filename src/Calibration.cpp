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

#include "Calibration.hpp"
#include "XCSoar.h"
#include "Math/FastMath.h"
#include "LogFile.hpp"
#include "XCSoar.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

#include <tchar.h>

#ifndef _MSC_VER
#include <algorithm>
using std::max;
#endif

#define NUM_CAL_SPEED 25
#define NUM_CAL_VARIO 101
#define NUM_CAL_VSPEED 50

static double calibration_tevario_val[NUM_CAL_SPEED][NUM_CAL_VARIO];
static unsigned int calibration_tevario_num[NUM_CAL_SPEED][NUM_CAL_VARIO];
static double calibration_speed_val[NUM_CAL_VSPEED];
static unsigned int calibration_speed_num[NUM_CAL_VSPEED];

void CalibrationInit(void) {
  int i, j;
  for (i=0; i< NUM_CAL_SPEED; i++) {
    for (j=0; j< NUM_CAL_VARIO; j++) {
      calibration_tevario_val[i][j] = 0;
      calibration_tevario_num[i][j] = 0;
    }
  }
  for (i=0; i< NUM_CAL_VSPEED; i++) {
    calibration_speed_val[i] = 0;
    calibration_speed_num[i] = 0;
  }
}


void CalibrationSave(void) {
  int i, j;
  double v, w = 0, wav;
  StartupStore(_T("Calibration data for TE vario\n"));
  for (i=0; i< NUM_CAL_SPEED; i++) {
    for (j=0; j< NUM_CAL_VARIO; j++) {
      if (calibration_tevario_num[i][j]>0) {
        v = i*2.0+20.0;
        w = (j-50.0)/10.0;
        wav = calibration_tevario_val[i][j]/calibration_tevario_num[i][j];
        StartupStore(_T("%g %g %g %d\n"), v, w, wav,
                  calibration_tevario_num[i][j]);
      }
    }
  }
  StartupStore(_T("Calibration data for ASI\n"));
  for (i=0; i< NUM_CAL_VSPEED; i++) {
    if (calibration_speed_num[i]>0) {
      v = i+20.0;
      wav = calibration_speed_val[i]/calibration_speed_num[i];
      StartupStore(_T("%g %g %g %d\n"), v, w, wav,
                calibration_speed_num[i]);
    }
  }
}

void
CalibrationUpdate(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  if (!Calculated->Flying) return;
  if ((!Basic->AirspeedAvailable) || (Basic->TrueAirspeed<=0)) {
    return;
  }
  double ias_to_tas = Basic->TrueAirspeed/
    max(fixed(1.0), Basic->IndicatedAirspeed);

  // Vario calibration info
  int index_te_vario = lround(Basic->GPSVarioTE*10)+50;
  int index_speed = lround((Basic->TrueAirspeed-20)/2);
  if (index_te_vario < 0)
    return;
  if (index_te_vario >= NUM_CAL_VARIO)
    return;
  if (index_speed<0)
    return;
  if (index_speed>= NUM_CAL_SPEED)
    return;

  calibration_tevario_val[index_speed][index_te_vario] +=
    (double)Basic->Vario*ias_to_tas;
  calibration_tevario_num[index_speed][index_te_vario] ++;

  // ASI calibration info
  int index_vspeed = lround((Basic->TrueAirspeed-20));
  if (index_vspeed<0)
    return;
  if (index_vspeed>= NUM_CAL_VSPEED)
    return;

  calibration_speed_val[index_vspeed] += (double)Basic->TrueAirspeedEstimated;
  calibration_speed_num[index_vspeed] ++;

}
