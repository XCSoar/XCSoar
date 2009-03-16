#if !defined(AFX_DIALOGS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_DIALOGS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

void ReadLanguageFile(void);

void SetWindowText_gettext(HWND hDlg, int entry);
TCHAR* gettext(const TCHAR* text);
void ClearStatusMessages(void);

void StartupScreen();

HWND CreateProgressDialog(TCHAR *text);
void CloseProgressDialog();
void StepProgressDialog();
BOOL SetProgressStepSize(int nSize);
void StartHourglassCursor();
void StopHourglassCursor();

extern void DoStatusMessage(const TCHAR* text, const TCHAR* data = NULL);

#define NUMPOLARS 7 // number of in-built polars

#include "dlgTools.h"

bool dlgAirspaceWarningShowDlg(bool force);
// int dlgWayPointSelect(void);
int dlgWayPointSelect(double lon=0.0, double lat=90.0, int type=-1, int FilterNear=0);
int dlgAirspaceColoursShowModal(void);
int dlgAirspacePatternsShowModal(void);
void dlgAirspaceShowModal(bool colored);
void dlgBasicSettingsShowModal(void);
void dlgBrightnessShowModal(void);
void dlgHelpShowModal(const TCHAR* Caption, const TCHAR* HelpText);
void dlgChecklistShowModal(void);
void dlgConfigurationShowModal(void);
void dlgVegaDemoShowModal(void);
bool dlgConfigurationVarioShowModal(void);
void dlgLoggerReplayShowModal(void);
void dlgBasicSettingsShowModal(void);
void dlgStartupShowModal(void);
void dlgTaskCalculatorShowModal(void);
void dlgWindSettingsShowModal(void);
void dlgStartTaskShowModal(bool *validStart, double Time, double Speed, double Altitude);
void dlgAnalysisShowModal(void);
void dlgStatusShowModal(int page);
void dlgSwitchesShowModal(void);
void dlgTaskWaypointShowModal(int itemindex, int type, bool addonly=false);
void dlgTaskOverviewShowModal(void);
void dlgVoiceShowModal(void);
void dlgWayPointDetailsShowModal(void);
void dlgTextEntryShowModal(TCHAR *text, int width=0);
void dlgTeamCodeShowModal(void);
void dlgStartPointShowModal(void);
#include "MapWindow.h"
void dlgWaypointEditShowModal(WAYPOINT *wpt);
void dlgWeatherShowModal(void);
void dlgAirspaceSelect(void);
void dlgTarget(void);
bool dlgTaskRules(void);
void dlgAirspaceDetails(int the_circle, int the_area);
bool dlgAirspaceWarningVisible(void);
void dlgFlarmTrafficShowModal(void);


#if (WINDOWSPC>0)
#ifdef DEBUG
//#define DEBUG_TRANSLATIONS
#pragma warning( disable : 4786 )
#endif
#endif

void WriteMissingTranslations(void);
void dlgTextEntryKeyboardShowModal(TCHAR *text, int width=0);
void dlgNumberEntryKeyboardShowModal(int *value, int width=0);

#endif
