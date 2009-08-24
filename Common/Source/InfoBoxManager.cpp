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
#include "XCSoar.h"
#include "Protection.hpp"
#include "InfoBox.h"
#include "InfoBoxLayout.h"
#include "InfoBoxEvents.h"
#include "Formatter/TeamCode.hpp"
#include "Formatter/WayPoint.hpp"
#include "Formatter/LowWarning.hpp"
#include "Formatter/Time.hpp"
#include "Dialogs.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "MapWindow.h"
#include "Device/Parser.h"
#include "Utils.h"
#include "Utils2.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "Registry.hpp"
#include "Screen/Blank.hpp"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Interface.hpp"

#include "Calculations.h" // TODO danger! IsFlarmTargetCNInRange

extern bool DialogActive;
int  InfoBoxFocusTimeOut = 0;
bool InfoBoxesDirty= false;
static bool InfoBoxesHidden = false;
int InfoFocus = 0;
bool InfoWindowActive = true;
bool EnableAuxiliaryInfo = false;
double LastFlipBoxTime = 0; // VENTA3 need this global for slowcalculations cycle

int ActiveAlternate = -1;

// fwd declarations
void DisplayText(void);
void AssignValues(void);

// from XCSoar.cpp
void SwitchToMapWindow(void);


void PopupAnalysis()
{
  DialogActive = true;
  dlgAnalysisShowModal();
  DialogActive = false;
}


void PopupWaypointDetails()
{
  dlgWayPointDetailsShowModal();
}


void PopupBugsBallast(int UpDown)
{
	(void)UpDown;
  DialogActive = true;
  //  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}



int numInfoWindows = 8;

InfoBox *InfoBoxes[MAXINFOWINDOWS];

int                                     InfoType[MAXINFOWINDOWS] =
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
	  {ugAltitude,        TEXT("Height GPS"), TEXT("H GPS"), new InfoBoxFormatter(TEXT("%2.0f")), AltitudeProcessing, 1, 33},
	  // 1
	  {ugAltitude,        TEXT("Height AGL"), TEXT("H AGL"), new FormatterLowWarning(TEXT("%2.0f"),0.0), NoProcessing, 20, 0},
	  // 2
	  {ugVerticalSpeed,   TEXT("Thermal last 30 sec"), TEXT("TC 30s"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), NoProcessing, 7, 44},
	  // 3
#ifdef FIVV
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), NoProcessing, 6, 54},
#else
	  {ugNone,            TEXT("Bearing"), TEXT("Bearing"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), NoProcessing, 6, 54},
