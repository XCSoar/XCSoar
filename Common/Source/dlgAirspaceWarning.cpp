/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#if (NEWINFOBOX>0)

#include "stdafx.h"

#include "InfoBoxLayout.h"

#include "externs.h"
#include "units.h"
#include "Airspace.h"
#include "MapWindow.h"

#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;
static WndOwnerDrawFrame *wAirspaceListEntry = NULL;
static HBRUSH hBrushInsideBk;
static HBRUSH hBrushNearBk;
static HBRUSH hBrushInsideAckBk;
static HBRUSH hBrushNearAckBk;
//static HWND   hActiveWindow;

static int Count=0;
static int ItemIndex=-1;
static int DrawListIndex=-1;
static int FocusedID = -1;     // Currently focused airspace ID
static int FocusedIdx = -1;    // Currently socused airspace List Index
static int SelectedID = -1;    // Currently selected airspace ID
static int SelectedIdx = -1;   // Currently selected airspace List Index

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

  if (AirspaceWarnGetItem(Idx, &pAS)){
    AirspaceWarnDoAck(pAS.ID, Ack);
    wAirspaceList->Redraw();
  }

}

static void OnAckClicked(WindowControl * Sender){
  DoAck(3);
}

static void OnAck1Clicked(WindowControl * Sender){
  DoAck(-1);
}

static void OnAck2Clicked(WindowControl * Sender){
  DoAck(4);
}

static void OnEnableClicked(WindowControl * Sender){
  DoAck(0);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetVisible(false);
  MapWindow::RequestFastRefresh();

  SetFocus(hWndMainWindow);

}

static int OnTimer(WindowControl * Sender){
  return(0);
}

static int OnKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
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
  }

  return(1);
  
}

static void OnDistroy(WindowControl * Sender){

  AirspaceWarnListRemoveNotifier(AirspaceWarningNotify);
  DeleteObject(hBrushInsideBk);
  DeleteObject(hBrushNearBk);
  DeleteObject(hBrushInsideAckBk);
  DeleteObject(hBrushNearAckBk);

  delete wf;
  wf = NULL;

}

 static TCHAR * getAirspaceType(int Type){
   switch (Type)
    {
    case RESTRICT:
      return(TEXT("LxR"));
    case PROHIBITED:
      return(TEXT("LxP"));
    case DANGER:
      return(TEXT("LxD"));
    case CLASSA:
      return(TEXT("A"));
    case CLASSB:
      return(TEXT("B"));
    case CLASSC:
      return(TEXT("C"));
    case CLASSD:
      return(TEXT("D"));
    case CLASSE:
      return(TEXT("E"));
    case CLASSF:
      return(TEXT("F"));
    case NOGLIDER:
      return(TEXT("NoGld"));
    case CTR:
      return(TEXT("CTR"));
    case WAVE:
      return(TEXT("Wav"));
    default:
      return(TEXT("?"));
    }
}

static TCHAR *fmtAirspaceAlt(TCHAR *Buffer, AIRSPACE_ALT *alt){

  switch (alt->Base){
    case abUndef:
      _stprintf(Buffer, TEXT("%.0fm %.0fft"), alt->Altitude, alt->Altitude*TOFEET);
    break;
    case abMSL:
      _stprintf(Buffer, TEXT("%.0fm %.0fft MSL"), alt->Altitude, alt->Altitude*TOFEET);
    break;
    case abAGL:
      if (alt->Altitude == 0)
        _stprintf(Buffer, TEXT("SFC"));
      else
        _stprintf(Buffer, TEXT("%.0fm %.0fft AGL"), alt->Altitude, alt->Altitude*TOFEET);
    break;
    case abQNH:
      _stprintf(Buffer, TEXT("%.0fm %.0fft STD"), alt->Altitude, alt->Altitude*TOFEET);
    break;
    case abFL:
      _stprintf(Buffer, TEXT("FL %.0f %.0fm"), alt->FL, AltitudeToQNHAltitude(alt->Altitude));
    break;
  }
  return(Buffer);
}

