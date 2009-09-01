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
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "Language.hpp"
#include "Units.h"
#include "Airspace.h"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "Math/Units.h"
#include "Screen/MainWindow.hpp"
#include "Dialogs/dlgTools.h"
#include "Compatibility/vk.h"

#include <assert.h>
#include <stdlib.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;
static WndOwnerDrawFrame *wAirspaceListEntry = NULL;
static Brush hBrushInsideBk;
static Brush hBrushNearBk;
static Brush hBrushInsideAckBk;
static Brush hBrushNearAckBk;
//static HWND   hActiveWindow;

static int Count=0;
static int ItemIndex=-1;
static int DrawListIndex=-1;
static int FocusedID = -1;     // Currently focused airspace ID
static int FocusedIdx = -1;    // Currently socused airspace List Index
static int SelectedID = -1;    // Currently selected airspace ID
static int SelectedIdx = -1;   // Currently selected airspace List Index
static bool fDialogOpen = false;

void AirspaceWarningNotify(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace);

static void DoAck(int Ack){

  AirspaceInfo_c pAS;
  int Idx;

  if (!wAirspaceListEntry->GetFocused())
    Idx = SelectedIdx;
  else
    Idx = ItemIndex;

  if (Idx < 0)
    Idx = 0;

  if (AirspaceWarnGetItem(Idx, pAS)){
    AirspaceWarnDoAck(pAS.ID, Ack);
    wAirspaceList->Redraw();
  }

}

static void OnAckClicked(WindowControl * Sender){
  (void)Sender;
  DoAck(3);
}

static void OnAck1Clicked(WindowControl * Sender){
  (void)Sender;
  DoAck(-1);
}

static void OnAck2Clicked(WindowControl * Sender){
  (void)Sender;
  DoAck(4);
}

static void OnEnableClicked(WindowControl * Sender){
  (void)Sender;
  DoAck(0);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetVisible(false);
//  SetFocus(hWndMainWindow);
//  SetFocus(hWndMapWindow);

  wf->SetModalResult(mrOK);

}

static int OnTimer(WindowControl * Sender){
  (void)Sender;
	return(0);
}

static int OnKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)lParam;
	switch(wParam){
    case VK_RETURN:
      if (wAirspaceListEntry->GetFocused()){
        SelectedID = FocusedID;
        SelectedIdx = FocusedIdx;
        wAirspaceList->Redraw();
        return(0);
      }
      return(1);
    case VK_ESCAPE:
      OnCloseClicked(Sender);
      return(0);
#ifdef GNAV
    case VK_APP1:
    case '6':
      OnAckClicked(Sender);
      return(0);
    case VK_APP2:
    case '7':
      OnAck1Clicked(Sender);
      return(0);
    case VK_APP3:
    case '8':
      OnAck2Clicked(Sender);
      return(0);
    case VK_APP4:
    case '9':
      OnEnableClicked(Sender);
      return(0);
#endif
  }

  return(1);

}

/*
static void OnDistroy(WindowControl * Sender){
  (void)Sender;
  // TODO code: This currently isn't called!

  AirspaceWarnListRemoveNotifier(AirspaceWarningNotify);
  DeleteObject(hBrushInsideBk);
  DeleteObject(hBrushNearBk);
  DeleteObject(hBrushInsideAckBk);
  DeleteObject(hBrushNearAckBk);

  delete wf;
  wf = NULL;
}
*/