#endif
	  // 4
	  {ugNone,            TEXT("L/D instantaneous"), TEXT("L/D Inst"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 5, 38},
	  // 5
	  {ugNone,            TEXT("L/D cruise"), TEXT("L/D Cru"), new InfoBoxFormatter(TEXT("%2.0f")), PopupBugsBallast, 19, 4},
	  // 6
	  {ugHorizontalSpeed, TEXT("Speed ground"), TEXT("V Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), SpeedProcessing, 23, 3},
	  // 7
	  {ugVerticalSpeed,   TEXT("Last Thermal Average"), TEXT("TL Avg"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 8
	  {ugAltitude,        TEXT("Last Thermal Gain"), TEXT("TL Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 9, 7},
	  // 9
	  {ugNone,            TEXT("Last Thermal Time"), TEXT("TL Time"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 21, 8},
	  // 10
	  {ugVerticalSpeed,   TEXT("MacCready Setting"), TEXT("MacCready"), new InfoBoxFormatter(TEXT("%2.1f")), MacCreadyProcessing, 34, 43},
	  // 11
	  {ugDistance,        TEXT("Next Distance"), TEXT("WP Dist"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 12, 31},
	  // 12
	  {ugAltitude,        TEXT("Next Altitude Difference"), TEXT("WP AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 13, 11},
	  // 13
	  {ugAltitude,        TEXT("Next Altitude Required"), TEXT("WP AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 15, 12},
	  // 14
	  {ugNone,            TEXT("Next Waypoint"), TEXT("Next"), new FormatterWaypoint(TEXT("\0")), NextUpDown, 36, 46},
	  // 15
	  {ugAltitude,        TEXT("Final Altitude Difference"), TEXT("Fin AltD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 16, 13},
	  // 16
	  {ugAltitude,        TEXT("Final Altitude Required"), TEXT("Fin AltR"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 17, 15},
	  // 17
	  {ugTaskSpeed, TEXT("Speed Task Average"), TEXT("V Task Av"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
	  // 18
	  {ugDistance,        TEXT("Final Distance"), TEXT("Fin Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 27, 17},
	  // 19
	  {ugNone,            TEXT("Final LD"), TEXT("Fin LD"), new InfoBoxFormatter(TEXT("%1.0f")), NoProcessing, 38, 5},
	  // 20
	  {ugAltitude,        TEXT("Terrain Elevation"), TEXT("H Gnd"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 33, 1},
	  // 21
	  {ugVerticalSpeed,   TEXT("Thermal Average"), TEXT("TC Avg"), new FormatterLowWarning(TEXT("%-2.1f"),0.0), NoProcessing, 22, 9},
	  // 22
	  {ugAltitude,        TEXT("Thermal Gain"), TEXT("TC Gain"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 24, 21},
	  // 23
#ifdef FIVV
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), DirectionProcessing, 32, 6},
#else
	  {ugNone,            TEXT("Track"), TEXT("Track"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), DirectionProcessing, 32, 6},
#endif
	  // 24
	  {ugVerticalSpeed,   TEXT("Vario"), TEXT("Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 44, 22},
	  // 25
	  {ugWindSpeed,       TEXT("Wind Speed"), TEXT("Wind V"), new InfoBoxFormatter(TEXT("%2.0f")), WindSpeedProcessing, 26, 50},
	  // 26
#ifdef FIVV
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), WindDirectionProcessing, 48, 25},
#else
	  {ugNone,            TEXT("Wind Bearing"), TEXT("Wind B"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), WindDirectionProcessing, 48, 25},
#endif
	  // 27
	  {ugNone,            TEXT("AA Time"), TEXT("AA Time"), new FormatterAATTime(TEXT("%2.0f")), NoProcessing, 28, 18},
	  // 28
	  {ugDistance,        TEXT("AA Distance Max"), TEXT("AA Dmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 29, 27},
	  // 29
	  {ugDistance,        TEXT("AA Distance Min"), TEXT("AA Dmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 30, 28},
	  // 30
	  {ugTaskSpeed, TEXT("AA Speed Max"), TEXT("AA Vmax"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 31, 29},
	  // 31
	  {ugTaskSpeed, TEXT("AA Speed Min"), TEXT("AA Vmin"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 51, 30},
	  // 32
	  {ugHorizontalSpeed, TEXT("Airspeed IAS"), TEXT("V IAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 37, 23},
	  // 33
	  {ugAltitude,        TEXT("Pressure Altitude"), TEXT("H Baro"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 0, 20},
	  // 34
	  {ugHorizontalSpeed, TEXT("Speed MacReady"), TEXT("V Mc"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 35, 10},
	  // 35
	  {ugNone,            TEXT("Percentage climb"), TEXT("% Climb"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 43, 34},
	  // 36
	  {ugNone,            TEXT("Time of flight"), TEXT("Time flt"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 39, 14},
	  // 37
	  {ugNone,            TEXT("G load"), TEXT("G"), new InfoBoxFormatter(TEXT("%2.2f")), AccelerometerProcessing, 47, 32},
	  // 38
	  {ugNone,            TEXT("Next LD"), TEXT("WP LD"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 53, 19},
	  // 39
	  {ugNone,            TEXT("Time local"), TEXT("Time loc"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 40, 36},
	  // 40
	  {ugNone,            TEXT("Time UTC"), TEXT("Time UTC"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 41, 39},
	  // 41
	  {ugNone,            TEXT("Task Time To Go"), TEXT("Fin ETE"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 42, 40},
	  // 42
	  {ugNone,            TEXT("Next Time To Go"), TEXT("WP ETE"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 45, 41},
	  // 43
	  {ugHorizontalSpeed, TEXT("Speed Dolphin"), TEXT("V Opt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 10, 35},
	  // 44
	  {ugVerticalSpeed,   TEXT("Netto Vario"), TEXT("Netto"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 2, 24},
	  // 45
	  {ugNone,            TEXT("Task Arrival Time"), TEXT("Fin ETA"), new FormatterAATTime(TEXT("%04.0f")), NoProcessing, 46, 42},
	  // 46
	  {ugNone,            TEXT("Next Arrival Time"), TEXT("WP ETA"), new FormatterTime(TEXT("%04.0f")), NoProcessing, 14, 45},
	  // 47
	  {ugNone,            TEXT("Bearing Difference"), TEXT("Brng D"), new FormatterDiffBearing(TEXT("")), NoProcessing, 54, 37},
	  // 48
	  {ugNone,            TEXT("Outside Air Temperature"), TEXT("OAT"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), NoProcessing, 49, 26},
	  // 49
	  {ugNone,            TEXT("Relative Humidity"), TEXT("RelHum"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 50, 48},
	  // 50
	  {ugNone,            TEXT("Forecast Temperature"), TEXT("MaxTemp"), new InfoBoxFormatter(TEXT("%2.1f")TEXT(DEG)), ForecastTemperatureProcessing, 49, 25},
	  // 51
	  {ugDistance,        TEXT("AA Distance Tgt"), TEXT("AA Dtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 52, 31},
	  // 52
	  {ugTaskSpeed, TEXT("AA Speed Tgt"), TEXT("AA Vtgt"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 11, 51},
	  // 53
	  {ugNone,            TEXT("L/D vario"), TEXT("L/D vario"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 4, 38},
	  // 54
	  {ugHorizontalSpeed, TEXT("Airspeed TAS"), TEXT("V TAS"), new InfoBoxFormatter(TEXT("%2.0f")), AirspeedProcessing, 3, 47},
	  // 55
	  {ugNone,            TEXT("Own Team Code"), TEXT("TeamCode"), new FormatterTeamCode(TEXT("\0")), TeamCodeProcessing, 56, 54},
	  // 56
#ifdef FIVV
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)), NoProcessing, 57, 55},
#else
	  {ugNone,            TEXT("Team Bearing"), TEXT("Tm Brng"), new InfoBoxFormatter(TEXT("%2.0f")TEXT(DEG)TEXT("T")), NoProcessing, 57, 55},
#endif
	  // 57
	  {ugNone,            TEXT("Team Bearing Diff"), TEXT("Team Bd"), new FormatterDiffTeamBearing(TEXT("")), NoProcessing, 58, 56},
	  // 58
	  {ugNone,            TEXT("Team Range"), TEXT("Team Dis"), new InfoBoxFormatter(TEXT("%2.1f")), NoProcessing, 55, 57},
          // 59
	  {ugTaskSpeed, TEXT("Speed Task Instantaneous"), TEXT("V Tsk Ins"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
          // 60
	  {ugDistance, TEXT("Distance Home"), TEXT("Home Dis"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
	  // 61
	  {ugTaskSpeed, TEXT("Speed Task Achieved"), TEXT("V Tsk Ach"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 18, 16},
          // 62
	  {ugNone,            TEXT("AA Delta Time"), TEXT("AA dT"), new FormatterAATTime(TEXT("%2.0f")), NoProcessing, 28, 18},
          // 63
	  {ugVerticalSpeed,   TEXT("Thermal All"), TEXT("TC All"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
          // 64
	  {ugVerticalSpeed,   TEXT("Distance Vario"), TEXT("D Vario"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 65
#ifndef GNAV
	  {ugNone,            TEXT("Battery Percent"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.0f%%")), NoProcessing, 49, 26},
#else
	  {ugNone,            TEXT("Battery Voltage"), TEXT("Battery"), new InfoBoxFormatter(TEXT("%2.1fV")), NoProcessing, 49, 26},
#endif
	  // 66  VENTA-ADDON added Final GR
	  // VENTA-TODO: fix those 38,5 numbers to point correctly menu items
	  {ugNone,            TEXT("Final GR"), TEXT("Fin GR"), new InfoBoxFormatter(TEXT("%1.1f")), NoProcessing, 38, 5},

	  // 67 VENTA3-ADDON Alternate1 destinations infoboxes  TODO> fix 36 46 to something correct
	  {ugNone,            TEXT("Alternate1 GR"), TEXT("Altern 1"), new FormatterAlternate(TEXT("\0")), Alternate1Processing, 36, 46},
	  // 68 Alternate 2
	  {ugNone,            TEXT("Alternate2 GR"), TEXT("Altern 2"), new FormatterAlternate(TEXT("\0")), Alternate2Processing, 36, 46},
	  // 69 BestAlternate aka BestLanding
	  {ugNone,            TEXT("Best Alternate"), TEXT("BestAltn"), new FormatterAlternate(TEXT("\0")), BestAlternateProcessing, 36, 46},
          // 70
	  {ugAltitude,        TEXT("QFE GPS"), TEXT("QFE GPS"), new InfoBoxFormatter(TEXT("%2.0f")), QFEAltitudeProcessing, 1, 33},
          // 71 TODO FIX those 19,4 values
	  {ugNone,            TEXT("L/D Average"), TEXT("L/D Avg"), new InfoBoxFormatter(TEXT("%2.0f")), NoProcessing, 19, 4},
	  // 72 //
	  {ugNone,   TEXT("Experimental1"), TEXT("Exp1"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	  // 73 //
	  {ugNone,   TEXT("Experimental2"), TEXT("Exp2"), new InfoBoxFormatter(TEXT("%-2.1f")), NoProcessing, 8, 2},
	};

const int NUMSELECTSTRINGS = 74;


/////////////////

void HideMenu();


void HideInfoBoxes() {
  int i;
  InfoBoxesHidden = true;
  for (i=0; i<numInfoWindows+1; i++) {
    InfoBoxes[i]->SetVisible(false);
  }
}


void ShowInfoBoxes() {
  int i;
  InfoBoxesHidden = false;
  for (i=0; i<numInfoWindows; i++) {
    InfoBoxes[i]->SetVisible(true);
  }
}

void Event_SelectInfoBox(int i) {
//  int oldinfofocus = InfoFocus;

  // must do this
  InfoBoxFocusTimeOut = 0;

  if (InfoFocus>= 0) {
    FocusOnWindow(InfoFocus,false);
  }
  InfoFocus+= i;
  if (InfoFocus>=numInfoWindows) {
    InfoFocus = -1; // deactivate if wrap around
  }
  if (InfoFocus<0) {
    InfoFocus = -1; // deactivate if wrap around
  }
  if (InfoFocus<0) {
    DefocusInfoBox();
    SwitchToMapWindow();
    return;
  }

  //  SetFocus(hWndInfoWindow[InfoFocus]);
  FocusOnWindow(InfoFocus,true);
  InfoWindowActive = true;
  DisplayText();

  InputEvents::setMode(TEXT("infobox"));
}


////////////////////////////////////

static int getInfoType(int i) {
  int retval = 0;
  if (i<0) return 0; // error

  if (EnableAuxiliaryInfo) {
    retval = (InfoType[i] >> 24) & 0xff; // auxiliary
  } else {
    if (DisplayMode == dmCircling)
      retval = InfoType[i] & 0xff; // climb
    else if (DisplayMode == dmFinalGlide) {
      retval = (InfoType[i] >> 16) & 0xff; //final glide
    } else {
      retval = (InfoType[i] >> 8) & 0xff; // cruise
    }
  }
  return min(NUMSELECTSTRINGS-1,retval);
}


static void setInfoType(int i, char j) {
  if (i<0) return; // error

  if (EnableAuxiliaryInfo) {
    InfoType[i] &= 0x00ffffff;
    InfoType[i] += (j<<24);
  } else {
    if (DisplayMode == dmCircling) {
      InfoType[i] &= 0xffffff00;
      InfoType[i] += (j);
    } else if (DisplayMode == dmFinalGlide) {
      InfoType[i] &= 0xff00ffff;
      InfoType[i] += (j<<16);
    } else {
      InfoType[i] &= 0xffff00ff;
      InfoType[i] += (j<<8);
    }
  }
}


void Event_ChangeInfoBoxType(int i) {
  int j=0, k;

  if (InfoFocus<0) {
    return;
  }

  k = getInfoType(InfoFocus);
  if (i>0) {
    j = Data_Options[k].next_screen;
  }
  if (i<0) {
    j = Data_Options[k].prev_screen;
  }

  // TODO code: if i==0, go to default or reset

  setInfoType(InfoFocus, j);
  DisplayText();

}

////////////////////

void DefocusInfoBox() {
  FocusOnWindow(InfoFocus,false);
  InfoFocus = -1;
  if (MapWindow::isPan() && !MapWindow::isTargetPan()) {
    InputEvents::setMode(TEXT("pan"));
  } else {
    InputEvents::setMode(TEXT("default"));
  }
  InfoWindowActive = false;
}


void FocusOnWindow(int i, bool selected) {
    //hWndTitleWindow

  if (i<0) return; // error

  InfoBoxes[i]->SetFocus(selected);
  // todo defocus all other?

}


void    AssignValues(void)
{
  if (InfoBoxesHidden) {
    // no need to assign values
    return;
  }

  //  DetectStartTime(); moved to Calculations

  // nothing to do here now!
}

void DisplayText(void)
{
  if (InfoBoxesHidden)
    return;

  int i;
  static int DisplayType[MAXINFOWINDOWS];
  static bool first=true;
  static int InfoFocusLast = -1;
  static int DisplayTypeLast[MAXINFOWINDOWS];
  static bool FlipBoxValue = false;

  // VENTA3 - Dynamic box values
  if (LastFlipBoxTime > DYNABOXTIME ) {
    FlipBoxValue = ( FlipBoxValue == false );
    LastFlipBoxTime = 0;
  }

  LockNavBox();

  // JMW note: this is updated every GPS time step

  if (InfoFocus != InfoFocusLast) {
    first = true; // force re-setting title
  }
  if ((InfoFocusLast>=0)&&(!InfoWindowActive)) {
    first = true;
  }
  InfoFocusLast = InfoFocus;

  for(i=0;i<numInfoWindows;i++) {

    // VENTA3
    // All calculations are made in a separate thread. Slow calculations should apply to
    // the function DoCalculationsSlow() . Do not put calculations here!

    DisplayType[i] = getInfoType(i);
    Data_Options[DisplayType[i]].Formatter->AssignValue(DisplayType[i]);

    TCHAR sTmp[32];

    int color = 0;

    bool needupdate = ((DisplayType[i] != DisplayTypeLast[i])||first);

    int theactive = ActiveWayPoint;
    if (!ValidTaskPoint(theactive)) {
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
	  ActiveAlternate=Alternate1;
	else
	  if (DisplayType[i]==68)
	    ActiveAlternate=Alternate2;
	  else
	    ActiveAlternate=BestAlternate;

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
        int index;
        index = Task[theactive].Index;
        if (index>=0) {
          InfoBoxes[i]->
            SetComment(WayPointList[index].Comment);
        }
        break;
      }
      InfoBoxes[i]->SetComment(TEXT(""));
      break;
    case 10:
      if (CALCULATED_INFO.AutoMacCready)
	InfoBoxes[i]->SetComment(TEXT("AUTO"));
      else
	InfoBoxes[i]->SetComment(TEXT("MANUAL"));
      break;
    case 0: // GPS Alt
      Units::FormatAlternateUserAltitude(GPS_INFO.Altitude,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 1: // AGL
      Units::FormatAlternateUserAltitude(CALCULATED_INFO.AltitudeAGL,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
      InfoBoxes[i]->SetComment(sTmp);
      break;
    case 33:
      Units::FormatAlternateUserAltitude(GPS_INFO.BaroAltitude,
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
      if (EnableBlockSTF) {
	InfoBoxes[i]->SetComment(TEXT("BLOCK"));
      } else {
	InfoBoxes[i]->SetComment(TEXT("DOLPHIN"));
      }
      break;
    case 55: // own team code
      InfoBoxes[i]->SetComment(TeammateCode);
      if (TeamFlarmTracking)
	{
	  if (IsFlarmTargetCNInRange())
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

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
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

      if (IsFlarmTargetCNInRange())
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 57: // team bearing dif

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
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
      if (IsFlarmTargetCNInRange())
	{
	  InfoBoxes[i]->SetColorBottom(2);
	}
      else
	{
	  InfoBoxes[i]->SetColorBottom(1);
	}

      break;
    case 58: // team range

      if (TeamFlarmIdTarget != 0)
	{
	  if (_tcslen(TeamFlarmCNTarget) != 0)
	    {
	      InfoBoxes[i]->SetComment(TeamFlarmCNTarget);
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
      if (IsFlarmTargetCNInRange())
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
		if ( CALCULATED_INFO.WindBearing == 0 )
		_stprintf(sTmp,_T("0%s"),_T(DEG)); else
		_stprintf(sTmp,_T("%1.0d%s"),(int)CALCULATED_INFO.WindBearing,_T(DEG));
		InfoBoxes[i]->SetComment(sTmp);
		break;

	// VENTA3 radial
	case 60:
		if ( HomeWaypoint == -1 ) {  // should be redundant
      			InfoBoxes[i]->SetComment(TEXT(""));
			break;
		}
		if ( CALCULATED_INFO.HomeRadial == 0 )
		_stprintf(sTmp,_T("0%s"),_T(DEG)); else
		_stprintf(sTmp,_T("%1.0d%s"),(int)CALCULATED_INFO.HomeRadial,_T(DEG));
		InfoBoxes[i]->SetComment(sTmp);
		break;

	// VENTA3 battery temperature under voltage. There is a good reason to see the temperature,
	// if available: many PNA/PDA will switch OFF during flight under direct sunlight for several
	// hours due to battery temperature too high!! The 314 does!

	// TODO: check temperature too high and set a warning flag to be used by an event or something
	#if (WINDOWSPC<1)
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
			Units::FormatUserDistance(WayPointCalc[ActiveAlternate].Distance,
					 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
			InfoBoxes[i]->SetComment(sTmp);
		} else {
			Units::FormatUserArrival(WayPointCalc[ActiveAlternate].AltArriv,
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
		//Units::FormatUserArrival(GPS_INFO.Altitude,
		Units::FormatUserAltitude(GPS_INFO.Altitude,
			 sTmp, sizeof(sTmp)/sizeof(sTmp[0]));
		InfoBoxes[i]->SetComment(sTmp);
		break;

    default:
      InfoBoxes[i]->SetComment(TEXT(""));
    };

    DisplayTypeLast[i] = DisplayType[i];

  }
  InfoBoxLayout::Paint();

  first = false;

  UnlockNavBox();
}




void DoInfoKey(int keycode) {
  int i;

  if (InfoFocus<0) return; // paranoid

  HideMenu();

  LockNavBox();
  i = getInfoType(InfoFocus);

  // XXX This could crash if MapWindow does not capture

  LockFlightData();
  Data_Options[min(NUMSELECTSTRINGS-1,i)].Process(keycode);
  UnlockFlightData();

  UnlockNavBox();

  InfoBoxesDirty = true;

  TriggerGPSUpdate(); // emulate update to trigger calculations

  InfoBoxFocusTimeOut = 0;
  ResetDisplayTimeOut();

}

int CurrentInfoType;


void PopUpSelect(int Index)
{
  DialogActive = true;
  CurrentInfoType = InfoType[Index];
  StoreType(Index, InfoType[Index]);
  //  ShowWindow(hWndCB,SW_HIDE);
  FullScreen();
  SwitchToMapWindow();
  DialogActive = false;
}


bool InfoBoxClick(HWND wmControl, bool display_locked) {
  int i;

  InfoBoxFocusTimeOut = 0;

  for(i=0;i<numInfoWindows;i++) {
    if(wmControl == InfoBoxes[i]->GetHandle()) {
      InfoWindowActive = true;
      if(display_locked) {
	if( i!= InfoFocus) {
	  FocusOnWindow(i,true);
	  FocusOnWindow(InfoFocus,false);
	  InfoFocus = i;
	  InfoWindowActive = true;
	}
	DisplayText();
	InputEvents::setMode(TEXT("infobox"));
      } else {
	PopUpSelect(i);
	DisplayText();
      }
      return 1;
    }
  }
  return 0;
}


void DeleteInfoBoxFormatters() {
  int i;
  //  CommandBar_Destroy(hWndCB);
  for (i=0; i<NUMSELECTSTRINGS; i++) {
    delete Data_Options[i].Formatter;
  }
}

extern MapWindow hWndMapWindow; // TODO try to avoid this


void InfoBoxFocus(bool display_locked) {
  if (InfoWindowActive) {

    if (display_locked) {
      FocusOnWindow(InfoFocus,true);
    } else {
      FocusOnWindow(InfoFocus,true);
    }
  } else {
    DefocusInfoBox();
    HideMenu();
    SetFocus(hWndMapWindow);
  }
}


void InfoBoxProcessTimer(void) {
  if(InfoWindowActive)
    {
      if (!dlgAirspaceWarningVisible()) {
	// JMW prevent switching to map window if in airspace warning dialog

	if(InfoBoxFocusTimeOut >= FOCUSTIMEOUTMAX)
	  {
	    SwitchToMapWindow();
	  }
	InfoBoxFocusTimeOut ++;
      }
    }
}


void InfoBoxDrawIfDirty(void) {
  if (InfoBoxesDirty) {
    //JMWTEST    LockFlightData();
    AssignValues();
    DisplayText();
    InfoBoxesDirty = false;
    //JMWTEST    UnlockFlightData();
  }
}

void InfoBoxFocusSetMaxTimeOut(void) {
  if (InfoBoxFocusTimeOut< FOCUSTIMEOUTMAX) {
    InfoBoxFocusTimeOut = FOCUSTIMEOUTMAX;
  }
}

void InfoBoxesSetDirty(bool is_dirty) {
  InfoBoxesDirty = is_dirty;
}


void ResetInfoBoxes(void) {
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
