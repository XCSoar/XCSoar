
#if !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#include "resource.h"
#include "sizes.h"


class InfoBoxFormatter {
 public:
  InfoBoxFormatter(TCHAR *theformat) {
    _tcscpy(Format, theformat);
    Valid = TRUE;
    Value = 0.0;    
  }

  virtual void Render(HWND hWnd);
  BOOL Valid;
  double Value;
  TCHAR Format[FORMAT_SIZE+1];
  TCHAR Text[100];

  virtual void AssignValue(int i);

};


class FormatterWaypoint: public InfoBoxFormatter {
 public:
  FormatterWaypoint(TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual void Render(HWND hWnd);
};

class FormatterLowWarning: public InfoBoxFormatter {
 public:
  FormatterLowWarning(TCHAR *theformat, double the_minimum)
    :InfoBoxFormatter(theformat) { 

    minimum = the_minimum;

  };

  virtual void Render(HWND hWnd);
  double minimum;
  virtual void AssignValue(int i);
};


class FormatterTime: public InfoBoxFormatter {
 public:
  FormatterTime(TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual void Render(HWND hWnd);
  virtual void AssignValue(int i);
  int hours;
  int mins;
  int seconds;
  void SecsToDisplayTime(int i);
};



typedef struct _SCREEN_INFO
{
  TCHAR Description[DESCRIPTION_SIZE +1];
  TCHAR Title[TITLE_SIZE + 1];
  InfoBoxFormatter *Formatter;
  void (*Process)(int UpDown);  
  char next_screen;
  char prev_screen;
} SCREEN_INFO;



void ProcessChar1 (char c);
void ProcessChar2 (char c);

extern void UnlockFlightData();
extern void LockFlightData();
extern void UnlockTerrainDataCalculations();
extern void LockTerrainDataCalculations();
extern void UnlockTerrainDataGraphics();
extern void LockTerrainDataGraphics();
extern void UnlockNavBox();
extern void LockNavBox();
void FocusOnWindow(int i, bool selected);
void FullScreen();

extern void ShowInfoBoxes();
extern void HideInfoBoxes();

extern void PopupWaypointDetails();
extern void PopupAnalysis();
extern void ShowStatus();

#define DEG_TO_RAD .0174532925199432958
#define RAD_TO_DEG 57.2957795131		
#define M_PI 3.14159265359
#define M_2PI 6.28318530718

void DebugStore(char *Str);


typedef struct
{
  BYTE acStatus;
  // 0 offline
  // 1 online
  // 255 unknown
  BYTE chargeStatus;
  // 1 high
  // 2 low
  // 4 critical
  // 8 charging
  // 128 no system battery
  // 255 unknown
  BYTE BatteryLifePercent;
  // 0-100 or 255 if unknown

} BATTERYINFO;


DWORD GetBatteryInfo(BATTERYINFO* pBatteryInfo);
void BlankDisplay(bool doblank);      
void DefocusInfoBox(void);
void Event_SelectInfoBox(int i);
void Event_ChangeInfoBoxType(int i);
void DoInfoKey(int keycode);
void SwitchToMapWindow(void);



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

typedef struct{
  int Height;
  int AscentHeight;
  int CapitalHeight;
}FontHeightInfo_t;

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
  BestCruiseTrack_t BestCruiseTrack;
  Aircraft_t Aircraft;
  bool DontShowSpeedToFly;
  IndFinalGlide_t IndFinalGlide;
  IndLandable_t IndLandable;
  bool DontShowAutoMacCready;
} Appearance_t;

extern Appearance_t Appearance;

// ******************************************************************

#endif // !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
