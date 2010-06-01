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

#include "InfoBoxManager.hpp"
#include "TaskClientUI.hpp"
#include "InfoBox.hpp"
#include "InfoBoxLayout.hpp"
#include "Form/Control.hpp"
#include "Protection.hpp"
#include "Formatter/TeamCode.hpp"
#include "Formatter/WayPoint.hpp"
#include "Formatter/LowWarning.hpp"
#include "Formatter/Time.hpp"
#include "Dialogs.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "Device/Parser.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Screen/Blank.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include "Interface.hpp"
#include "Battery.h"
#include "FlarmCalculations.h"
#include "UtilsSystem.hpp"
#include "MainWindow.hpp"
#include "MapWindow.hpp"
#include "Defines.h"
#include "Components.hpp"
#include "StringUtil.hpp"
#include "Appearance.hpp"

#include <assert.h>

#include <algorithm>

using std::min;

BufferWindow InfoBoxManager::full_window;

static bool InfoBoxesDirty = false;
static bool InfoBoxesHidden = false;
static double LastFlipBoxTime = 0; // VENTA3
unsigned numInfoWindows = 8;

InfoBox *InfoBoxes[MAXINFOWINDOWS];

static const int InfoTypeDefault[MAXINFOWINDOWS] = {
  921102,
  725525,
  262144,
  74518,
  657930,
  2236963,
  394758,
  1644825
};

