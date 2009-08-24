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

#include "XCSoar.h"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Math/Earth.hpp"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "Utils.h"
#include "externs.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "McReady.h"
#include "Dialogs/dlgTools.h"
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"

#include <assert.h>

extern void DrawJPG(HDC hdc, RECT rc);

#ifndef CECORE
#ifndef GNAV
#include "VOIMAGE.h"
#endif
#endif

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndOwnerDrawFrame *wDetailsEntry = NULL;
static WndFrame *wInfo=NULL;
static WndFrame *wCommand=NULL;
static WndFrame *wSpecial=NULL; // VENTA3
static WndOwnerDrawFrame *wImage=NULL;
static BOOL hasimage1 = false;
static BOOL hasimage2 = false;

#ifndef CECORE
#ifndef GNAV
static CVOImage jpgimage1;
static CVOImage jpgimage2;
#endif
#endif

static TCHAR path_modis[MAX_PATH];
static TCHAR path_google[MAX_PATH];
static TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
static TCHAR Directory[MAX_PATH];

#define MAXLINES 100
static int LineOffsets[MAXLINES];
static int DrawListIndex=0;
static int nTextLines=0;

static void NextPage(int Step){
  bool page_ok=false;
  page += Step;
  do {
    if (page<0) {
      page = 5;
    }
    if (page>5) {
      page = 0;
    }
    switch(page) {
    case 0:
      page_ok = true;
      break;
    case 1:
      if (!WayPointList[SelectedWaypoint].Details) {
        page += Step;
      } else {
        page_ok = true;
      }
      break;
    case 2:
      page_ok = true;
      break;
    case 3: // VENTA3
      page_ok = true;
      break;
    case 4:
      if (!hasimage1) {
        page += Step;
      } else {
        page_ok = true;
      }
      break;
    case 5:
      if (!hasimage2) {
        page += Step;
      } else {
        page_ok = true;
      }
      break;
    default:
      page_ok = true;
      page = 0;
      break;
      // error!
    }
  } while (!page_ok);

  wInfo->SetVisible(page == 0);
  wDetails->SetVisible(page == 1);
  wCommand->SetVisible(page == 2);
  wSpecial->SetVisible(page == 3);
  wImage->SetVisible(page > 4);

  if (page==1) {
    wDetails->ResetList();
    wDetails->Redraw();
  }

}


static void OnPaintDetailsListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  if (DrawListIndex < nTextLines){
    TCHAR* text = WayPointList[SelectedWaypoint].Details;
    int nstart = LineOffsets[DrawListIndex];
    int nlen;
    if (DrawListIndex<nTextLines-1) {
      nlen = LineOffsets[DrawListIndex+1]-LineOffsets[DrawListIndex]-1;
      nlen--;
    } else {
      nlen = _tcslen(text+nstart);
    }
    while (_tcscmp(text+nstart+nlen-1,TEXT("\r"))==0) {
      nlen--;
    }
    while (_tcscmp(text+nstart+nlen-1,TEXT("\n"))==0) {
      nlen--;
    }
    if (nlen>0) {
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 text+nstart,
		 nlen,
		 NULL);
    }
  }
}


static void OnDetailsListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
	(void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = nTextLines-1;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
  }
}



static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)lParam; (void)Sender;
  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}


static void OnGotoClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  FlyDirectTo(SelectedWaypoint);
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnReplaceClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  ReplaceWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnNewHomeClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  HomeWaypoint = SelectedWaypoint;
  SetToRegistry(szRegistryHomeWaypoint, HomeWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

// VENTA3
static void OnSetAlternate1Clicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  Alternate1 = SelectedWaypoint;
  SetToRegistry(szRegistryAlternate1, Alternate1);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnSetAlternate2Clicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  Alternate2 = SelectedWaypoint;
  SetToRegistry(szRegistryAlternate2, Alternate2);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnClearAlternatesClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  Alternate1 = -1; OnAlternate1=false;
  Alternate2 = -1; OnAlternate2=false;
  SetToRegistry(szRegistryAlternate1, Alternate1);
  SetToRegistry(szRegistryAlternate2, Alternate2);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}