static void getAirspaceType(TCHAR *buf, int Type){
  switch (Type)
    {
    case RESTRICT:
      _tcscpy(buf, TEXT("LxR"));
      return;
    case PROHIBITED:
      _tcscpy(buf, TEXT("LxP"));
      return;
    case DANGER:
      _tcscpy(buf, TEXT("LxD"));
      return;
    case CLASSA:
      _tcscpy(buf, TEXT("A"));
      return;
    case CLASSB:
      _tcscpy(buf, TEXT("B"));
      return;
    case CLASSC:
      _tcscpy(buf, TEXT("C"));
      return;
    case CLASSD:
      _tcscpy(buf, TEXT("D"));
      return;
    case CLASSE:
      _tcscpy(buf, TEXT("E"));
      return;
    case CLASSF:
      _tcscpy(buf, TEXT("F"));
      return;
    case NOGLIDER:
      _tcscpy(buf, TEXT("NoGld"));
      return;
    case CTR:
      _tcscpy(buf, TEXT("CTR"));
      return;
    case WAVE:
      _tcscpy(buf, TEXT("Wav"));
      return;
    default:
      _tcscpy(buf, TEXT("?"));
      return;
    }
}

static double FLAltRounded(double alt) {
  int f = iround(alt/10)*10;
  return (double)f;
}


static TCHAR *fmtAirspaceAlt(TCHAR *Buffer, AIRSPACE_ALT *alt){

  TCHAR sUnitBuffer[24];
  TCHAR sAltUnitBuffer[24];

  Units::FormatUserAltitude(alt->Altitude, sUnitBuffer,
			    sizeof(sUnitBuffer)/sizeof(sUnitBuffer[0]));
  Units::FormatAlternateUserAltitude(alt->Altitude,
				     sAltUnitBuffer,
		       sizeof(sAltUnitBuffer)/sizeof(sAltUnitBuffer[0]));

  switch (alt->Base){
    case abUndef:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(Buffer, TEXT("%s %s"), sUnitBuffer, sAltUnitBuffer);
      } else {
	_stprintf(Buffer, TEXT("%s"), sUnitBuffer);
      }
    break;
    case abMSL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(Buffer, TEXT("%s %s MSL"), sUnitBuffer, sAltUnitBuffer);
      } else {
	_stprintf(Buffer, TEXT("%s MSL"), sUnitBuffer);
      }
    break;
    case abAGL:
      if (alt->Altitude == 0)
        _stprintf(Buffer, TEXT("SFC"));
      else {
	Units::FormatUserAltitude(alt->AGL, sUnitBuffer,
				  sizeof(sUnitBuffer)/sizeof(sUnitBuffer[0]));
	Units::FormatAlternateUserAltitude(alt->AGL, sAltUnitBuffer,
			    sizeof(sAltUnitBuffer)/sizeof(sAltUnitBuffer[0]));
	if (Units::GetUserAltitudeUnit() == unMeter) {
	  _stprintf(Buffer, TEXT("%s %s AGL"), sUnitBuffer, sAltUnitBuffer);
	} else {
	  _stprintf(Buffer, TEXT("%s AGL"), sUnitBuffer);
	}
      }
    break;
    case abFL:
      /*AltitudeToQNHAltitude(alt->Altitude)*/
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(Buffer, TEXT("FL%.0f %.0f m %.0f ft"),
		  alt->FL, FLAltRounded(alt->Altitude),
		  FLAltRounded(alt->Altitude*TOFEET));
      } else {
	_stprintf(Buffer, TEXT("FL%.0f %.0f ft"),
		  alt->FL, FLAltRounded(alt->Altitude*TOFEET));
      }
    break;
  }
  return(Buffer);
}

