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

#if !defined(XCSOAR_SETTINGS_AIRSPACE_H)
#define XCSOAR_SETTINGS_AIRSPACE_H

#define OTHER                           0
#define RESTRICT                        1
#define PROHIBITED                      2
#define DANGER                          3
#define CLASSA				4
#define CLASSB				5
#define CLASSC				6
#define CLASSD				7
#define	NOGLIDER			8
#define CTR                             9
#define WAVE				10
#define AATASK				11
#define CLASSE				12
#define CLASSF				13
#define AIRSPACECLASSCOUNT              14

// modes
#define ALLON 0
#define CLIP 1
#define AUTO 2
#define ALLBELOW 3
#define INSIDE 4
#define ALLOFF 5

extern int    iAirspaceMode[AIRSPACECLASSCOUNT];
extern int    iAirspaceBrush[AIRSPACECLASSCOUNT];
extern int    iAirspaceColour[AIRSPACECLASSCOUNT];
extern int    AirspacePriority[AIRSPACECLASSCOUNT];

extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;
extern int AltitudeMode;
extern int ClipAltitude;
extern int AltWarningMargin;
extern double airspace_QNH; 

#endif