static void OnAirspaceListItemPaint(WindowControl * Sender, HDC hDC){
  TCHAR sTmp[128];

  if (Count != 0){

    TCHAR sAckIndicator[6] = TEXT(" -++*");
    TCHAR sName[21];
    TCHAR sTop[32];
    TCHAR sBase[32];
    TCHAR *pType;
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
    HBRUSH       hBrushBk;


    CopyRect(&rc, Sender->GetBoundRect());
    CopyRect(&rcTextClip, Sender->GetBoundRect());
    rcTextClip.right = Col1Left - 2;

    InflateRect(&rc, -2, -2);

    if (!AirspaceWarnGetItem(i, &pAS)) return;

    if (ItemIndex == DrawListIndex){
      FocusedID = pAS.ID;
    }

    if (pAS.IsCircle){
      _tcsncpy(sName, AirspaceCircle[pAS.AirspaceIndex].Name, sizeof(sName)/sizeof(sName[0]));
      Base = AirspaceCircle[pAS.AirspaceIndex].Base;
      Top  = AirspaceCircle[pAS.AirspaceIndex].Top;
      Type = AirspaceCircle[pAS.AirspaceIndex].Type;
    } else {
      _tcsncpy(sName, AirspaceArea[pAS.AirspaceIndex].Name, sizeof(sName)/sizeof(sName[0]));
      Base = AirspaceArea[pAS.AirspaceIndex].Base;
      Top  = AirspaceArea[pAS.AirspaceIndex].Top;
      Type = AirspaceArea[pAS.AirspaceIndex].Type;
    }

    sName[sizeof(sName)/sizeof(sName[0])-1] = '\0';

    fmtAirspaceAlt(sTop, &Top);
    fmtAirspaceAlt(sBase, &Base);
    pType = getAirspaceType(Type);

    if (pAS.Inside){
      if (pAS.Acknowledge >= 2)
        hBrushBk = hBrushInsideAckBk;
      else
        hBrushBk = hBrushInsideBk;
    } else { if ((pAS.hDistance < 2500) && (abs(pAS.vDistance) < 250))
      if (pAS.Acknowledge >= 1)
        hBrushBk = hBrushNearAckBk;
      else
        hBrushBk = hBrushNearBk;
    }

    
    if (SelectedIdx == DrawListIndex){
      InflateRect(&rc, 1, 1);
      SelectObject(hDC, (HPEN)GetStockObject(BLACK_PEN));
      Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    } else
      FillRect(hDC, &rc, hBrushBk);

    if ((pAS.Acknowledge > 0) && (pAS.Acknowledge >= pAS.WarnLevel)){
      SetTextColor(hDC, clGray);
    }

    wsprintf(sTmp, TEXT("%-20s%d"), sName, pAS.WarnLevel - pAS.Acknowledge);

    ExtTextOut(hDC, Col0Left*InfoBoxLayout::scale, TextTop*InfoBoxLayout::scale,
      ETO_CLIPPED, &rcTextClip, sTmp, _tcslen(sTmp), NULL);

    wsprintf(sTmp, TEXT("%-20s"), sTop);

    ExtTextOut(hDC, Col1Left*InfoBoxLayout::scale, TextTop*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL, sTmp, _tcslen(sTmp), NULL);

    wsprintf(sTmp, TEXT("%-20s"), sBase);

    ExtTextOut(hDC, Col1Left*InfoBoxLayout::scale, (TextTop+TextHeight)*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL, sTmp, _tcslen(sTmp), NULL);

    if (pAS.Inside){
      wsprintf(sTmp, TEXT("> %c %s"), sAckIndicator[pAS.Acknowledge], pType);
    } else {
      if (pAS.hDistance == 0 && pAS.vDistance > 0){
        wsprintf(sTmp, TEXT("< %c %s ab %dm"), sAckIndicator[pAS.Acknowledge], pType, (int)pAS.vDistance);
      }
      else if (pAS.hDistance == 0 && pAS.vDistance < 0){
        wsprintf(sTmp, TEXT("< %c %s bl %dm"), sAckIndicator[pAS.Acknowledge], pType, (int)-pAS.vDistance);
      } else {
        if ((pAS.vDistance == 0) || (pAS.hDistance < (abs(pAS.vDistance)*30)))
          wsprintf(sTmp, TEXT("< %c %s H %dm"), sAckIndicator[pAS.Acknowledge], pType, (int)pAS.hDistance);
        else
          if (pAS.vDistance > 0)
            wsprintf(sTmp, TEXT("< %c %s ab %dm"), sAckIndicator[pAS.Acknowledge], pType, (int)pAS.vDistance);
          else
            wsprintf(sTmp, TEXT("< %c %s bl %dm"), sAckIndicator[pAS.Acknowledge], pType, (int)pAS.vDistance);
      }
    }

    ExtTextOut(hDC, Col0Left*InfoBoxLayout::scale, (TextTop+TextHeight)*InfoBoxLayout::scale,
      ETO_CLIPPED, &rcTextClip, sTmp, _tcslen(sTmp), NULL);


  } else {
    if (DrawListIndex == 0){
      _stprintf(sTmp, TEXT("%s"), gettext(TEXT("No Warnings")));
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
        ETO_OPAQUE, NULL,
        sTmp, _tcslen(sTmp), NULL);
    }
  }
}