static void
OnAirspaceListItemPaint(WindowControl *Sender, Canvas &canvas)
{
  TCHAR sTmp[128];

  if (Count != 0){

    TCHAR sAckIndicator[6] = TEXT(" -++*");
    TCHAR sName[21];
    TCHAR sTop[32];
    TCHAR sBase[32];
    TCHAR sType[32];
    AIRSPACE_ALT Base;
    AIRSPACE_ALT Top;
    int i = DrawListIndex;
    AirspaceInfo_c pAS;
    int          Type;
    int          TextHeight = 12;
    int          TextTop = 1;
    int          Col0Left = 3;
    int          Col1Left = 120;
    RECT         rc;
    RECT         rcTextClip;
    Brush *hBrushBk = NULL;

    if (i>=Count) return;

    CopyRect(&rc, Sender->GetBoundRect());
    CopyRect(&rcTextClip, Sender->GetBoundRect());
    rcTextClip.right = IBLSCALE(Col1Left - 2);

    InflateRect(&rc, IBLSCALE(-2), IBLSCALE(-2));

    if (!AirspaceWarnGetItem(i, pAS)) return;

    if (ItemIndex == DrawListIndex){
      FocusedID = pAS.ID;
    }

    if (pAS.IsCircle){
      _tcsncpy(sName, AirspaceCircle[pAS.AirspaceIndex].Name,
	       sizeof(sName)/sizeof(sName[0]));
      Base = AirspaceCircle[pAS.AirspaceIndex].Base;
      Top  = AirspaceCircle[pAS.AirspaceIndex].Top;
      Type = AirspaceCircle[pAS.AirspaceIndex].Type;
    } else {
      _tcsncpy(sName, AirspaceArea[pAS.AirspaceIndex].Name,
	       sizeof(sName)/sizeof(sName[0]));
      Base = AirspaceArea[pAS.AirspaceIndex].Base;
      Top  = AirspaceArea[pAS.AirspaceIndex].Top;
      Type = AirspaceArea[pAS.AirspaceIndex].Type;
    }

    sName[sizeof(sName)/sizeof(sName[0])-1] = '\0';

    fmtAirspaceAlt(sTop, &Top);
    fmtAirspaceAlt(sBase, &Base);
    getAirspaceType(sType, Type);

    if (pAS.Inside){
      if (pAS.Acknowledge >= 3)
        hBrushBk = &hBrushInsideAckBk;
      else
        hBrushBk = &hBrushInsideBk;
    } else {
      if ((pAS.hDistance < 2500) && (abs(pAS.vDistance) < 250))
        if (pAS.Acknowledge >= 1)
          hBrushBk = &hBrushNearAckBk;
        else
          hBrushBk = &hBrushNearBk;
    }


    if (SelectedIdx == DrawListIndex){
      InflateRect(&rc, 1, 1);
      canvas.black_pen();
      canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
    } else if (hBrushBk != NULL) {
      canvas.fill_rectangle(rc, *hBrushBk);
    }

    if ((pAS.Acknowledge > 0) && (pAS.Acknowledge >= pAS.WarnLevel)){
      canvas.set_text_color(clGray);
    }

    #ifndef NDEBUG
    wsprintf(sTmp, TEXT("%-20s%d"), sName , pAS.WarnLevel - pAS.Acknowledge);
    #else
    wsprintf(sTmp, TEXT("%-20s"), sName);
    #endif

    canvas.text_clipped(IBLSCALE(Col0Left), IBLSCALE(TextTop),
                        rcTextClip, sTmp);

    wsprintf(sTmp, TEXT("%-20s"), sTop);
    canvas.text_opaque(IBLSCALE(Col1Left), IBLSCALE(TextTop), sTmp);

    wsprintf(sTmp, TEXT("%-20s"), sBase);
    canvas.text_opaque(IBLSCALE(Col1Left), IBLSCALE(TextTop + TextHeight),
                       sTmp);

    if (pAS.Inside){
      wsprintf(sTmp, TEXT("> %c %s"), sAckIndicator[pAS.Acknowledge], sType);
    } else {
      TCHAR DistanceText[MAX_PATH];
      if (pAS.hDistance == 0) {

        // Directly above or below airspace

        Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
        if (pAS.vDistance > 0) {
          wsprintf(sTmp, TEXT("< %c %s ab %s"),
                   sAckIndicator[pAS.Acknowledge],
                   sType, DistanceText);
        }
        if (pAS.vDistance < 0) {
          Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
          wsprintf(sTmp, TEXT("< %c %s bl %s"),
                   sAckIndicator[pAS.Acknowledge],
                   sType, DistanceText);
        }
      } else {
        if ((pAS.vDistance == 0) ||
            (pAS.hDistance < abs(pAS.vDistance)*30 )) {

          // Close to airspace altitude, horizontally separated

          Units::FormatUserDistance(fabs((double)pAS.hDistance),DistanceText, 7);
          wsprintf(sTmp, TEXT("< %c %s H %s"), sAckIndicator[pAS.Acknowledge],
                   sType, DistanceText);
        } else {

          // Effectively above or below airspace, steep climb or descent
          // necessary to enter

          Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
          if (pAS.vDistance > 0) {
            wsprintf(sTmp, TEXT("< %c %s ab %s"),
                     sAckIndicator[pAS.Acknowledge],
                     sType, DistanceText);
          } else {
            wsprintf(sTmp, TEXT("< %c %s bl %s"),
                     sAckIndicator[pAS.Acknowledge], sType,
                     DistanceText);
          }
        }
      }
    }

    canvas.text_clipped(IBLSCALE(Col0Left), IBLSCALE(TextTop + TextHeight),
                        rcTextClip, sTmp);

  } else {
    if (DrawListIndex == 0){
      _stprintf(sTmp, TEXT("%s"), gettext(TEXT("No Warnings")));
      canvas.text_opaque(IBLSCALE(2), IBLSCALE(2), sTmp);
    }
  }
}

