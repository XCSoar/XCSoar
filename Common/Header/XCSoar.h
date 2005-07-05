
#if !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "sizes.h"

typedef struct _SCREEN_INFO
{
	TCHAR Description[DESCRIPTION_SIZE +1];
	TCHAR Title[TITLE_SIZE + 1];
	TCHAR Format[FORMAT_SIZE + 1];
	double Value;
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
extern void PopupWaypointDetails();

extern void ShowInfoBoxes();
extern void HideInfoBoxes();
extern void ToggleFullScreen();
extern void RequestToggleFullScreen();

#define DEG_TO_RAD 0.0174532923889
#define RAD_TO_DEG 57.2957799433			

#endif // !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
