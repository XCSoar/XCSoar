/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#if !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include "StdAfx.h"
#include "Defines.h"
#include "resource.h"
#include "Sizes.h"
#include "Units.h"
#include "Statistics.h"
#include "Screen/Font.hpp"
#include "Formatter/Base.hpp"

class Trigger;
class MapWindow;

class FormatterLowWarning: public InfoBoxFormatter {
 public:
  FormatterLowWarning(const TCHAR *theformat, double the_minimum)
    :InfoBoxFormatter(theformat) {

    minimum = the_minimum;

  };

  virtual const TCHAR *Render(int *color);
  double minimum;
  virtual void AssignValue(int i);
};


class FormatterTime: public InfoBoxFormatter {
 public:
  FormatterTime(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
  virtual void AssignValue(int i);
  int hours;
  int mins;
  int seconds;
  void SecsToDisplayTime(int i);
};


class FormatterAATTime: public FormatterTime {
 public:
  FormatterAATTime(const TCHAR *theformat):FormatterTime(theformat) {};

  virtual const TCHAR *Render(int *color);
  virtual void AssignValue(int i);
  int status;
};


class FormatterDiffBearing: public InfoBoxFormatter {
 public:
  FormatterDiffBearing(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
};

typedef struct _SCREEN_INFO
{
  UnitGroup_t UnitGroup;
  TCHAR Description[DESCRIPTION_SIZE +1];
  TCHAR Title[TITLE_SIZE + 1];
  InfoBoxFormatter *Formatter;
  void (*Process)(int UpDown);
  char next_screen;
  char prev_screen;
} SCREEN_INFO;



void ProcessChar1 (char c);
void ProcessChar2 (char c);

extern void UnlockEventQueue();
extern void LockEventQueue();
extern void UnlockComm();
extern void LockComm();
extern void UnlockGraphicsData();
extern void LockGraphicsData();
extern void UnlockFlightData();
extern void LockFlightData();
extern void UnlockTaskData();
extern void LockTaskData();
extern void UnlockTerrainDataCalculations();
extern void LockTerrainDataCalculations();
extern void UnlockTerrainDataGraphics();
extern void LockTerrainDataGraphics();
extern void UnlockNavBox();
extern void LockNavBox();
extern void TriggerGPSUpdate();
extern void TriggerVarioUpdate();
extern Trigger drawTriggerEvent;

void FocusOnWindow(int i, bool selected);
void FullScreen();

extern void ShowInfoBoxes();
extern void HideInfoBoxes();

extern void PopupWaypointDetails();
extern void PopupAnalysis();
extern void RestartCommPorts();
extern bool Debounce();

#define DEG_TO_RAD .0174532925199432958
#define RAD_TO_DEG 57.2957795131
#if defined(M_PI)
  #undef M_PI
#endif
#define M_PI 3.14159265359

extern "C" {
void DebugStore(const char *Str, ...);
}

void StartupStore(const TCHAR *Str, ...);

void DefocusInfoBox(void);
void Event_SelectInfoBox(int i);
void Event_ChangeInfoBoxType(int i);
void DoInfoKey(int keycode);
void SwitchToMapWindow(void);


/*
    Here we declare Model Types for embedded custom versions. Initially for PNAs only.
	We don't need a "type" and a "model" such as "pna" and "hp310". Instead we use a
	single int value with subsets made of ranges.
	We use modeltypes currently for extraclipping, hardware key transcoding, and we should
	also handle embedded gps com ports and adjustments (TODO)

    types     0 -    99 are reserved and 0 is generic/unknown
    types   100 -   999 are special devices running embedded XCSoar
    types  1000 -  9999 are PDAs
    types 10000 - 99999 are PNAs, each brand with 200 units slots for inner types
                                 (initially we try to stay below 32767 within a short var)
    types over 100000	are reserved and should not be used
 */

#if defined(PNA) || defined(FIVV) // VENTA
#define MODELTYPE_UNKNOWN		0
#define MODELTYPE_GENERIC		0

#define MODELTYPE_EMBEDDED		 100	// generic embedded
#define MODELTYPE_ALTAIR		 101

#define MODELTYPE_PDA_PDA		1000	// generic PDA
#define MODELTYPE_PDA			1000

#define MODELTYPE_PNA_PNA		10000	// generic PNA
#define MODELTYPE_PNA			10000
#define MODELTYPE_PNA_HP		10200	// Generic HP
#define MODELTYPE_PNA_HP31X		10201	// HP310, 312, 314, 316

#define MODELTYPE_PNA_DAYTON	10400	// Generic VDO Dayton
#define MODELTYPE_PNA_PN6000	10401

#define MODELTYPE_PNA_MIO		10600	// Generic definitions
#define MODELTYPE_PNA_MIO520	10601
#define	MODELTYPE_PNA_MIOP350	10602

#define MODELTYPE_PNA_NAVMAN	10800
#define MODELTYPE_PNA_GARMIN	11000
#define MODELTYPE_PNA_CLARION	11200
#define MODELTYPE_PNA_MEDION	11400
#define MODELTYPE_PNA_MEDION_P5	11401	// clipping problems for P5430 and P5 family
#define MODELTYPE_PNA_SAMSUNG	11600
#define MODELTYPE_PNA_NAVIGO	11800
#define MODELTYPE_PNA_NOKIA	12000
#define MODELTYPE_PNA_NOKIA_500	12001 // 480x272


#endif

// This could be also used for PDA in landscape..
typedef enum{
  ssnone=0,
  ss240x320,
  ss480x640,
  ss480x800,
  sslandscape, //  <landscape=portrait modes, >landscape=landscape modes
  ss320x240,
  ss480x234,
  ss480x272,
  ss640x480,
  ss800x480
}ScreenSize_t;

typedef enum{
  apMsDefault=0,
  apMsNone,
  apMsAltA
}MapScaleAppearance_t;

typedef enum{
  apMs2Default=0,
  apMs2None,
  apMs2AltA
}MapScale2Appearance_t;

typedef enum{
  apFlightModeIconDefault=0,
  apFlightModeIconAltA
}FlightModeIconAppearance_t;

typedef enum{
  apCompassDefault=0,
  apCompassAltA
}CompassAppearance_t;

typedef enum{
  ctBestCruiseTrackDefault=0,
  ctBestCruiseTrackAltA,
}BestCruiseTrack_t;

typedef enum{
  afAircraftDefault=0,
  afAircraftAltA
}Aircraft_t;

typedef enum{
  fgFinalGlideDefault=0,
  fgFinalGlideAltA,
}IndFinalGlide_t;

typedef enum{
  wpLandableDefault=0,
  wpLandableAltA,
}IndLandable_t;

typedef enum{
  smAlligneCenter=0,
  smAlligneTopLeft,
}StateMessageAlligne_t;

typedef enum{
  tiHighScore=0,
  tiKeyboard,
}TextInputStyle_t;

typedef enum{
  gvnsDefault=0,
  gvnsLongNeedle,
}GaugeVarioNeedleStyle_t;

typedef enum{
  apIbBox=0,
  apIbTab
}InfoBoxBorderAppearance_t;

typedef enum{
  apIg0=0,
  apIg1,
  apIg2,
  apIg3,
  apIg4,
  apIg5,
  apIg6,
  apIg7
}InfoBoxGeomAppearance_t;

#if defined(PNA) || defined(FIVV)
// VENTA-ADDON MODEL
typedef enum{
	apImPnaGeneric=0,
	apImPnaHp31x,
	apImPnaMedionP5,
	apImPnaMio,
	apImPnaNokia500,
	apImPnaPn6000,
}InfoBoxModelAppearance_t;
#endif

typedef enum{
	evgNormal=0,
	evgExtended,
} ExtendedVisualGlide_t;

typedef enum{
	vkDisabled=0,
	vkEnabled,
} VirtualKeys_t;
typedef enum{
	ae15seconds=0,
	ae30seconds,
	ae60seconds,
	ae90seconds,
	ae2minutes,
	ae3minutes,
} AverEffTime_t;

typedef struct {
        int     array[RASIZE]; // rotary array with a predefined max capacity
        short   start;          // pointer to current first item in rotarybuf if used
        short   size;           // real size of rotary buffer (0-size)
} ifilter_s;

typedef struct {
        int     distance[MAXLDROTARYSIZE]; // rotary array with a predefined max capacity
        int     altitude[MAXLDROTARYSIZE];
	int	totaldistance;
        short   start;          // pointer to current first item in rotarybuf if used
        short   size;           // real size of rotary buffer (0-size)
	bool	valid;
} ldrotary_s;


typedef struct{
  MapScaleAppearance_t MapScale;
  MapScale2Appearance_t MapScale2;
  bool DontShowLoggerIndicator;
  int DefaultMapWidth;
  POINT GPSStatusOffset;
  FlightModeIconAppearance_t FlightModeIcon;
  POINT FlightModeOffset;
  CompassAppearance_t CompassAppearance;
  FontHeightInfo_t TitleWindowFont;
  FontHeightInfo_t MapWindowFont;
  FontHeightInfo_t MapWindowBoldFont;
  FontHeightInfo_t InfoWindowFont;
  FontHeightInfo_t CDIWindowFont;
  FontHeightInfo_t StatisticsFont;
  FontHeightInfo_t MapLabelFont; // VENTA6 added
  FontHeightInfo_t TitleSmallWindowFont;
  BestCruiseTrack_t BestCruiseTrack;
  Aircraft_t Aircraft;
  bool DontShowSpeedToFly;
  IndFinalGlide_t IndFinalGlide;
  IndLandable_t IndLandable;
  bool DontShowAutoMacCready;
  bool InverseInfoBox;
  bool InfoTitelCapital;
  StateMessageAlligne_t StateMessageAlligne;
  TextInputStyle_t TextInputStyle;
  bool GaugeVarioAvgText;
  bool GaugeVarioMc;
  bool GaugeVarioSpeedToFly;
  bool GaugeVarioBallast;
  bool GaugeVarioBugs;
  GaugeVarioNeedleStyle_t GaugeVarioNeedleStyle;
  bool InfoBoxColors;
  InfoBoxBorderAppearance_t InfoBoxBorder;
#if defined(PNA) || defined(FIVV)
  InfoBoxGeomAppearance_t InfoBoxGeom; // VENTA-ADDON
  InfoBoxModelAppearance_t InfoBoxModel; // VENTA-ADDON model change
#endif
  bool InverseAircraft;
  bool GaugeVarioGross;
  bool GaugeVarioAveNeedle;
} Appearance_t;

extern TCHAR XCSoar_Version[256];

// instance of main program
extern HINSTANCE hInst;

// windows
extern HWND hWndMainWindow;           // HWND Main Window
extern MapWindow hWndMapWindow;

// infoboxes
extern int  CurrentInfoType;          // Used for Popup Menu Select
extern int  InfoType[MAXINFOWINDOWS]; //
extern int  InfoFocus;
extern SCREEN_INFO Data_Options[];
extern const int NUMSELECTSTRINGS;
extern int numInfoWindows;

extern Appearance_t Appearance;

// Specials
#ifdef FIVV
extern double GPSAltitudeOffset; 	// VENTA3
#endif
extern double QFEAltitudeOffset; // VENTA3
extern int OnAirSpace; // VENTA3 toggle DrawAirSpace
extern bool WasFlying; // used by auto QFE..
extern double LastFlipBoxTime; // used by XCSoar and Calculations
#if defined(PNA) || defined(FIVV)
extern bool needclipping;
#endif
extern bool EnableAutoBacklight; // VENTA4
extern bool EnableAutoSoundVolume; // VENTA4
extern bool ExtendedVisualGlide;
extern bool VirtualKeys;
extern short ArrivalValue;
extern short AverEffTime;

extern ldrotary_s rotaryLD;

// user controls/parameters
extern double MACCREADY;
extern bool   AutoMacCready;
extern double SAFETYALTITUDEARRIVAL;
extern double SAFETYALTITUDEBREAKOFF;
extern double SAFETYALTITUDETERRAIN;
extern double SAFTEYSPEED;

extern int WindUpdateMode; // unused
extern int NettoSpeed;
extern bool EnableAuxiliaryInfo;
extern int debounceTimeout;

// statistics
extern Statistics flightstats;

extern int UseCustomFonts;
#if (EXPERIMENTAL > 0)
extern BlueDialupSMS bsms;
#endif

#if (WINDOWSPC>0)
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
#endif

typedef enum {psInitInProgress=0, psInitDone=1, psFirstDrawDone=2, psNormalOp=3} StartupState_t;
// 0: not started at all
// 1: everything is alive
// 2: done first draw
// 3: normal operation

extern StartupState_t ProgramStarted;

// ******************************************************************

#ifndef __MINGW32__
#define DEG "°"
#else
#define DEG "Â°"
#endif

#endif // !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
