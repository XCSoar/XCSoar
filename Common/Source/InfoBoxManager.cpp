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

#include "InfoBoxManager.h"
#include "InfoBox.h"
#include "WindowControls.h"
#include "Protection.hpp"
#include "InfoBox.h"
#include "InfoBoxLayout.h"
#include "Formatter/TeamCode.hpp"
#include "Formatter/WayPoint.hpp"
#include "Formatter/LowWarning.hpp"
#include "Formatter/Time.hpp"
#include "Dialogs.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "Device/Parser.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "Registry.hpp"
#include "Screen/Blank.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Interface.hpp"
#include "Battery.h"
#include "FlarmCalculations.h"
#include "UtilsSystem.hpp"
#include "MainWindow.hpp"
#include "MapWindow.h"
#include "Defines.h"
#include "options.h" /* for IBLSCALE() */
#include "Components.hpp"
#include "WayPointList.hpp"
#include "XCSoar.h"

#include <assert.h>

BufferWindow InfoBoxManager::full_window;

static bool InfoBoxesDirty= false;
static bool InfoBoxesHidden = false;
static double LastFlipBoxTime = 0; // VENTA3
unsigned numInfoWindows = 8;

InfoBox *InfoBoxes[MAXINFOWINDOWS];

static int InfoType[MAXINFOWINDOWS] =
#ifdef GNAV
  {
    873336334,
    856820491,
    822280982,
    2829105,
    103166000,
    421601569,
    657002759,
    621743887,
    439168301
  };
#else
  {921102,
   725525,
   262144,
   74518,
   657930,
   2236963,
   394758,
   1644825};
#endif

typedef struct _SCREEN_INFO
{
  UnitGroup_t UnitGroup;
  const TCHAR Description[DESCRIPTION_SIZE +1];
  const TCHAR Title[TITLE_SIZE + 1];
  InfoBoxFormatter *Formatter;
  void (*Process)(int UpDown);
  char next_screen;
  char prev_screen;
} SCREEN_INFO;