static void OnAirspaceListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemIndex = FocusedIdx;
    if (Count == 0)
      ListInfo->ItemCount = 1;
    else
      ListInfo->ItemCount = Count;

  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    FocusedIdx = ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}



bool actShow = false;
bool actListSizeChange = false;
bool actListChange = false;

int UserMsgNotify(WindowControl *Sender, MSG *msg){

  if (msg->message != WM_USER+1)
    return(1);

  if (actShow){
    actShow = false;
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

    return(0);

  }

  if (!wf->GetVisible())
    return(0);

  if (actListSizeChange){
    actListSizeChange = false;

    Count = AirspaceWarnGetItemCount();

    FocusedIdx = 0;
    if (FocusedID >= 0){
      FocusedIdx = AirspaceWarnFindIndexByID(FocusedID);
      if (FocusedIdx < 0)
        FocusedIdx = 0;
      SelectedIdx = AirspaceWarnFindIndexByID(SelectedID);
      if (SelectedIdx < 0){
        SelectedID = -1;
      }

    }

    wAirspaceList->ResetList();

    if (Count == 0 && wf->GetVisible()){
      OnCloseClicked(Sender);
    }

  }

  if (actListChange){
    actListChange = false;

    wAirspaceList->Redraw();
  }

  return(1);

}

// WARNING: this is NOT called from the windows thread!
void AirspaceWarningNotify(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace) {

  if (Action == asaItemAdded || Action == asaItemRemoved || Action == asaClearAll) {
    actShow = true;
    actListSizeChange = true;
  }

  if (Action == asaWarnLevelIncreased){
    actShow = true;
    actListChange = true;
  }

  if (Action == asaItemChanged){
    actListChange = true;
  }

  if (Action == asaProcessEnd){
    PostMessage(wf->GetHandle(), WM_USER+1, 0, 0);
  }

}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAckClicked),
  DeclearCallBackEntry(OnAck1Clicked),
  DeclearCallBackEntry(OnAck2Clicked),
  DeclearCallBackEntry(OnEnableClicked),
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(OnAirspaceListInfo),
  DeclearCallBackEntry(OnAirspaceListItemPaint),
  DeclearCallBackEntry(NULL)
};

bool dlgAirspaceWarningShow(void){
  if (Count == 0)
    return(false);
  actShow = true;
  PostMessage(wf->GetHandle(), WM_USER+1, 0, 0);

  return(true);
}

int dlgAirspaceWarningInit(void){

  int res = 0;

  __try{

//    hActiveWindow = GetActiveWindow();

    wf = dlgLoadFromXML(CallBackTable,
		        LocalPathS(TEXT("dlgAirspaceWarning.xml")),
		        hWndMainWindow,
		        TEXT("IDR_XML_AIRSPACEWARNING"));

    if (wf) {

      wf->SetKeyDownNotify(OnKeyDown);
      wf->SetUserMsgNotify(UserMsgNotify);
      wf->SetTimerNotify(OnTimer);

      hBrushInsideBk = (HBRUSH)CreateSolidBrush(RGB(254,50,50));
      hBrushNearBk = (HBRUSH)CreateSolidBrush(RGB(254,254,50));
      hBrushInsideAckBk = (HBRUSH)CreateSolidBrush(RGB(254,100,100));
      hBrushNearAckBk = (HBRUSH)CreateSolidBrush(RGB(254,254,100));

      wAirspaceList = (WndListFrame*)wf->FindByName(TEXT("frmAirspaceWarningList"));
      wAirspaceListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmAirspaceWarningListEntry"));
      wAirspaceListEntry->SetCanFocus(true);

      AirspaceWarnListAddNotifier(AirspaceWarningNotify);

      wf->Close();  // hide the window

    }


  }__except(EXCEPTION_EXECUTE_HANDLER ){

    res = 0;
    // ToDo: log that problem

  };

  return(res);

}

int dlgAirspaceWarningDeInit(void){

  wf->SetVisible(false);

  AirspaceWarnListRemoveNotifier(AirspaceWarningNotify);

  delete wf;

  wf = NULL;

  return(0);
  
}


#endif
