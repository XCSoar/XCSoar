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

#include "Polar/Historical.hpp"
#include "Polar/WinPilot.hpp"
#include "Polar/BuiltIn.hpp"
#include "LogFile.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs.h"
#include "McReady.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

typedef double PolarCoefficients_t[3];
typedef double WeightCoefficients_t[3];

const TCHAR *PolarLabels[] = {TEXT("Vintage - Ka6"),
			      TEXT("Club - ASW19"),
			      TEXT("Standard - LS8"),
			      TEXT("15M - ASW27"),
			      TEXT("18M - LS6C"),
			      TEXT("Open - ASW22"),
			      TEXT("WinPilot File")};

void CalculateNewPolarCoef(void)
{

  StartupStore(TEXT("Calculate New Polar Coef\n"));

  static PolarCoefficients_t Polars[7] =
    {
      {-0.0538770500225782443497, 0.1323114348, -0.1273364037098239098543},
      {-0.0532456270195884696748, 0.1509454717, -0.1474304674787072275183},
      {-0.0598306909918491529791, 0.1896480967, -0.1883344146619101871894},
      {-0.0303118230885946660507, 0.0771466019, -0.0799469636558217515699},
      {-0.0222929913566948641563, 0.0318771616, -0.0307925896846546928318},
      {-0.0430828898445299480353, 0.0746938776, -0.0487285153053357557183},
      {0.0, 0.0, 0.0}

    };


  /* Weights:
     0 Pilot Weight?
     1 Glider Weight
     2 BallastWeight
  */

  static WeightCoefficients_t Weights[7] = { {70,190,1},
                                             {70,250,100},
                                             {70,240,285},
                                             {70,287,165},  // w ok!
                                             {70,400,120},  //
                                             {70,527,303},
                                             {0,0,0}
  };
  static double WingAreas[7] = {
    12.4,  // Ka6
    11.0,  // ASW19
    10.5,  // LS8
    9.0,   // ASW27
    11.4,  // LS6C-18
    16.31, // ASW22
    0};
  int i;

  assert(sizeof(Polars)/sizeof(Polars[0]) == sizeof(Weights)/sizeof(Weights[0]));

  if (POLARID < sizeof(Polars)/sizeof(Polars[0])){
    for(i=0;i<3;i++){
      POLAR[i] = Polars[POLARID][i];
      WEIGHTS[i] = Weights[POLARID][i];
    }
    GlidePolar::WingArea = WingAreas[POLARID];
  }
  if (POLARID==POLARUSEWINPILOTFILE) {
    if (ReadWinPilotPolar())
    // polar data gets from winpilot file
      return;
  } else if (POLARID>POLARUSEWINPILOTFILE){
    if (ReadWinPilotPolarInternal(POLARID-7))
      // polar data get from build in table
      return;
  } else if (POLARID<POLARUSEWINPILOTFILE){
    // polar data get from historical table
    return;
  }

  // ups
  // error reading winpilot file

  POLARID = 2;              // do it again with default polar (LS8)

  CalculateNewPolarCoef();
  MessageBoxX(gettext(TEXT("Error loading Polar file!\r\nUse LS8 Polar.")),
              gettext(TEXT("Warning")),
              MB_OK|MB_ICONERROR);

}

