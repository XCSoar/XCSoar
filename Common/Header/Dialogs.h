#if !defined(AFX_DIALOGS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_DIALOGS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
LRESULT CALLBACK AAT						(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AATStart				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AATTurn				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Menu						(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About					(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Progress				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetUnits				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Select					(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetPolar				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DisplayOptions	(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MapDisplayOptions	(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetTask				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FinalGlide 		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetBugsBallast	(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TaskSettings		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Register 			(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetFiles				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetMapFiles				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetInterfaceFiles				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AirspaceAlt		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AirspaceColourDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MapColour			(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ColourSelect		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AirspacePress	(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK COMMOptions		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SaveProfile		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LoadProfile		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SetAirspaceWarnings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Settings				(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WaypointDetails		(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void AssignedArea(void);
LRESULT CALLBACK LoggerDetails 			(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AudioSettings					(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void SetWindowText_gettext(HWND hDlg, int entry);
TCHAR* gettext(TCHAR* text);
void ClearStatusMessages(void);

void StartupScreen();

HWND CreateProgressDialog(TCHAR *text);
void CloseProgressDialog();
BOOL StepProgressDialog();
BOOL SetProgressStepSize(int nSize);


extern void DoStatusMessage(TCHAR* text, TCHAR* data = NULL);

#define NUMPOLARS 7 // number of in-built polars

#if (NEWINFOBOX<1)
#define MessageBoxX MessageBox
#else
#include "dlgTools.h"

bool dlgAirspaceWarningShowDlg(bool force);
int dlgWayPointSelect(void);
int dlgAirspaceColoursShowModal(void);
int dlgAirspacePatternsShowModal(void);
void dlgAirspaceShowModal(bool colored);
void dlgBasicSettingsShowModal(void);
void dlgBrightnessShowModal(void);
void dlgHelpShowModal(TCHAR* Caption, TCHAR* HelpText);
void dlgChecklistShowModal(void);
void dlgConfigurationShowModal(void);
void dlgVegaDemoShowModal(void);
bool dlgConfigurationVarioShowModal(void);
void dlgLoggerReplayShowModal(void);
void dlgBasicSettingsShowModal(void);
void dlgStartupShowModal(void);
void dlgTaskCalculatorShowModal(void);
void dlgWindSettingsShowModal(void);
void dlgAnalysisShowModal(void);
void dlgStatusShowModal(void);
void dlgStatusSystemShowModal(void);
void dlgStatusTaskShowModal(void);
void dlgSwitchesShowModal(void);
void dlgTaskWaypointShowModal(int itemindex, int type);
void dlgTaskOverviewShowModal(void);
void dlgVoiceShowModal(void);
void dlgWayPointDetailsShowModal(void);
void dlgTextEntryShowModal(TCHAR *text);
void dlgTeamCodeShowModal(void);

#endif

#endif