static void OnAirspaceListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    if (FocusedIdx < 0) {
      FocusedIdx = 0;
    }
    ListInfo->ItemIndex = FocusedIdx;
    ListInfo->ItemCount = max(1,Count);

    ListInfo->DrawIndex = 0;
    ListInfo->ScrollIndex = 0; // JMW bug fix
    DrawListIndex = 0;

  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    FocusedIdx = ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}



bool actShow = false;
bool actListSizeChange = false;
bool actListChange = false;

static bool FindFocus() {
  bool do_refocus = false;

  FocusedIdx = 0;
  FocusedIdx = AirspaceWarnFindIndexByID(FocusedID);
  if (FocusedIdx < 0) {
    FocusedIdx = 0;
    FocusedID = -1; // JMW bug fix

    if (wAirspaceListEntry->GetFocused()) {
      // JMW attempt to find fix...
      do_refocus = true;
    }
  }
  SelectedIdx = AirspaceWarnFindIndexByID(SelectedID);
  if (SelectedIdx < 0){
    SelectedID = -1;
  }
  return do_refocus;
}


int UserMsgNotify(WindowControl *Sender, MSG *msg){

  if (msg->message != WM_USER+1)
    return(1);

  if (!wf->GetVisible())
    return(0);

  bool do_refocus = false;

  if (actListSizeChange){
    actListSizeChange = false;

    Count = AirspaceWarnGetItemCount();

    do_refocus = FindFocus();

    wAirspaceList->ResetList();

    if (Count == 0) {
      // auto close
      OnCloseClicked(Sender);
    }
  }

  if (actShow){
    actShow = false;
    if (!do_refocus) {
      do_refocus = FindFocus();
    }

    /*
    if (!wf->GetVisible()){
      Count = AirspaceWarnGetItemCount();
      wAirspaceList->ResetList();
      FocusedIdx = 0;
      FocusedID = -1;
      wf->Show();
      SetFocus(wAirspaceListEntry->GetHandle());
    } else {
      SetFocus(wAirspaceListEntry->GetHandle());
    }
    */
    //    return(0);
  }

  if (actListChange) {
    actListChange = false;
    wAirspaceList->Redraw();
  }

  if (do_refocus) {
    SetFocus(wAirspaceListEntry->GetHandle());
  }

  // this is our message, we have handled it.
  return(0);
}


