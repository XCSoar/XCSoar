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

#if !defined(AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
BOOL DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			double screen_distance);

double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic, 
					DERIVED_INFO *Calculated, 
					int thepoint); // VENTA3

bool ClearAirspaceWarnings(const bool ack, const bool allday=false);
void RefreshTaskStatistics(void);
void  SetWindEstimate(const double speed, const double bearing, const int quality=6);

void CloseCalculations(void);
void InitCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      const bool doadvance, const bool doannounce);

bool  InAATTurnSector(const double longitude, const double latitude, const int thepoint);

void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

double FAIFinishHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int wp);
bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin);
bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin);


void BallastDump();

#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

int FindFlarmSlot(const int flarmId);
int FindFlarmSlot(const TCHAR *flarmCN);
bool IsFlarmTargetCNInRange(void);

#endif