static const int InfoTypeAltairDefault[MAXINFOWINDOWS] = {
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

static int InfoType[MAXINFOWINDOWS];

typedef struct _SCREEN_INFO
{
  UnitGroup_t UnitGroup;
  const TCHAR *Description;
  const TCHAR *Title;
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
//   MacCready 10,34,35,43
//   Nav 11,12,13,15,16,17,18,27,28,29,30,31
//   Waypoint 14,36,39,40,41,42,45,46
static const SCREEN_INFO Data_Options[] = {
  // 0
  { ugAltitude, _T("Height GPS"), _T("H GPS"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_Altitude,
    1, 33,
  },
  // 1
  { ugAltitude, _T("Height AGL"), _T("H AGL"),
    new FormatterLowWarning(_T("%2.0f"),0.0),
    ActionInterface::on_key_None,
    20, 0,
  },
  // 2
  { ugVerticalSpeed, _T("Thermal last 30 sec"), _T("TC 30s"),
    new FormatterLowWarning(_T("%-2.1f"),0.0),
    ActionInterface::on_key_None,
    7, 44,
  },
  // 3
  { ugNone, _T("Bearing"), _T("Bearing"),
#ifdef FIVV
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)),
#else
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)_T("T")),
#endif
    ActionInterface::on_key_None,
    6, 54,
  },
  // 4
  { ugNone, _T("L/D instantaneous"), _T("L/D Inst"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    5, 38,
  },
  // 5
  { ugNone, _T("L/D cruise"), _T("L/D Cru"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    19, 4,
  },
  // 6
  { ugHorizontalSpeed, _T("Speed ground"), _T("V Gnd"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_Speed,
    23, 3,
  },
  // 7
  { ugVerticalSpeed, _T("Last Thermal Average"), _T("TL Avg"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
  // 8
  { ugAltitude, _T("Last Thermal Gain"), _T("TL Gain"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    9, 7,
  },
  // 9
  { ugNone, _T("Last Thermal Time"), _T("TL Time"),
    new FormatterTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    21, 8,
  },
  // 10
  { ugVerticalSpeed, _T("MacCready Setting"), _T("MacCready"),
    new InfoBoxFormatter(_T("%2.1f")),
    ActionInterface::on_key_MacCready,
    34, 43,
  },
  // 11
  { ugDistance, _T("Next Distance"), _T("WP Dist"),
    new InfoBoxFormatter(_T("%2.1f")),
    ActionInterface::on_key_None,
    12, 31,
  },
  // 12
  { ugAltitude, _T("Next Altitude Difference"), _T("WP AltD"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    13, 11,
  },
  // 13
  { ugAltitude, _T("Next Altitude Required"), _T("WP AltR"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    15, 12,
  },
  // 14
  { ugNone, _T("Next Waypoint"), _T("Next"),
    new FormatterWaypoint(_T("\0")),
    ActionInterface::on_key_Waypoint,
    36, 46,
  },
  // 15
  { ugAltitude, _T("Final Altitude Difference"), _T("Fin AltD"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    16, 13,
  },
  // 16
  { ugAltitude, _T("Final Altitude Required"), _T("Fin AltR"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    17, 15,
  },
  // 17
  { ugTaskSpeed, _T("Speed Task Average"), _T("V Task Av"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    18, 16,
  },
  // 18
  { ugDistance, _T("Final Distance"), _T("Fin Dis"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    27, 17,
  },
  // 19
  { ugNone, _T("Final LD"), _T("Fin LD"),
    new InfoBoxFormatter(_T("%1.0f")),
    ActionInterface::on_key_None,
    38, 5,
  },
  // 20
  { ugAltitude, _T("Terrain Elevation"), _T("H Gnd"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    33, 1,
  },
  // 21
  { ugVerticalSpeed, _T("Thermal Average"), _T("TC Avg"),
    new FormatterLowWarning(_T("%-2.1f"),0.0),
    ActionInterface::on_key_None,
    22, 9,
  },
  // 22
  { ugAltitude, _T("Thermal Gain"), _T("TC Gain"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    24, 21,
  },
  // 23
  { ugNone, _T("Track"), _T("Track"),
#ifdef FIVV
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)),
#else
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)_T("T")),
#endif
    ActionInterface::on_key_Direction,
    32, 6,
  },
  // 24
  { ugVerticalSpeed, _T("Vario"), _T("Vario"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    44, 22,
  },
  // 25
  { ugWindSpeed, _T("Wind Speed"), _T("Wind V"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_WindSpeed,
    26, 50,
  },
  // 26
  { ugNone, _T("Wind Bearing"), _T("Wind B"),
#ifdef FIVV
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)),
#else
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)_T("T")),
#endif
    ActionInterface::on_key_WindDirection,
    48, 25,
  },
  // 27
  { ugNone, _T("AA Time"), _T("AA Time"),
    new FormatterAATTime(_T("%2.0f")),
    ActionInterface::on_key_None,
    28, 18,
  },
  // 28
  { ugDistance, _T("AA Distance Max"), _T("AA Dmax"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    29, 27,
  },
  // 29
  { ugDistance, _T("AA Distance Min"), _T("AA Dmin"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    30, 28,
  },
  // 30
  { ugTaskSpeed, _T("AA Speed Max"), _T("AA Vmax"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    31, 29,
  },
  // 31
  { ugTaskSpeed, _T("AA Speed Min"), _T("AA Vmin"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    51, 30,
  },
  // 32
  { ugHorizontalSpeed, _T("Airspeed IAS"), _T("V IAS"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_Airspeed,
    37, 23,
  },
  // 33
  { ugAltitude, _T("Pressure Altitude"), _T("H Baro"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    0, 20,
  },
  // 34
  { ugHorizontalSpeed, _T("Speed MacCready"), _T("V Mc"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    35, 10,
  },
  // 35
  { ugNone, _T("Percentage climb"), _T("% Climb"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    43, 34,
  },
  // 36
  { ugNone, _T("Time of flight"), _T("Time flt"),
    new FormatterTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    39, 14,
  },
  // 37
  { ugNone, _T("G load"), _T("G"),
    new InfoBoxFormatter(_T("%2.2f")),
    ActionInterface::on_key_Accelerometer,
    47, 32,
  },
  // 38
  { ugNone, _T("Next LD"), _T("WP LD"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    53, 19,
  },
  // 39
  { ugNone, _T("Time local"), _T("Time loc"),
    new FormatterTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    40, 36,
  },
  // 40
  { ugNone, _T("Time UTC"), _T("Time UTC"),
    new FormatterTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    41, 39,
  },
  // 41
  { ugNone, _T("Task Time To Go"), _T("Fin ETE"),
    new FormatterAATTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    42, 40,
  },
  // 42
  { ugNone, _T("Next Time To Go"), _T("WP ETE"),
    new FormatterAATTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    45, 41,
  },
  // 43
  { ugHorizontalSpeed, _T("Speed Dolphin"), _T("V Opt"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    10, 35,
  },
  // 44
  { ugVerticalSpeed, _T("Netto Vario"), _T("Netto"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    2, 24,
  },
  // 45
  { ugNone, _T("Task Arrival Time"), _T("Fin ETA"),
    new FormatterAATTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    46, 42,
  },
  // 46
  { ugNone, _T("Next Arrival Time"), _T("WP ETA"),
    new FormatterTime(_T("%04.0f")),
    ActionInterface::on_key_None,
    14, 45,
  },
  // 47
  { ugNone, _T("Bearing Difference"), _T("Brng D"),
    new FormatterDiffBearing(_T("")),
    ActionInterface::on_key_None,
    54, 37,
  },
  // 48
  { ugNone, _T("Outside Air Temperature"), _T("OAT"),
    new InfoBoxFormatter(_T("%2.1f")_T(DEG)),
    ActionInterface::on_key_None,
    49, 26,
  },
  // 49
  { ugNone, _T("Relative Humidity"), _T("RelHum"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    50, 48,
  },
  // 50
  { ugNone, _T("Forecast Temperature"), _T("MaxTemp"),
    new InfoBoxFormatter(_T("%2.1f")_T(DEG)),
    ActionInterface::on_key_ForecastTemperature, 
    49, 25,
  },
  // 51
  { ugDistance, _T("AA Distance Tgt"), _T("AA Dtgt"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    52, 31,
  },
  // 52
  { ugTaskSpeed, _T("AA Speed Tgt"), _T("AA Vtgt"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    11, 51,
  },
  // 53
  { ugNone, _T("L/D vario"), _T("L/D vario"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    4, 38,
  },
  // 54
  { ugHorizontalSpeed, _T("Airspeed TAS"), _T("V TAS"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_Airspeed,
    3, 47,
  },
  // 55
  { ugNone, _T("Own Team Code"), _T("TeamCode"),
    new FormatterTeamCode(_T("\0")),
    ActionInterface::on_key_TeamCode,
    56, 54,
  },
  // 56
  { ugNone, _T("Team Bearing"), _T("Tm Brng"),
#ifdef FIVV
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)),
#else
    new InfoBoxFormatter(_T("%2.0f")_T(DEG)_T("T")),
#endif
    ActionInterface::on_key_None,
    57, 55,
  },
  // 57
  { ugNone, _T("Team Bearing Diff"), _T("Team Bd"),
    new FormatterDiffTeamBearing(_T("")),
    ActionInterface::on_key_None,
    58, 56,
  },
  // 58
  { ugNone, _T("Team Range"), _T("Team Dis"),
    new InfoBoxFormatter(_T("%2.1f")),
    ActionInterface::on_key_None,
    55, 57,
  },
  // 59
  { ugTaskSpeed, _T("Speed Task Instantaneous"), _T("V Tsk Ins"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    18, 16,
  },
  // 60
  { ugDistance, _T("Distance Home"), _T("Home Dis"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    18, 16,
  },
  // 61
  { ugTaskSpeed, _T("Speed Task Achieved"), _T("V Tsk Ach"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    18, 16,
  },
  // 62
  { ugNone, _T("AA Delta Time"), _T("AA dT"),
    new FormatterAATTime(_T("%2.0f")),
    ActionInterface::on_key_None,
    28, 18,
  },
  // 63
  { ugVerticalSpeed, _T("Thermal All"), _T("TC All"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
  // 64
  { ugVerticalSpeed, _T("Distance Vario"), _T("D Vario"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
  // 65
#ifndef GNAV
  { ugNone, _T("Battery Percent"), _T("Battery"),
    new InfoBoxFormatter(_T("%2.0f%%")),
    ActionInterface::on_key_None,
    49, 26,
  },
#else
  { ugNone, _T("Battery Voltage"), _T("Battery"),
    new InfoBoxFormatter(_T("%2.1fV")),
    ActionInterface::on_key_None,
    49, 26,
  },
#endif
  // 66  VENTA-ADDON added Final GR
  // VENTA-TODO: fix those 38,5 numbers to point correctly menu items
  { ugNone, _T("Final GR"), _T("Fin GR"),
    new InfoBoxFormatter(_T("%1.1f")),
    ActionInterface::on_key_None,
    38, 5,
  },

  // 67 VENTA3-ADDON Alternate1 destinations infoboxes  TODO> fix 36 46 to something correct
  { ugNone, _T("Alternate1 GR"), _T("Altern 1"),
    new FormatterAlternate(_T("\0")),
    ActionInterface::on_key_Alternate1,
    36, 46,
  },
  // 68 Alternate 2
  { ugNone, _T("Alternate2 GR"), _T("Altern 2"),
    new FormatterAlternate(_T("\0")),
    ActionInterface::on_key_Alternate2,
    36, 46,
  },
  // 69 BestAlternate aka BestLanding
  { ugNone, _T("Best Alternate"), _T("BestAltn"),
    new FormatterAlternate(_T("\0")),
    ActionInterface::on_key_BestAlternate,
    36, 46,
  },
  // 70
  { ugAltitude, _T("QFE GPS"), _T("QFE GPS"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    1, 33,
  },
  // 71 TODO FIX those 19,4 values
  { ugNone, _T("L/D Average"), _T("L/D Avg"),
    new InfoBoxFormatter(_T("%2.0f")),
    ActionInterface::on_key_None,
    19, 4,
  },
  // 72 //
  { ugNone, _T("Experimental1"), _T("Exp1"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
  // 73 //
  { ugDistance, _T("Online Contest Distance"), _T("OLC"),
    new InfoBoxFormatter(_T("%2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
  // 74 //
  { ugNone, _T("Experimental2"), _T("Exp2"),
    new InfoBoxFormatter(_T("%-2.1f")),
    ActionInterface::on_key_None,
    8, 2,
  },
};

const unsigned NUMSELECTSTRINGS = 75;

static InfoBoxLook info_box_look;

// TODO locking
void
InfoBoxManager::Hide()
{
  InfoBoxesHidden = true;

  for (unsigned i = 0; i < numInfoWindows; i++) {
    InfoBoxes[i]->hide();
  }

  full_window.hide();
}


void
InfoBoxManager::Show()
{
  InfoBoxesHidden = false;

  for (unsigned i = 0; i < numInfoWindows; i++) {
    InfoBoxes[i]->show();
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

void
InfoBoxManager::Event_Select(int i)
{
  int InfoFocus = GetFocused();

  if (InfoFocus < 0) {
    InfoFocus = (i >= 0 ? 0 : numInfoWindows - 1);
  } else {
    InfoFocus += i;

    if (InfoFocus < 0 || (unsigned)InfoFocus >= numInfoWindows)
      InfoFocus = -1;
  }

  if (InfoFocus >= 0) {
    main_window.map.set_focus();
  } else {
    InfoBoxes[i]->set_focus();
  }
}

int
InfoBoxManager::getType(unsigned i, unsigned layer)
{
  assert(i < MAXINFOWINDOWS);
  assert(layer < 4);

  switch (layer) {
  case 0:
    return InfoType[i] & 0xff; // climb
  case 1:
    return (InfoType[i] >> 8) & 0xff; // cruise
  case 2:
    return (InfoType[i] >> 16) & 0xff; // final glide
  case 3:
    return (InfoType[i] >> 24) & 0xff; // auxiliary
  }

  return 0xdeadbeef; /* not reachable */
}

int
InfoBoxManager::getTypeAll(unsigned i)
{
  assert(i < MAXINFOWINDOWS);

  return InfoType[i];
}

void
InfoBoxManager::setTypeAll(unsigned i, unsigned j)
{
  assert(i < MAXINFOWINDOWS);

  InfoType[i] = j;
  // TODO: check it's within range
}

int
InfoBoxManager::getType(unsigned i)
{
  unsigned retval = 0;

  if (SettingsMap().EnableAuxiliaryInfo) {
    retval = getType(i, 3);
  } else {
    if (MapProjection().GetDisplayMode() == dmCircling)
      retval = getType(i, 0);
    else if (MapProjection().GetDisplayMode() == dmFinalGlide) {
      retval = getType(i, 2);
    } else {
      retval = getType(i, 1); // cruise
    }
  }

  return min(NUMSELECTSTRINGS - 1, retval);
}

bool
InfoBoxManager::IsEmpty(unsigned mode)
{
  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i)
    if (InfoBoxManager::getType(i, mode) != 0)
      return false;

  return true;
}

bool
InfoBoxManager::IsEmpty()
{
  for (unsigned i = 0; i < MAXINFOWINDOWS; ++i)
    if (InfoBoxManager::getTypeAll(i) != 0)
      return false;

  return true;
}

void
InfoBoxManager::setType(unsigned i, char j, unsigned layer)
{
  assert(i < MAXINFOWINDOWS);

  switch (layer) {
  case 0:
    InfoType[i] &= 0xffffff00;
    InfoType[i] += j;
    break;
  case 1:
    InfoType[i] &= 0xffff00ff;
    InfoType[i] += (j << 8);
    break;
  case 2:
    InfoType[i] &= 0xff00ffff;
    InfoType[i] += (j << 16);
    break;
  case 3:
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j << 24);
    break;
  }
}

void
InfoBoxManager::setType(unsigned i, char j)
{
  if (SettingsMap().EnableAuxiliaryInfo) {
    setType(i, j, 3);
  } else {
    if (MapProjection().GetDisplayMode() == dmCircling) {
      setType(i, j, 0);
    } else if (MapProjection().GetDisplayMode() == dmFinalGlide) {
      setType(i, j, 2);
    } else {
      setType(i, j, 1);
    }
  }
}

void
InfoBoxManager::Event_Change(int i)
{
  int j = 0, k;

  int InfoFocus = GetFocused();
  if (InfoFocus < 0)
    return;

  k = getType(InfoFocus);
  if (i > 0)
    j = Data_Options[k].next_screen;
  else if (i < 0)
    j = Data_Options[k].prev_screen;

  // TODO code: if i==0, go to default or reset

  setType(InfoFocus, j);

  Update(*InfoBoxes[InfoFocus], j, true);
  Paint();
}

void
InfoBoxManager::Update(InfoBox &info_box, unsigned type, bool needupdate)
{
  TCHAR sTmp[32];
  int color = 0;

  //
  // Set Infobox title and middle value. Bottom line comes next
  //
  switch (type) {
  case 67: // VENTA3 alternate1 and 2
  case 68:
  case 69:
    if (type == 67)
      ActiveAlternate = SettingsComputer().Alternate1;
    else if (type == 68)
      ActiveAlternate = SettingsComputer().Alternate2;
#ifdef OLD_TASK // alternates
    else
      ActiveAlternate = Calculated().BestAlternate;
#endif

    info_box.SetSmallerFont(false);

    if (ActiveAlternate != -1) {
      info_box.SetTitle(Data_Options[type].Formatter->RenderTitle(&color));
      info_box.SetColor(color);
      info_box.SetValue(Data_Options[type].Formatter->Render(&color));
      info_box.SetColor(color);
    } else {
      if (type == 67)
        info_box.SetTitle(_T("Altern1"));
      else if (type == 68)
        info_box.SetTitle(_T("Altern2"));
      else
        info_box.SetTitle(_T("BestAltr"));

      info_box.SetValue(_T("---"));
      info_box.SetColor(-1);
    }

    if (needupdate)
      info_box.SetValueUnit(Units::GetUserUnitByGroup(Data_Options[type].UnitGroup));

    break;

  case 55:
    info_box.SetSmallerFont(true);

    if (needupdate)
      info_box.SetTitle(Data_Options[type].Title);

    info_box.SetValue(Data_Options[type].Formatter->Render(&color));

    // to be optimized!
    if (needupdate)
      info_box.SetValueUnit(Units::GetUserUnitByGroup(Data_Options[type].UnitGroup));

    info_box.SetColor(color);

    break;

  case 14: // Next waypoint
    info_box.SetSmallerFont(false);

    if (Calculated().task_stats.task_valid) {
      info_box.SetTitle(
                              Data_Options[type].Formatter-> Render(&color));
      info_box.SetColor(color);
      info_box.SetValue(Data_Options[47].Formatter->Render(&color));
    } else {
      info_box.SetTitle(_T("Next"));
      info_box.SetValue(_T("---"));
      info_box.SetColor(-1);
    }

    if (needupdate)
      info_box.SetValueUnit(Units::GetUserUnitByGroup(Data_Options[type].UnitGroup));
    break;

  default:
    info_box.SetSmallerFont(false);

    if (needupdate)
      info_box.SetTitle(Data_Options[type].Title);

    info_box.SetValue(Data_Options[type].Formatter->Render(&color));

    // to be optimized!
    if (needupdate)
      info_box.SetValueUnit(Units::GetUserUnitByGroup(Data_Options[type].UnitGroup));

    info_box.SetColor(color);
  }

  //
  // Infobox bottom line
  //
  switch (type) {
  case 14: // Next waypoint
    if (const Waypoint* way_point = task_ui.getActiveWaypoint()) {
      info_box.SetComment(way_point->Comment.c_str());
    } else {
      info_box.SetComment(_T(""));
    }
    break;

  case 10:
    if (SettingsComputer().auto_mc)
      info_box.SetComment(_T("AUTO"));
    else
      info_box.SetComment(_T("MANUAL"));
    break;

  case 0: // GPS Alt
    Units::FormatAlternateUserAltitude(Basic().GPSAltitude, sTmp, sizeof(sTmp)
                                       / sizeof(sTmp[0]));
    info_box.SetComment(sTmp);
    break;

  case 1: // AGL
    Units::FormatAlternateUserAltitude(Basic().AltitudeAGL, sTmp,
                                       sizeof(sTmp) / sizeof(sTmp[0]));
    info_box.SetComment(sTmp);
    break;

  case 33:
    Units::FormatAlternateUserAltitude(Basic().BaroAltitude, sTmp,
                                       sizeof(sTmp) / sizeof(sTmp[0]));
    info_box.SetComment(sTmp);
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
    if (Data_Options[type].Formatter->isValid()) {
      info_box.SetComment(Data_Options[type].Formatter->GetCommentText());
    } else {
      info_box.SetComment(_T(""));
    }
    break;

  case 43:
    if (SettingsComputer().EnableBlockSTF) {
      info_box.SetComment(_T("BLOCK"));
    } else {
      info_box.SetComment(_T("DOLPHIN"));
    }
    break;

  case 55: // own team code
    info_box.SetComment(Calculated().TeammateCode);

    if (SettingsComputer().TeamFlarmTracking) {
      if (Basic().flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget) != NULL) {
        info_box.SetColorBottom(2);
      } else {
        info_box.SetColorBottom(1);
      }
    } else {
      info_box.SetColorBottom(0);
    }
    break;

  case 56: // team bearing
    if (SettingsComputer().TeamFlarmIdTarget.defined()) {
      if (!string_is_empty(SettingsComputer().TeamFlarmCNTarget)) {
        info_box.SetComment(SettingsComputer().TeamFlarmCNTarget);
      } else {
        info_box.SetComment(_T("???"));
      }
    } else {
      info_box.SetComment(_T("---"));
    }

    if (Basic().flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget) != NULL) {
      info_box.SetColorBottom(2);
    } else {
      info_box.SetColorBottom(1);
    }
    break;

  case 57: // team bearing dif
    if (SettingsComputer().TeamFlarmIdTarget.defined()) {
      if (!string_is_empty(SettingsComputer().TeamFlarmCNTarget)) {
        info_box.SetComment(SettingsComputer().TeamFlarmCNTarget);
      } else {
        info_box.SetComment(_T("???"));
      }
    } else {
      info_box.SetComment(_T("---"));
    }

    if (Basic().flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget) != NULL) {
      info_box.SetColorBottom(2);
    } else {
      info_box.SetColorBottom(1);
    }
    break;

  case 58: // team range
    if (SettingsComputer().TeamFlarmIdTarget.defined()) {
      if (!string_is_empty(SettingsComputer().TeamFlarmCNTarget)) {
        info_box.SetComment(SettingsComputer().TeamFlarmCNTarget);
      } else {
        info_box.SetComment(_T("???"));
      }
    } else {
      info_box.SetComment(_T("---"));
    }

    if (Basic().flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget) != NULL) {
      info_box.SetColorBottom(2);
    } else {
      info_box.SetColorBottom(1);
    }
    break;

    // VENTA3 wind speed + bearing bottom line
  case 25:
    _stprintf(sTmp, _T("%d%s"), (int)Basic().wind.bearing.value_degrees(), 
              _T(DEG));
    info_box.SetComment(sTmp);
    break;

    // VENTA3 radial
  case 60:
    _stprintf(sTmp, _T("%d%s"),
              (int)Calculated().common_stats.vector_home.Bearing.value_degrees(), 
              _T(DEG));
    info_box.SetComment(sTmp);
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
    if (PDABatteryTemperature > 0) {
      _stprintf(sTmp, _T("%1.0d%SC"), (int)PDABatteryTemperature, _T(DEG));
      info_box.SetComment(sTmp);
    } else
      info_box.SetComment(_T(""));
    break;
#endif

    // VENTA3 alternates
  case 67:
  case 68:
  case 69:
    if (ActiveAlternate == -1) {
      // should be redundant
      info_box.SetComment(_T(""));
      break;
    }
#ifdef OLD_TASK // alternates
    if (FlipBoxValue == true) {
      Units::FormatUserDistance(way_points.get_calc(ActiveAlternate).Distance,
                                sTmp, sizeof(sTmp) / sizeof(sTmp[0]));
      info_box.SetComment(sTmp);
    } else {
      Units::FormatUserArrival(way_points.get_calc(ActiveAlternate).AltArrival,
                               sTmp, sizeof(sTmp) / sizeof(sTmp[0]));
      info_box.SetComment(sTmp);
    }
#endif
    break;

  case 70: // QFE
    /*
    // Showing the diff value offset was just interesting ;-)
    if (FlipBoxValue == true) {
      //Units::FormatUserArrival(QFEAltitudeOffset,
      Units::FormatUserAltitude(QFEAltitudeOffset,
                                sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
                                info_box.SetComment(sTmp);
    } else {
    */

    Units::FormatUserAltitude(Basic().GPSAltitude,
                              sTmp, sizeof(sTmp) / sizeof(sTmp[0]));
    info_box.SetComment(sTmp);
    break;

  default:
    info_box.SetComment(_T(""));
  }
}

void
InfoBoxManager::DisplayInfoBox(void)
{
  if (InfoBoxesHidden)
    return;

  static int DisplayType[MAXINFOWINDOWS];
  static bool first = true;
  static int DisplayTypeLast[MAXINFOWINDOWS];
  static bool FlipBoxValue = false;

  // VENTA3 - Dynamic box values
  if (LastFlipBoxTime > DYNABOXTIME) {
    FlipBoxValue = (FlipBoxValue == false);
    LastFlipBoxTime = 0;
  }

  // JMW note: this is updated every GPS time step

  for (unsigned i = 0; i < numInfoWindows; i++) {
    // VENTA3
    // All calculations are made in a separate thread. Slow calculations should apply to
    // the function DoCalculationsSlow() . Do not put calculations here!

    DisplayType[i] = getType(i);
    Data_Options[DisplayType[i]].Formatter->AssignValue(DisplayType[i]);

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i]) || first);

    Update(*InfoBoxes[i], DisplayType[i], needupdate);

    DisplayTypeLast[i] = DisplayType[i];
  }
  Paint();

  first = false;
}

void
InfoBoxManager::ProcessKey(int keycode)
{
  unsigned i;

  int InfoFocus = GetFocused();
  if (InfoFocus < 0)
    return; // paranoid

  InputEvents::HideMenu();

  i = getType(InfoFocus);
  Data_Options[min(NUMSELECTSTRINGS - 1, i)].Process(keycode);

  InfoBoxesDirty = true;

  TriggerGPSUpdate(); // emulate update to trigger calculations

  ResetDisplayTimeOut();
}

void
InfoBoxManager::DestroyInfoBoxFormatters()
{
  //  CommandBar_Destroy(hWndCB);
  for (unsigned i = 0; i < NUMSELECTSTRINGS; i++) {
    delete Data_Options[i].Formatter;
  }
}

bool
InfoBoxManager::IsFocus()
{
  return GetFocused() >= 0;
}

void
InfoBoxManager::InfoBoxDrawIfDirty(void)
{
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (InfoBoxesDirty && !SettingsMap().ScreenBlanked) {
    DisplayInfoBox();
    InfoBoxesDirty = false;
  }
}

void
InfoBoxManager::SetDirty(bool is_dirty)
{
  InfoBoxesDirty = is_dirty;
}

void
InfoBoxManager::ProcessTimer(void)
{
  static fixed lasttime;

  if (Basic().Time != lasttime) {
    SetDirty(true);
    lasttime = Basic().Time;
  }
  InfoBoxDrawIfDirty();
  LastFlipBoxTime++;
}

void InfoBoxManager::ResetInfoBoxes(void) {
  memcpy(InfoType,
         is_altair() ? InfoTypeAltairDefault : InfoTypeDefault,
         sizeof(InfoType));
}

const TCHAR *
InfoBoxManager::GetTypeDescription(unsigned i)
{
  return Data_Options[i].Description;
}

// TODO: this should go into the manager
extern InfoBox *InfoBoxes[MAXINFOWINDOWS];

void
InfoBoxManager::Paint(void)
{
  unsigned i;

  if (!InfoBoxLayout::fullscreen) {
    full_window.hide();

    for (i = 0; i < numInfoWindows; i++)
      InfoBoxes[i]->invalidate();
  } else {
    Canvas &canvas = full_window.get_canvas();

    canvas.white_brush();
    canvas.white_pen();
    canvas.clear();

    for (i = 0; i < numInfoWindows; i++) {
      // JMW TODO: make these calculated once only.
      int x, y;
      int rx, ry;
      int rw;
      int rh;
      double fw, fh;

      if (Layout::landscape) {
        rw = 84;
        rh = 68;
      } else {
        rw = 120;
        rh = 80;

      }
      fw = rw / (double)InfoBoxLayout::ControlWidth;
      fh = rh / (double)InfoBoxLayout::ControlHeight;
      double f = min(fw, fh);
      rw = (int)(f * InfoBoxLayout::ControlWidth);
      rh = (int)(f * InfoBoxLayout::ControlHeight);

      if (Layout::landscape) {
        rx = i % 3;
        ry = i / 3;

        x = (rw + 4) * rx;
        y = (rh + 3) * ry;
      } else {
        rx = i % 2;
        ry = i / 4;

        x = (rw) * rx;
        y = (rh) * ry;
      }

      InfoBoxes[i]->PaintInto(canvas, IBLSCALE(x), IBLSCALE(y),
                              IBLSCALE(rw), IBLSCALE(rh));
    }

    full_window.invalidate();
    full_window.show();
  }
}

void
InfoBoxManager::Create(RECT rc)
{
  ResetInfoBoxes();

  info_box_look.value.fg_color = info_box_look.title.fg_color
    = info_box_look.comment.fg_color = Appearance.InverseInfoBox
    ? Color::WHITE : Color::BLACK;
  info_box_look.value.bg_color = info_box_look.title.bg_color
    = info_box_look.comment.bg_color = Appearance.InverseInfoBox
    ? Color::BLACK : Color::WHITE;

  Color border_color = Color(80, 80, 80);
  info_box_look.border_pen.set(InfoBox::BORDER_WIDTH, border_color);
  info_box_look.selector_pen.set(IBLSCALE(1) + 2,
                                 info_box_look.value.fg_color);

  info_box_look.value.font = &InfoWindowFont;
  info_box_look.title.font = &TitleWindowFont;
  info_box_look.comment.font = &TitleWindowFont;
  info_box_look.small_font = &TitleSmallWindowFont;

  info_box_look.colors[0] = border_color;
  info_box_look.colors[1] = Appearance.InverseInfoBox
    ? MapGfx.inv_redColor : Color::RED;
  info_box_look.colors[2] = Appearance.InverseInfoBox
    ? MapGfx.inv_blueColor : Color::BLUE;
  info_box_look.colors[3] = Appearance.InverseInfoBox
    ? MapGfx.inv_greenColor : Color::GREEN;
  info_box_look.colors[4] = Appearance.InverseInfoBox
    ? MapGfx.inv_yellowColor : Color::YELLOW;
  info_box_look.colors[5] = Appearance.InverseInfoBox
    ? MapGfx.inv_magentaColor : Color::MAGENTA;

  WindowStyle style;
  style.hide();
  full_window.set(main_window, rc.left, rc.right,
                  rc.right - rc.left, rc.bottom - rc.top);

  // create infobox windows
  for (unsigned i = 0; i < numInfoWindows; i++) {
    int xoff, yoff, sizex, sizey;
    InfoBoxLayout::GetInfoBoxPosition(i, rc, &xoff, &yoff, &sizex, &sizey);

    int Border;
    if (Appearance.InfoBoxBorder == apIbTab)
      Border = 0;
    else if (InfoBoxLayout::gnav) {
      Border = 0;
      if (i > 0)
        Border |= BORDERTOP;
      if (i < 6)
        Border |= BORDERRIGHT;
    } else if (!Layout::landscape) {
      Border = 0;
      if (i < 4)
        Border |= BORDERBOTTOM;
      else
        Border |= BORDERTOP;

      Border |= BORDERRIGHT;
    } else
      Border = BORDERRIGHT | BORDERBOTTOM;

    InfoBoxes[i] = new InfoBox(main_window, xoff, yoff, sizex, sizey,
                               Border, info_box_look);
  }
}

void
InfoBoxManager::Destroy(void)
{
  for (unsigned i = 0; i < numInfoWindows; i++) {
    delete (InfoBoxes[i]);
  }

  full_window.reset();

  DestroyInfoBoxFormatters();
}