// Groups:
//   Altitude 0,1,20,33
//   Aircraft info 3,6,23,32,37,47,54
//   LD 4,5,19,38,53, 66    VENTA-ADDON added 66 for GR final
//   Vario 2,7,8,9,21,22,24,44
//   Wind 25,26,48,49,50
//   Mcready 10,34,35,43
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36,39,40,41,42,45,46
SCREEN_INFO Data_Options[] = {
          // 0
	  {ugAltitude,        TEXT("Height GPS"), TEXT("H GPS"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_Altitude, 1, 33},
	  // 1
	  {ugAltitude,        TEXT("Height AGL"), TEXT("H AGL"), new FormatterLowWarning(TEXT("%2.0f"),0.0), ActionInterface::on_key_None, 20, 0},
	  // 2
	  {ugVerticalSpeed,   TEXT("Thermal last 30 sec"), TEXT("TC 30s"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), ActionInterface::on_key_None, 7, 44},
	  // 3
#ifdef FIVV
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), ActionInterface::on_key_None, 6, 54},
#else
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), ActionInterface::on_key_None, 6, 54},
#endif
	  // 4
	  {ugNone,            TEXT("L/D instantaneous"), TEXT("L/D Inst"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 5, 38},
	  // 5
	  {ugNone,            TEXT("L/D cruise"), TEXT("L/D Cru"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 19, 4},
	  // 6
	  {ugHorizontalSpeed, TEXT("Speed ground"), TEXT("V Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_Speed, 23, 3},
	  // 7
	  {ugVerticalSpeed,   TEXT("Last Thermal Average"), TEXT("TL Avg"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 8, 2},
	  // 8
	  {ugAltitude,        TEXT("Last Thermal Gain"), TEXT("TL Gain"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 9, 7},
	  // 9
	  {ugNone,            TEXT("Last Thermal Time"), TEXT("TL Time"), new FormatterTime(TEXT("%04.0f")), ActionInterface::on_key_None, 21, 8},
	  // 10
	  {ugVerticalSpeed,   TEXT("MacCready Setting"), TEXT("MacCready"), new InfoBoxFormatter(TEXT("%2.1f")), ActionInterface::on_key_MacCready, 34, 43},
	  // 11
	  {ugDistance,        TEXT("Next Distance"), TEXT("WP Dist"), new InfoBoxFormatter(TEXT("%2.1f")), ActionInterface::on_key_None, 12, 31},
	  // 12
	  {ugAltitude,        TEXT("Next Altitude Difference"), TEXT("WP AltD"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 13, 11},
	  // 13
	  {ugAltitude,        TEXT("Next Altitude Required"), TEXT("WP AltR"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 15, 12},
	  // 14
	  {ugNone,            TEXT("Next Waypoint"), TEXT("Next"), new FormatterWaypoint(TEXT("\0")), ActionInterface::on_key_Waypoint, 36, 46},
	  // 15
	  {ugAltitude,        TEXT("Final Altitude Difference"), TEXT("Fin AltD"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 16, 13},
	  // 16
	  {ugAltitude,        TEXT("Final Altitude Required"), TEXT("Fin AltR"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 17, 15},
	  // 17
	  {ugTaskSpeed, TEXT("Speed Task Average"), TEXT("V Task Av"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 18, 16},
	  // 18
	  {ugDistance,        TEXT("Final Distance"), TEXT("Fin Dis"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 27, 17},
	  // 19
	  {ugNone,            TEXT("Final LD"), TEXT("Fin LD"), new InfoBoxFormatter(TEXT("%1.0f")), ActionInterface::on_key_None, 38, 5},
	  // 20
	  {ugAltitude,        TEXT("Terrain Elevation"), TEXT("H Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 33, 1},
	  // 21
	  {ugVerticalSpeed,   TEXT("Thermal Average"), TEXT("TC Avg"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), ActionInterface::on_key_None, 22, 9},
	  // 22
	  {ugAltitude,        TEXT("Thermal Gain"), TEXT("TC Gain"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 24, 21},
	  // 23
#ifdef FIVV
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), ActionInterface::on_key_Direction, 32, 6},
#else
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), ActionInterface::on_key_Direction, 32, 6},
#endif
	  // 24
	  {ugVerticalSpeed,   TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 44, 22},
	  // 25
	  {ugWindSpeed,       TEXT("Wind Speed"), TEXT("Wind V"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_WindSpeed, 26, 50},
	  // 26
#ifdef FIVV
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), ActionInterface::on_key_WindDirection, 48, 25},
#else
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), ActionInterface::on_key_WindDirection, 48, 25},
#endif
	  // 27
	  {ugNone,            TEXT("AA Time"), TEXT("AA Time"), new FormatterAATTime(TEXT("%2.0f")), ActionInterface::on_key_None, 28, 18},
	  // 28
	  {ugDistance,        TEXT("AA Distance Max"), TEXT("AA Dmax"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 29, 27},
	  // 29
	  {ugDistance,        TEXT("AA Distance Min"), TEXT("AA Dmin"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 30, 28},
	  // 30
	  {ugTaskSpeed, TEXT("AA Speed Max"), TEXT("AA Vmax"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 31, 29},
	  // 31
	  {ugTaskSpeed, TEXT("AA Speed Min"), TEXT("AA Vmin"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 51, 30},
	  // 32
	  {ugHorizontalSpeed, TEXT("Airspeed IAS"), TEXT("V IAS"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_Airspeed, 37, 23},
	  // 33
	  {ugAltitude,        TEXT("Pressure Altitude"), TEXT("H Baro"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 0, 20},
	  // 34
	  {ugHorizontalSpeed, TEXT("Speed MacReady"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 35, 10},
	  // 35
	  {ugNone,            TEXT("Percentage climb"), TEXT("% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 43, 34},
	  // 36
	  {ugNone,            TEXT("Time of flight"), TEXT("Time flt"), new FormatterTime(TEXT("%04.0f")), ActionInterface::on_key_None, 39, 14},
	  // 37
	  {ugNone,            TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), ActionInterface::on_key_Accelerometer, 47, 32},
	  // 38
	  {ugNone,            TEXT("Next LD"), TEXT("WP LD"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 53, 19},
	  // 39
	  {ugNone,            TEXT("Time local"), TEXT("Time loc"), new FormatterTime(TEXT("%04.0f")), ActionInterface::on_key_None, 40, 36},
	  // 40
	  {ugNone,            TEXT("Time UTC"), TEXT("Time UTC"), new FormatterTime(TEXT("%04.0f")), ActionInterface::on_key_None, 41, 39},
	  // 41
	  {ugNone,            TEXT("Task Time To Go"), TEXT("Fin ETE"), new FormatterAATTime(TEXT("%04.0f")), ActionInterface::on_key_None, 42, 40},
	  // 42
	  {ugNone,            TEXT("Next Time To Go"), TEXT("WP ETE"), new FormatterAATTime(TEXT("%04.0f")), ActionInterface::on_key_None, 45, 41},
	  // 43
	  {ugHorizontalSpeed, TEXT("Speed Dolphin"), TEXT("V Opt"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 10, 35},
	  // 44
	  {ugVerticalSpeed,   TEXT("Netto Vario"), TEXT("Netto"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 2, 24},
	  // 45
	  {ugNone,            TEXT("Task Arrival Time"), TEXT("Fin ETA"), new FormatterAATTime(TEXT("%04.0f")), ActionInterface::on_key_None, 46, 42},
	  // 46
	  {ugNone,            TEXT("Next Arrival Time"), TEXT("WP ETA"), new FormatterTime(TEXT("%04.0f")), ActionInterface::on_key_None, 14, 45},
	  // 47
	  {ugNone,            TEXT("Bearing Difference"), TEXT("Brng D"), new FormatterDiffBearing(TEXT("")), ActionInterface::on_key_None, 54, 37},
	  // 48
	  {ugNone,            TEXT("Outside Air Temperature"), TEXT("OAT"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), ActionInterface::on_key_None, 49, 26},
	  // 49
	  {ugNone,            TEXT("Relative Humidity"), TEXT("RelHum"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 50, 48},
	  // 50
	  {ugNone,            TEXT("Forecast Temperature"), TEXT("MaxTemp"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), ActionInterface::on_key_ForecastTemperature, 49, 25},
	  // 51
	  {ugDistance,        TEXT("AA Distance Tgt"), TEXT("AA Dtgt"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 52, 31},
	  // 52
	  {ugTaskSpeed, TEXT("AA Speed Tgt"), TEXT("AA Vtgt"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 11, 51},
	  // 53
	  {ugNone,            TEXT("L/D vario"), TEXT("L/D vario"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 4, 38},
	  // 54
	  {ugHorizontalSpeed, TEXT("Airspeed TAS"), TEXT("V TAS"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_Airspeed, 3, 47},
	  // 55
	  {ugNone,            TEXT("Own Team Code"), TEXT("TeamCode"), new FormatterTeamCode(TEXT("\0")), ActionInterface::on_key_TeamCode, 56, 54},
	  // 56
#ifdef FIVV
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), ActionInterface::on_key_None, 57, 55},
#else
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), ActionInterface::on_key_None, 57, 55},
#endif
	  // 57
	  {ugNone,            TEXT("Team Bearing Diff"), TEXT("Team Bd"), new FormatterDiffTeamBearing(TEXT("")), ActionInterface::on_key_None, 58, 56},
	  // 58
	  {ugNone,            TEXT("Team Range"), TEXT("Team Dis"), new InfoBoxFormatter(TEXT("%2.1f")), ActionInterface::on_key_None, 55, 57},
          // 59
	  {ugTaskSpeed, TEXT("Speed Task Instantaneous"), TEXT("V Tsk Ins"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 18, 16},
          // 60
	  {ugDistance, TEXT("Distance Home"), TEXT("Home Dis"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 18, 16},
	  // 61
	  {ugTaskSpeed, TEXT("Speed Task Achieved"), TEXT("V Tsk Ach"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 18, 16},
          // 62
	  {ugNone,            TEXT("AA Delta Time"), TEXT("AA dT"), new FormatterAATTime(TEXT("%2.0f")), ActionInterface::on_key_None, 28, 18},
          // 63
	  {ugVerticalSpeed,   TEXT("Thermal All"), TEXT("TC All"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 8, 2},
          // 64
	  {ugVerticalSpeed,   TEXT("Distance Vario"), TEXT("D Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 8, 2},
	  // 65
#ifndef GNAV
	  {ugNone,            TEXT("Battery Percent"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.0f%%")), ActionInterface::on_key_None, 49, 26},
#else
	  {ugNone,            TEXT("Battery Voltage"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.1fV")), ActionInterface::on_key_None, 49, 26},
#endif
	  // 66  VENTA-ADDON added Final GR
	  // VENTA-TODO: fix those 38,5 numbers to point correctly menu items
	  {ugNone,            TEXT("Final GR"), TEXT("Fin GR"), new InfoBoxFormatter(TEXT("%1.1f")), ActionInterface::on_key_None, 38, 5},

	  // 67 VENTA3-ADDON Alternate1 destinations infoboxes  TODO> fix 36 46 to something correct
	  {ugNone,            TEXT("Alternate1 GR"), TEXT("Altern 1"), new FormatterAlternate(TEXT("\0")), ActionInterface::on_key_Alternate1, 36, 46},
	  // 68 Alternate 2
	  {ugNone,            TEXT("Alternate2 GR"), TEXT("Altern 2"), new FormatterAlternate(TEXT("\0")), ActionInterface::on_key_Alternate2, 36, 46},
	  // 69 BestAlternate aka BestLanding
	  {ugNone,            TEXT("Best Alternate"), TEXT("BestAltn"), new FormatterAlternate(TEXT("\0")), ActionInterface::on_key_BestAlternate, 36, 46},
          // 70
	  {ugAltitude,        TEXT("QFE GPS"), TEXT("QFE GPS"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_QFEAltitude, 1, 33},
          // 71 TODO FIX those 19,4 values
	  {ugNone,            TEXT("L/D Average"), TEXT("L/D Avg"), new InfoBoxFormatter(TEXT("%2.0f")), ActionInterface::on_key_None, 19, 4},
	  // 72 //
	  {ugNone,   TEXT("Experimental1"), TEXT("Exp1"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 8, 2},
	  // 73 //
	  {ugNone,   TEXT("Experimental2"), TEXT("Exp2"), new InfoBoxFormatter(TEXT("%-2.1f")), ActionInterface::on_key_None, 8, 2},
	};

const unsigned NUMSELECTSTRINGS = 74;

// TODO locking
void InfoBoxManager::Hide() {
  InfoBoxesHidden = true;
  for (unsigned i = 0; i < numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(false);
  }

  full_window.hide();
}


void InfoBoxManager::Show() {
  InfoBoxesHidden = false;
  for (unsigned i = 0; i < numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(true);
  }
}

int
InfoBoxManager::GetFocused()
{
  for (unsigned i = 0; i < numInfoWindows; i++)
    if (InfoBoxes[i]->has_focus())
      return i;

  return -1;
}

void InfoBoxManager::Event_Select(int i) {
  int InfoFocus = GetFocused();

  if (InfoFocus < 0) {
    InfoFocus = i >= 0 ? 0 : numInfoWindows - 1;
  } else {
    InfoFocus += i;

    if (InfoFocus < 0 || (unsigned)InfoFocus >= numInfoWindows)
      InfoFocus = -1;
  }

  if (InfoFocus >= 0) {
    main_window.map.set_focus();
  } else {
    InfoBoxes[i]->set_focus();
    DisplayInfoBox();
    InputEvents::setMode(InputEvents::MODE_INFOBOX);
  }
}


////////////////////////////////////

int InfoBoxManager::getType(unsigned i, unsigned layer) {
  assert(i < MAXINFOWINDOWS);
  assert(layer < 4);

  switch(layer) {
  case 0:
    return InfoType[i] & 0xff;         // climb
  case 1:
    return (InfoType[i] >> 8) & 0xff;  // cruise
  case 2:
    return (InfoType[i] >> 16) & 0xff; // final glide
  case 3:
    return (InfoType[i] >> 24) & 0xff; // auxiliary
  };

  return 0xdeadbeef; /* not reachable */
}

int InfoBoxManager::getTypeAll(unsigned i) {
  assert(i < MAXINFOWINDOWS);

  return InfoType[i];
}

void InfoBoxManager::setTypeAll(unsigned i, unsigned j) {
  assert(i < MAXINFOWINDOWS);

  InfoType[i] = j;
  // TODO: check it's within range
}

#define m_min(a,b)     (((a)<(b))?(a):(b))
#define m_max(a,b)	(((a)>(b))?(a):(b))


int InfoBoxManager::getType(unsigned i) {
  unsigned retval = 0;

  if (SettingsMap().EnableAuxiliaryInfo) {
    retval = getType(i,3);
  } else {
    if (MapProjection().GetDisplayMode() == dmCircling)
      retval = getType(i,0);
    else if (MapProjection().GetDisplayMode() == dmFinalGlide) {
      retval = getType(i,2);
    } else {
      retval = getType(i,1); // cruise
    }
  }
  return m_min(NUMSELECTSTRINGS - 1, retval);
}


void InfoBoxManager::setType(unsigned i, char j, unsigned layer)
{
  assert(i < MAXINFOWINDOWS);

  switch(layer) {
  case 0:
    InfoType[i] &= 0xffffff00;
    InfoType[i] += (j);
    break;
  case 1:
    InfoType[i] &= 0xffff00ff;
    InfoType[i] += (j<<8);
    break;
  case 2:
    InfoType[i] &= 0xff00ffff;
    InfoType[i] += (j<<16);
    break;
  case 3:
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j<<24);
    break;
  };
}

void InfoBoxManager::setType(unsigned i, char j)
{
  if (SettingsMap().EnableAuxiliaryInfo) {
    setType(i, 3, j);
  } else {
    if (MapProjection().GetDisplayMode() == dmCircling) {
      setType(i, 0, j);
    } else if (MapProjection().GetDisplayMode() == dmFinalGlide) {
      setType(i, 2, j);
    } else {
      setType(i, 1, j);
    }
  }
}


void InfoBoxManager::Event_Change(int i) {
  int j=0, k;

  int InfoFocus = GetFocused();
  if (InfoFocus<0) {
    return;
  }

  k = getType(InfoFocus);
  if (i>0) {
    j = Data_Options[k].next_screen;
  }
  if (i<0) {
    j = Data_Options[k].prev_screen;
  }

  // TODO code: if i==0, go to default or reset

  setType(InfoFocus, j);
  DisplayInfoBox();

}

void InfoBoxManager::DisplayInfoBox(void)
{
  if (InfoBoxesHidden)
    return;

  static int DisplayType[MAXINFOWINDOWS];
  static bool first=true;
  static int DisplayTypeLast[MAXINFOWINDOWS];
  static bool FlipBoxValue = false;

  // VENTA3 - Dynamic box values
  if (LastFlipBoxTime > DYNABOXTIME ) {
    FlipBoxValue = ( FlipBoxValue == false );
    LastFlipBoxTime = 0;
  }

  // JMW note: this is updated every GPS time step

  for (unsigned i = 0; i < numInfoWindows; i++) {

    // VENTA3
    // All calculations are made in a separate thread. Slow calculations should apply to
    // the function DoCalculationsSlow() . Do not put calculations here!

    DisplayType[i] = getType(i);
    Data_Options[DisplayType[i]].Formatter->AssignValue(DisplayType[i]);

    TCHAR sTmp[32];

    int color = 0;

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i])||first);

    int theactive = task.getActiveIndex();
    if (!task.ValidTaskPoint(theactive)) {
      theactive = -1;
    }

    //
    // Set Infobox title and middle value. Bottom line comes next
    //
    switch (DisplayType[i]) {

    case 67: // VENTA3 alternate1 and 2
    case 68:
    case 69:

	if (DisplayType[i]==67)
	  ActiveAlternate=SettingsComputer().Alternate1;
	else
	  if (DisplayType[i]==68)
	    ActiveAlternate=SettingsComputer().Alternate2;
	  else
	    ActiveAlternate=Calculated().BestAlternate;

	InfoBoxes[i]->SetSmallerFont(false);
	if ( ActiveAlternate != -1 ) {
		InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Formatter->
			   RenderTitle(&color));
		InfoBoxes[i]->SetColor(color);
		InfoBoxes[i]->SetValue(Data_Options[DisplayType[i]].Formatter->
			   Render(&color));
		InfoBoxes[i]->SetColor(color);
	} else {
		if ( DisplayType[i]==67 )
			InfoBoxes[i]->SetTitle(TEXT("Altern1"));
		else if ( DisplayType[i]==68 )
			InfoBoxes[i]->SetTitle(TEXT("Altern2"));
		else	InfoBoxes[i]->SetTitle(TEXT("BestAltr"));
		InfoBoxes[i]->SetValue(TEXT("---"));
		InfoBoxes[i]->SetColor(-1);
	}
      if (needupdate)
	InfoBoxes[i]->SetValueUnit(Units::GetUserUnitByGroup(
          Data_Options[DisplayType[i]].UnitGroup));
	break;
    case 55:
      InfoBoxes[i]->SetSmallerFont(true);
      if (needupdate)
	InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Title);

      InfoBoxes[i]->
	SetValue(Data_Options[DisplayType[i]].Formatter->Render(&color));

      // to be optimized!
      if (needupdate)
	InfoBoxes[i]->
	  SetValueUnit(Units::GetUserUnitByGroup(
              Data_Options[DisplayType[i]].UnitGroup)
	  );
      InfoBoxes[i]->SetColor(color);
      break;
    case 14: // Next waypoint
      InfoBoxes[i]->SetSmallerFont(false);
      if (theactive != -1){
	InfoBoxes[i]->
	  SetTitle(Data_Options[DisplayType[i]].Formatter->
		   Render(&color));
	InfoBoxes[i]->SetColor(color);
	InfoBoxes[i]->
	  SetValue(Data_Options[47].Formatter->Render(&color));
      }else{
	InfoBoxes[i]->SetTitle(TEXT("Next"));
	InfoBoxes[i]->SetValue(TEXT("---"));
	InfoBoxes[i]->SetColor(-1);
      }
      if (needupdate)
	InfoBoxes[i]->SetValueUnit(Units::GetUserUnitByGroup(
          Data_Options[DisplayType[i]].UnitGroup)
      );
      break;
    default:
      InfoBoxes[i]->SetSmallerFont(false);
      if (needupdate)
	InfoBoxes[i]->SetTitle(Data_Options[DisplayType[i]].Title);

      InfoBoxes[i]->
          SetValue(Data_Options[DisplayType[i]].Formatter->Render(&color));

      // to be optimized!
      if (needupdate)
	InfoBoxes[i]->
	  SetValueUnit(Units::GetUserUnitByGroup(
            Data_Options[DisplayType[i]].UnitGroup)
	  );

      InfoBoxes[i]->SetColor(color);
    };

    //
    // Infobox bottom line
    //
    switch (DisplayType[i]) {
    case 14: // Next waypoint

      if (theactive != -1){
        int index = task.getWaypointIndex();
        if ((index>=0)&& way_points.verify_index(index)) {
          InfoBoxes[i]->
            SetComment(way_points.get(index).Comment);
        }
      }
      InfoBoxes[i]->SetComment(TEXT(""));
      break;
    case 10:
      if (SettingsComputer().AutoMacCready)
	InfoBoxes[i]->SetComment(TEXT("AUTO"));
      else
	InfoBoxes[i]->SetComment(TEXT("MANUAL"));
      break;
    case 0: // GPS Alt
      Units::FormatAlternateUserAltitude(Basic().Altitude,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 1: // AGL
      Units::FormatAlternateUserAltitude(Calculated().AltitudeAGL,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 33:
      Units::FormatAlternateUserAltitude(Basic().BaroAltitude,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 27: // AAT time to go
    case 36: // flight time
    case 39: // current time
    case 40: // gps time
    case 41: // task time to go
    case 42: // task time to go
    case 45: // ete
    case 46: // leg ete
    case 62: // ete
      if (Data_Options[DisplayType[i]].Formatter->isValid()) {
        InfoBoxes[i]->
          SetComment(Data_Options[DisplayType[i]].Formatter->GetCommentText());
      } else {
        InfoBoxes[i]->
          SetComment(TEXT(""));
      }
      break;
    case 43:
      if (SettingsComputer().EnableBlockSTF) {
	InfoBoxes[i]->SetComment(TEXT("BLOCK"));
      } else {
	InfoBoxes[i]->SetComment(TEXT("DOLPHIN"));
      }
      break;
    case 55: // own team code
      InfoBoxes[i]->SetComment(Calculated().TeammateCode);
      if (SettingsComputer().TeamFlarmTracking)
	{
	  if (IsFlarmTargetCNInRange(Basic(),SettingsComputer().TeamFlarmIdTarget))
	    {
	      InfoBoxes[i]->SetColorBottom(2);
	    }
	  else
	    {
	      InfoBoxes[i]->SetColorBottom(1);
	    }
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(0);
	}
      break;
    case 56: // team bearing

      if (SettingsComputer().TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(SettingsComputer().TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(SettingsComputer().TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}

      if (IsFlarmTargetCNInRange(Basic(),SettingsComputer().TeamFlarmIdTarget))
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 57: // team bearing dif

      if (SettingsComputer().TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(SettingsComputer().TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(SettingsComputer().TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}
      if (IsFlarmTargetCNInRange(Basic(),SettingsComputer().TeamFlarmIdTarget))
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 58: // team range

      if (SettingsComputer().TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(SettingsComputer().TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(SettingsComputer().TeamFlarmCNTarget);
	    }
	  else
	    {
	      InfoBoxes[i]->SetComment(TEXT("???"));
	    }
	}
      else
	{
	  InfoBoxes[i]->SetComment(TEXT("---"));
	}
      if (IsFlarmTargetCNInRange(Basic(),SettingsComputer().TeamFlarmIdTarget))
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
	// VENTA3 wind speed + bearing bottom line
	case 25:
	  if (Calculated().WindBearing == 0 )
	    _stprintf(sTmp,_T("0%s"),_T(DEG)); else
	    _stprintf(sTmp,_T("%1.0d%s"),(int)Calculated().WindBearing,_T(DEG));
	  InfoBoxes[i]->SetComment(sTmp);
	  break;

	// VENTA3 radial
	case 60:
	  if ( SettingsComputer().HomeWaypoint == -1 ) {  // should be redundant
	    InfoBoxes[i]->SetComment(TEXT(""));
	    break;
	  }
	  if ( Calculated().HomeRadial == 0 ) {
	    _stprintf(sTmp,_T("0%s"),_T(DEG));
	  } else {
	    _stprintf(sTmp,_T("%1.0d%s"),(int)Calculated().HomeRadial,_T(DEG));
	  }
	  InfoBoxes[i]->SetComment(sTmp);
	  break;

	// VENTA3 battery temperature under voltage. There is a good
	// reason to see the temperature, if available: many PNA/PDA
	// will switch OFF during flight under direct sunlight for
	// several hours due to battery temperature too high!! The 314
	// does!

	// TODO: check temperature too high and set a warning flag to
	// be used by an event or something
#if !defined(GNAV) && !defined(WINDOWSPC) && !defined(HAVE_POSIX)
	case 65:
	  if ( PDABatteryTemperature >0 ) {
	    _stprintf(sTmp,_T("%1.0d%SC"),(int)PDABatteryTemperature,_T(DEG));
	    InfoBoxes[i]->SetComment(sTmp);
	  } else
	    InfoBoxes[i]->SetComment(TEXT(""));
	  break;
	#endif

	// VENTA3 alternates
	case 67:
	case 68:
	case 69:
	  if ( ActiveAlternate == -1 ) {  // should be redundant
	    InfoBoxes[i]->SetComment(TEXT(""));
	    break;
	  }
	  if (FlipBoxValue == true) {
            Units::FormatUserDistance(way_points.get_calc(ActiveAlternate).Distance,
				      sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
	    InfoBoxes[i]->SetComment(sTmp);
	  } else {
            Units::FormatUserArrival(way_points.get_calc(ActiveAlternate).AltArrival,
				     sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
	    InfoBoxes[i]->SetComment(sTmp);
	  }
	  break;
	case 70: // QFE
	  /*
		 // Showing the diff value offset was just interesting ;-)
		if (FlipBoxValue == true) {
			//Units::FormatUserArrival(QFEAltitudeOffset,
			Units::FormatUserAltitude(QFEAltitudeOffset,
				 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
			InfoBoxes[i]->SetComment(sTmp);
		} else {
		*/
		//Units::FormatUserArrival(Basic().Altitude,
		Units::FormatUserAltitude(Basic().Altitude,
			 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
		InfoBoxes[i]->SetComment(sTmp);
		break;

    default:
      InfoBoxes[i]->SetComment(TEXT(""));
    };

    DisplayTypeLast[i] = DisplayType[i];

  }
  Paint();

  first = false;
}


void InfoBoxManager::ProcessKey(int keycode) {
  unsigned i;

  int InfoFocus = GetFocused();
  if (InfoFocus<0) return; // paranoid

  InputEvents::HideMenu();

  i = getType(InfoFocus);
  Data_Options[m_min(NUMSELECTSTRINGS - 1, i)].Process(keycode);

  InfoBoxesDirty = true;

  TriggerGPSUpdate(); // emulate update to trigger calculations

  ResetDisplayTimeOut();

}

void InfoBoxManager::DestroyInfoBoxFormatters() {
  //  CommandBar_Destroy(hWndCB);
  for (unsigned i = 0; i < NUMSELECTSTRINGS; i++) {
    delete Data_Options[i].Formatter;
  }
}

bool InfoBoxManager::IsFocus() {
  return GetFocused() >= 0;
}

void InfoBoxManager::InfoBoxDrawIfDirty(void) {
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (InfoBoxesDirty && !SettingsMap().ScreenBlanked) {
    DisplayInfoBox();
    InfoBoxesDirty = false;
  }
}

void InfoBoxManager::SetDirty(bool is_dirty) {
  InfoBoxesDirty = is_dirty;
}


void InfoBoxManager::ProcessTimer(void) {
  static double lasttime;

  if (Basic().Time != lasttime) {
    SetDirty(true);
    lasttime = Basic().Time;
  }
  InfoBoxDrawIfDirty();
  LastFlipBoxTime++;
}


void InfoBoxManager::ResetInfoBoxes(void) {
#ifdef GNAV
  InfoType[0]=873336334;
  InfoType[1]=856820491;
  InfoType[2]=822280982;
  InfoType[3]=2829105;
  InfoType[4]=103166000;
  InfoType[5]=421601569;
  InfoType[6]=657002759;
  InfoType[7]=621743887;
  InfoType[8]=439168301;
#else
  InfoType[0] = 921102;
  InfoType[1] = 725525;
  InfoType[2] = 262144;
  InfoType[3] = 74518;
  InfoType[4] = 657930;
  InfoType[5] = 2236963;
  InfoType[6] = 394758;
  InfoType[7] = 1644825;
#endif
}

const TCHAR *InfoBoxManager::GetTypeDescription(unsigned i) {
  return Data_Options[i].Description;
}


/////////////////////////////////////////////////////////////////////////////////
//
// TODO: this should go into the manager

extern InfoBox *InfoBoxes[MAXINFOWINDOWS];


void InfoBoxManager::Paint(void) {
  unsigned i;
  for (i = 0; i < numInfoWindows; i++)
    InfoBoxes[i]->Paint();

  if (!InfoBoxLayout::fullscreen) {
    full_window.hide();

    for (i=0; i<numInfoWindows; i++)
      InfoBoxes[i]->PaintFast();
  } else {
    Canvas &canvas = full_window.get_canvas();

    canvas.white_brush();
    canvas.white_pen();
    canvas.rectangle(0, 0, canvas.get_width(), canvas.get_height());

    for (i=0; i<numInfoWindows; i++) {

      // JMW TODO: make these calculated once only.
      int x, y;
      int rx, ry;
      int rw;
      int rh;
      double fw, fh;
      if (InfoBoxLayout::landscape) {
        rw = 84;
        rh = 68;
      } else {
        rw = 120;
        rh = 80;
      }
      fw = rw/(double)InfoBoxLayout::ControlWidth;
      fh = rh/(double)InfoBoxLayout::ControlHeight;
      double f = m_min(fw, fh);
      rw = (int)(f*InfoBoxLayout::ControlWidth);
      rh = (int)(f*InfoBoxLayout::ControlHeight);

      if (InfoBoxLayout::landscape) {
        rx = i % 3;
        ry = i / 3;

        x = (rw+4)*rx;
        y = (rh+3)*ry;

      } else {
        rx = i % 2;
        ry = i / 4;

        x = (rw)*rx;
        y = (rh)*ry;

      }
      InfoBoxes[i]->PaintInto(canvas,
                              IBLSCALE(x), IBLSCALE(y), IBLSCALE(rw), IBLSCALE(rh));
    }

    full_window.show();
    full_window.commit_buffer();
  }
}


RECT InfoBoxManager::Create(RECT rc) {
  int xoff, yoff, sizex, sizey;

  RECT retval = InfoBoxLayout::GetInfoBoxSizes(rc);

  // JMW created full screen infobox mode
  xoff=0;
  yoff=0;
  sizex=rc.right-rc.left;
  sizey=rc.bottom-rc.top;

  full_window.set(main_window, xoff, yoff, sizex, sizey,
                  false, false, false);

  // create infobox windows

  for (unsigned i = 0; i < numInfoWindows; i++)
    {
      InfoBoxLayout::GetInfoBoxPosition(i, rc, &xoff, &yoff, &sizex, &sizey);

      InfoBoxes[i] = new InfoBox(main_window,
				 xoff, yoff, sizex, sizey);

      int Border=0;
      if (InfoBoxLayout::gnav){
        if (i>0)
          Border |= BORDERTOP;
        if (i<6)
          Border |= BORDERRIGHT;
        InfoBoxes[i]->SetBorderKind(Border);
      } else
      if (!InfoBoxLayout::landscape) {
        Border = 0;
        if (i<4) {
          Border |= BORDERBOTTOM;
        } else {
          Border |= BORDERTOP;
        }
        Border |= BORDERRIGHT;
        InfoBoxes[i]->SetBorderKind(Border);
      }
    }

  return retval; // for use in setting MapWindow
}

void InfoBoxManager::Destroy(void){
  for (unsigned i = 0; i < numInfoWindows; i++){
    delete (InfoBoxes[i]);
  }

  full_window.reset();

  DestroyInfoBoxFormatters();
}