static void OnTeamCodeClicked(WindowControl * Sender){
	(void)Sender;
  TeamCodeRefWaypoint = SelectedWaypoint;
  SetToRegistry(szRegistryTeamcodeRefWaypoint, TeamCodeRefWaypoint);
  wf->SetModalResult(mrOK);
}


static void OnInserInTaskClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  InsertWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnAppendInTaskClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  InsertWaypoint(SelectedWaypoint, true);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}


static void OnRemoveFromTaskClicked(WindowControl * Sender){
	(void)Sender;
  LockTaskData();
  RemoveWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnImagePaint(WindowControl * Sender, HDC hDC){
  (void)Sender;

#ifndef CECORE
#ifndef GNAV
  if (page == 3)
    jpgimage1.Draw(hDC, 0, 0, -1, -1);

  if (page == 4)
    jpgimage2.Draw(hDC, 0, 0, -1, -1);

#endif
#endif
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnPaintDetailsListItem),
  DeclareCallBackEntry(OnDetailsListInfo),
  DeclareCallBackEntry(NULL)
};



void dlgWayPointDetailsShowModal(void){

  TCHAR sTmp[128];
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  WndProperty *wp;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWayPointDetails_L.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_WAYPOINTDETAILS_L"));

  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWayPointDetails.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_WAYPOINTDETAILS"));
  }
  nTextLines = 0;

  if (!wf) return;

  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  ExpandLocalPath(szWaypointFile);
  ExtractDirectory(Directory, szWaypointFile);

  _stprintf(path_modis,TEXT("%s\\modis-%03d.jpg"),
           Directory,
           SelectedWaypoint+1);
  _stprintf(path_google,TEXT("%s\\google-%03d.jpg"),
           Directory,
           SelectedWaypoint+1);

  _stprintf(sTmp, TEXT("%s: "), wf->GetCaption());
  _tcscat(sTmp, WayPointList[SelectedWaypoint].Name);
  wf->SetCaption(sTmp);

  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpComment")));
  wp->SetText(WayPointList[SelectedWaypoint].Comment);
  wp->SetButtonSize(16);

  Units::LongitudeToString(WayPointList[SelectedWaypoint].Longitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLongitude")))
    ->SetText(sTmp);

  Units::LatitudeToString(WayPointList[SelectedWaypoint].Latitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLatitude")))
    ->SetText(sTmp);

  Units::FormatUserAltitude(WayPointList[SelectedWaypoint].Altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpAltitude")))
    ->SetText(sTmp);

  sunsettime = DoSunEphemeris(WayPointList[SelectedWaypoint].Longitude,
                              WayPointList[SelectedWaypoint].Latitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  _stprintf(sTmp, TEXT("%02d:%02d"), sunsethours, sunsetmins);
  ((WndProperty *)wf->FindByName(TEXT("prpSunset")))
    ->SetText(sTmp);

  double distance, bearing;
  DistanceBearing(GPS_INFO.Latitude,
                  GPS_INFO.Longitude,
                  WayPointList[SelectedWaypoint].Latitude,
                  WayPointList[SelectedWaypoint].Longitude,
                  &distance,
                  &bearing);

  TCHAR DistanceText[MAX_PATH];
  Units::FormatUserDistance(distance, DistanceText, 10);
  ((WndProperty *)wf->FindByName(TEXT("prpDistance")))
    ->SetText(DistanceText);

  _stprintf(sTmp, TEXT("%d")TEXT(DEG), iround(bearing));
  ((WndProperty *)wf->FindByName(TEXT("prpBearing")))
    ->SetText(sTmp);

  double alt=0;

  // alt reqd at mc 0

  alt = CALCULATED_INFO.NavAltitude -
    GlidePolar::MacCreadyAltitude(0.0,
				  distance,
				  bearing,
				  CALCULATED_INFO.WindSpeed,
				  CALCULATED_INFO.WindBearing,
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc0")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at safety mc

  alt = CALCULATED_INFO.NavAltitude -
    GlidePolar::MacCreadyAltitude(GlidePolar::AbortSafetyMacCready(),
				  distance,
				  bearing,
				  CALCULATED_INFO.WindSpeed,
				  CALCULATED_INFO.WindBearing,
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc1")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at current mc

  alt = CALCULATED_INFO.NavAltitude -
    GlidePolar::MacCreadyAltitude(MACCREADY,
				  distance,
				  bearing,
				  CALCULATED_INFO.WindSpeed,
				  CALCULATED_INFO.WindBearing,
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc2")));
  if (wp) wp->SetText(sTmp);

  /////

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wInfo    = ((WndFrame *)wf->FindByName(TEXT("frmInfos")));
  wCommand = ((WndFrame *)wf->FindByName(TEXT("frmCommands")));
  wSpecial = ((WndFrame *)wf->FindByName(TEXT("frmSpecial"))); // VENTA3
  wImage   = ((WndOwnerDrawFrame *)wf->FindByName(TEXT("frmImage")));
  wDetails = (WndListFrame*)wf->FindByName(TEXT("frmDetails"));

  assert(wInfo!=NULL);
  assert(wCommand!=NULL);
  assert(wSpecial!=NULL); // VENTA3
  assert(wImage!=NULL);
  assert(wDetails!=NULL);

  wDetailsEntry =
    (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmDetailsEntry"));
  assert(wDetailsEntry!=NULL);
  wDetailsEntry->SetCanFocus(true);

  nTextLines = TextToLineOffsets(WayPointList[SelectedWaypoint].Details,
				 LineOffsets,
				 MAXLINES);

  /* TODO enhancement: wpdetails
  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpDetails")));
  wp->SetText(WayPointList[SelectedWaypoint].Details);
  */

  wInfo->SetBorderKind(BORDERLEFT);
  wCommand->SetBorderKind(BORDERLEFT);
  wSpecial->SetBorderKind(BORDERLEFT);
  wImage->SetBorderKind(BORDERLEFT | BORDERTOP | BORDERBOTTOM | BORDERRIGHT);
  wDetails->SetBorderKind(BORDERLEFT);

  wCommand->SetVisible(false);
  wSpecial->SetVisible(false);
  wImage->SetCaption(gettext(TEXT("Blank!")));
  wImage->SetOnPaintNotify(OnImagePaint);

  WndButton *wb;

  wb = ((WndButton *)wf->FindByName(TEXT("cmdGoto")));
  if (wb)
    wb->SetOnClickNotify(OnGotoClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdReplace")));
  if (wb)
    wb->SetOnClickNotify(OnReplaceClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdNewHome")));
  if (wb)
    wb->SetOnClickNotify(OnNewHomeClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdSetAlternate1")));
  if (wb)
    wb->SetOnClickNotify(OnSetAlternate1Clicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdSetAlternate2")));
  if (wb)
    wb->SetOnClickNotify(OnSetAlternate2Clicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdClearAlternates")));
  if (wb)
    wb->SetOnClickNotify(OnClearAlternatesClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdTeamCode")));
  if (wb)
    wb->SetOnClickNotify(OnTeamCodeClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdInserInTask")));
  if (wb)
    wb->SetOnClickNotify(OnInserInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdAppendInTask")));
  if (wb)
    wb->SetOnClickNotify(OnAppendInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdRemoveFromTask")));
  if (wb)
    wb->SetOnClickNotify(OnRemoveFromTaskClicked);

#ifndef CECORE
#ifndef GNAV
  hasimage1 = jpgimage1.Load(wImage->GetDeviceContext() ,path_modis );
  hasimage2 = jpgimage2.Load(wImage->GetDeviceContext() ,path_google );
#endif
#endif

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}