// WARNING: this is NOT called from the windows thread!
void AirspaceWarningNotify(AirspaceWarningNotifyAction_t Action,
                           AirspaceInfo_c *AirSpace) {
  (void)AirSpace;
  if ((Action == asaItemAdded) || (Action == asaItemRemoved)
      || (Action == asaWarnLevelIncreased)) {
    actShow = true;
  }

  if ((Action == asaItemAdded) || (Action == asaItemRemoved)
      || (Action == asaClearAll)) {
    actListSizeChange = true;
  }

  if ((Action == asaItemChanged) || (Action == asaWarnLevelIncreased)){
    actListChange = true;
  }

  if ((Action == asaProcessEnd) && (actShow || actListSizeChange || actListChange)){
    if (fDialogOpen) {
      PostMessage(wf->GetHandle(), WM_USER+1, 0, 0);
    }
    else {
      airspaceWarningEvent.trigger();
      // JMW this was bad! PostMessage(hWndMapWindow, WM_USER+1, 0, 0);
      // (Makes it serviced by the main gui thread, much better)
    }
    // sync dialog with MapWindow (event processing etc)
  }

}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAckClicked),
  DeclareCallBackEntry(OnAck1Clicked),
  DeclareCallBackEntry(OnAck2Clicked),
  DeclareCallBackEntry(OnEnableClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnAirspaceListInfo),
  DeclareCallBackEntry(OnAirspaceListItemPaint),
  DeclareCallBackEntry(NULL)
};

/*
bool dlgAirspaceWarningShow(void){
  if (Count == 0)
    return(false);
  actShow = true;
  PostMessage(wf->GetHandle(), WM_USER+1, 0, 0);
  return(true);
}
*/

bool dlgAirspaceWarningIsEmpty(void) {
  return (AirspaceWarnGetItemCount()==0);
}

// WARING: may only be called from MapWindow event loop!
// JMW this is now called from ProcessCommon (main GUI loop)

bool dlgAirspaceWarningVisible(void) {
  return fDialogOpen;
}

bool dlgAirspaceWarningShowDlg(bool Force){

  if (fDialogOpen)
    return(false);

  if (!actShow && !Force)
    return false;

  Count = AirspaceWarnGetItemCount();

  if (Count == 0)
    return(false);

  assert(wf != NULL);
  assert(wAirspaceList != NULL);

  wAirspaceList->ResetList();

  if (!fDialogOpen) {
    fDialogOpen = true;
    HWND oldFocusHwnd = GetFocus();
    wf->ShowModal();
    if (oldFocusHwnd) {
      SetFocus(oldFocusHwnd);
    }

    fDialogOpen = false;

    // JMW need to deselect everything on new reopening of dialog
    SelectedID = -1;
    SelectedIdx = -1;
    FocusedID = -1;
    FocusedIdx = -1;

    //    SetFocus(hWndMapWindow);
    // JMW why do this? --- not necessary?
    // concerned it may interfere if a dialog is already open
  }

  return(true);
}


int dlgAirspaceWarningInit(void){

  int res = 0;

#ifdef HAVEEXCEPTIONS
  __try{
#endif

//    hActiveWindow = GetActiveWindow();

    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgAirspaceWarning.xml"),
		        main_window,
		        TEXT("IDR_XML_AIRSPACEWARNING"));

    if (wf) {

      wf->SetKeyDownNotify(OnKeyDown);
      wf->SetUserMsgNotify(UserMsgNotify);
      wf->SetTimerNotify(OnTimer);

      hBrushInsideBk.set(Color(254,50,50));
      hBrushNearBk.set(Color(254,254,50));
      hBrushInsideAckBk.set(Color(254,100,100));
      hBrushNearAckBk.set(Color(254,254,100));

      wAirspaceList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceWarningList"));
      wAirspaceListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmAirspaceWarningListEntry"));
      wAirspaceListEntry->SetCanFocus(true);

      AirspaceWarnListAddNotifier(AirspaceWarningNotify);

      wf->Close();  // hide the window

    }


#ifdef HAVEEXCEPTIONS
  }__except(EXCEPTION_EXECUTE_HANDLER ){

    res = 0;
    // ToDo: log that problem

  };
#endif

  return(res);

}

int dlgAirspaceWarningDeInit(void){

  if (wf)
    wf->SetVisible(false);

  AirspaceWarnListRemoveNotifier(AirspaceWarningNotify);

  delete wf;
  wf = NULL;

  return(0);

}

