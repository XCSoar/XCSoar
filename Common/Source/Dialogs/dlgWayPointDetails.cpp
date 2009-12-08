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

#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"
#include "UtilsText.hpp"
#include "Math/SunEphemeris.hpp"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "McCready.h"
#include "Screen/Layout.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"
#include "Task.h"

#include <assert.h>

#ifndef CECORE
#ifndef GNAV
#include "Screen/VOIMAGE.h"
#endif
#endif

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndFrame *wInfo=NULL;
static WndFrame *wCommand=NULL;
static WndFrame *wSpecial=NULL; // VENTA3
static WndOwnerDrawFrame *wImage=NULL;
static BOOL hasimage1 = false;
static BOOL hasimage2 = false;
static int SelectedWaypoint = -1;

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
      if (!way_points.get(SelectedWaypoint).Details) {
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
    wDetails->invalidate();
  }

}


static void
OnPaintDetailsListItem(WindowControl * Sender, Canvas &canvas)
{
  (void)Sender;
  if (DrawListIndex < nTextLines){
    TCHAR* text = way_points.get(SelectedWaypoint).Details;
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
      canvas.text_opaque(2 * Layout::scale, 2 * Layout::scale,
                         text + nstart, nlen);
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

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch (key_code) {
    case VK_LEFT:
    case '6':
      ((WndButton *)wf->FindByName(TEXT("cmdPrev")))->set_focus();
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return true;

    case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(TEXT("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}


static void OnGotoClicked(WindowControl * Sender){
  (void)Sender;
  task.FlyDirectTo(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void OnReplaceClicked(WindowControl * Sender){
  (void)Sender;
  task.ReplaceWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
                       XCSoarInterface::Basic());
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void OnNewHomeClicked(WindowControl * Sender){
	(void)Sender;
  XCSoarInterface::SetSettingsComputer().HomeWaypoint = SelectedWaypoint;
  SetToRegistry(szRegistryHomeWaypoint, XCSoarInterface::SettingsComputer().HomeWaypoint);
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

// VENTA3
static void OnSetAlternate1Clicked(WindowControl * Sender){
	(void)Sender;
  XCSoarInterface::SetSettingsComputer().Alternate1 = SelectedWaypoint;
  SetToRegistry(szRegistryAlternate1, XCSoarInterface::SettingsComputer().Alternate1);
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void OnSetAlternate2Clicked(WindowControl * Sender){
	(void)Sender;
  XCSoarInterface::SetSettingsComputer().Alternate2 = SelectedWaypoint;
  SetToRegistry(szRegistryAlternate2, XCSoarInterface::SettingsComputer().Alternate2);
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void OnClearAlternatesClicked(WindowControl * Sender){
	(void)Sender;
  XCSoarInterface::SetSettingsComputer().Alternate1 = -1;
  XCSoarInterface::SetSettingsComputer().EnableAlternate1=false;
  XCSoarInterface::SetSettingsComputer().Alternate2 = -1;
  XCSoarInterface::SetSettingsComputer().EnableAlternate2=false;
  SetToRegistry(szRegistryAlternate1, XCSoarInterface::SettingsComputer().Alternate1);
  SetToRegistry(szRegistryAlternate2, XCSoarInterface::SettingsComputer().Alternate2);
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}


static void OnTeamCodeClicked(WindowControl * Sender){
	(void)Sender;
  XCSoarInterface::SetSettingsComputer().TeamCodeRefWaypoint =
    SelectedWaypoint;
  SetToRegistry(szRegistryTeamcodeRefWaypoint,
		XCSoarInterface::SettingsComputer().TeamCodeRefWaypoint);
  wf->SetModalResult(mrOK);
}


static void OnInsertInTaskClicked(WindowControl * Sender){
  (void)Sender;
  task.InsertWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
                      XCSoarInterface::Basic());
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void OnAppendInTaskClicked(WindowControl * Sender){
  (void)Sender;
  task.InsertWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
                      XCSoarInterface::Basic(), true);
  wf->SetModalResult(mrOK);
}


static void OnRemoveFromTaskClicked(WindowControl * Sender){
  (void)Sender;
  task.RemoveWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
                      XCSoarInterface::Basic());
  wf->SetModalResult(mrOK);
}

static void
OnImagePaint(WindowControl *Sender, Canvas &canvas)
{
  (void)Sender;

#ifndef CECORE
#ifndef GNAV
  if (page == 3)
    jpgimage1.Draw(canvas, 0, 0, -1, -1);

  if (page == 4)
    jpgimage2.Draw(canvas, 0, 0, -1, -1);

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

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWayPointDetails_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_WAYPOINTDETAILS_L"));

  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWayPointDetails.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_WAYPOINTDETAILS"));
  }
  nTextLines = 0;

  if (!wf) return;

  SelectedWaypoint = task.getSelected();

  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  ExpandLocalPath(szWaypointFile);
  ExtractDirectory(Directory, szWaypointFile);

  _stprintf(path_modis,TEXT("%s\\modis-%03d.jpg"),
           Directory,
           SelectedWaypoint+1);
  _stprintf(path_google,TEXT("%s\\google-%03d.jpg"),
           Directory,
           SelectedWaypoint+1);

  const WAYPOINT &way_point = way_points.get(SelectedWaypoint);

  _stprintf(sTmp, TEXT("%s: "), wf->GetCaption());
  _tcscat(sTmp, way_point.Name);
  wf->SetCaption(sTmp);

  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpComment")));
  wp->SetText(way_point.Comment);
  wp->SetButtonSize(16);

  Units::LongitudeToString(way_point.Location.Longitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLongitude")))
    ->SetText(sTmp);

  Units::LatitudeToString(way_point.Location.Latitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLatitude")))
    ->SetText(sTmp);

  Units::FormatUserAltitude(way_point.Altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpAltitude")))
    ->SetText(sTmp);

  SunEphemeris sun;
  sunsettime = sun.CalcSunTimes
    (way_point.Location,
     XCSoarInterface::Basic(), XCSoarInterface::Calculated(),
     GetUTCOffset()/3600);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  _stprintf(sTmp, TEXT("%02d:%02d"), sunsethours, sunsetmins);
  ((WndProperty *)wf->FindByName(TEXT("prpSunset")))
    ->SetText(sTmp);

  double distance, bearing;
  DistanceBearing(XCSoarInterface::Basic().Location,
                  way_point.Location,
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

  alt = XCSoarInterface::Calculated().NavAltitude -
    GlidePolar::MacCreadyAltitude(0.0,
				  distance,
				  bearing,
				  XCSoarInterface::Calculated().WindSpeed,
				  XCSoarInterface::Calculated().WindBearing,
				  0, 0, true,
				  0)
    -XCSoarInterface::SettingsComputer().SafetyAltitudeArrival
    -way_point.Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc0")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at safety mc

  alt = XCSoarInterface::Calculated().NavAltitude -
    GlidePolar::MacCreadyAltitude(GlidePolar::AbortSafetyMacCready(),
				  distance,
				  bearing,
				  XCSoarInterface::Calculated().WindSpeed,
				  XCSoarInterface::Calculated().WindBearing,
				  0, 0, true,
				  0)
    -XCSoarInterface::SettingsComputer().SafetyAltitudeArrival
    -way_point.Altitude;

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc1")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at current mc

  alt = XCSoarInterface::Calculated().NavAltitude -
    GlidePolar::MacCreadyAltitude(GlidePolar::GetMacCready(),
				  distance,
				  bearing,
				  XCSoarInterface::Calculated().WindSpeed,
				  XCSoarInterface::Calculated().WindBearing,
				  0, 0, true,
				  0)
    -XCSoarInterface::SettingsComputer().SafetyAltitudeArrival
    -way_point.Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc2")));
  if (wp) wp->SetText(sTmp);

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

  nTextLines = TextToLineOffsets(way_point.Details,
				 LineOffsets,
				 MAXLINES);

  /* TODO enhancement: wpdetails
  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpDetails")));
  wp->SetText(way_point.Details);
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
    wb->SetOnClickNotify(OnInsertInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdAppendInTask")));
  if (wb)
    wb->SetOnClickNotify(OnAppendInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdRemoveFromTask")));
  if (wb)
    wb->SetOnClickNotify(OnRemoveFromTaskClicked);

#ifndef CECORE
#ifndef GNAV
  hasimage1 = jpgimage1.Load(wImage->get_canvas(), path_modis );
  hasimage2 = jpgimage2.Load(wImage->get_canvas(), path_google );
#endif
#endif

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}
