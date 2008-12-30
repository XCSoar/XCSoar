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


#include "StdAfx.h"
#include <aygshell.h>

#include "XCSoar.h"

#include "externs.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "Utils.h"

#define MAXTITLE 200
#define MAXDETAILS 5000

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndOwnerDrawFrame *wDetailsEntry = NULL;

#define MAXLINES 100
#define MAXLISTS 20
static int LineOffsets[MAXLINES];
static int DrawListIndex=0;
static int nTextLines=0;
static int nLists=0;
static TCHAR *ChecklistText[MAXLISTS];
static TCHAR *ChecklistTitle[MAXLISTS];

static void Update(){

  //wDetails->ResetList();
  wDetails->Redraw();

}


static void OnPaintDetailsListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  if (DrawListIndex < FLARM_MAX_TRAFFIC){
    TCHAR tmp[100];
    TCHAR text[100];

    double range;
    double bear;

    LockFlightData();

    DistanceBearing(GPS_INFO.Latitude,
		    GPS_INFO.Longitude,
		    GPS_INFO.FLARM_Traffic[DrawListIndex].Latitude,
		    GPS_INFO.FLARM_Traffic[DrawListIndex].Longitude,
		    &range,
		    &bear);

    wsprintf(tmp, TEXT("%3s %3ld %+3.1lf %5ld"),
	     GPS_INFO.FLARM_Traffic[DrawListIndex].Name,
	     (int)(SPEEDMODIFY * GPS_INFO.FLARM_Traffic[DrawListIndex].Speed),
#ifdef FLARM_AVERAGE
	     LIFTMODIFY * GPS_INFO.FLARM_Traffic[DrawListIndex].Average30s,
#else
	     0.0,
#endif
	     (int)(ALTITUDEMODIFY * GPS_INFO.FLARM_Traffic[DrawListIndex].Altitude)
	     );
    wsprintf(text, TEXT("%s %3.0lf %2.1lf"),
	     tmp,
	     bear,
	     (DISTANCEMODIFY * range));

    int txtLen = wcslen(text);
    if (txtLen>0 && GPS_INFO.FLARM_Traffic[DrawListIndex].ID != 0)
      {
	ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		   ETO_OPAQUE, NULL,
		   text,
		   txtLen,
		   NULL);
      }
    UnlockFlightData();
  }
}

int GetActiveFlarmTrafficCount()
{
  int count = 0;
  for (int i=0; i<FLARM_MAX_TRAFFIC; i++)
    {
      if (GPS_INFO.FLARM_Traffic[i].ID!=0)
	{
	  count++;
	}
    }
  return count;
}

static void OnDetailsListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = GetActiveFlarmTrafficCount();
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    if (DrawListIndex != -1)
      {
	if (GPS_INFO.FLARM_Traffic[DrawListIndex].ID != 0)
	  {
	    if (LookupFLARMDetails(GPS_INFO.FLARM_Traffic[DrawListIndex].ID) == NULL)
	      {
		// not existing en primary or secondary flarm id list
		((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetCaption(TEXT("Set CN"));
		((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetVisible(true);
	      }
	    else
	      {
		// the id was found - is it from secondary list ?
		int index = LookupSecondaryFLARMId(GPS_INFO.FLARM_Traffic[DrawListIndex].ID);

		if (index != -1)
		  {
		    ((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetCaption(TEXT("Edit CN"));
		    ((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetVisible(true);
		  }
		else
		  {
		    ((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetVisible(false);
		  }
	      }
	    ((WndButton *)wf->FindByName(TEXT("cmdTrack")))->SetVisible(true);
	  }
	else
	  {
	    ((WndButton *)wf->FindByName(TEXT("cmdTrack")))->SetVisible(false);
	    ((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetVisible(false);
	  }
      }
  }
}

void SelectAsTeamTrack()
{
  int index = wDetails->GetItemIndex();
  if (index != -1)
    {
      if (GPS_INFO.FLARM_Traffic[index].Name[0] == 0)
	{
	  TeamFlarmCNTarget[0] = 0;
	}
      else
	{
	  // copy the 3 first chars from the name
	  for (int z = 0; z < 3; z++)
	    {
	      TeamFlarmCNTarget[z] = GPS_INFO.FLARM_Traffic[index].Name[z];
	    }
	  TeamFlarmCNTarget[3] = 0;
	}
      // now tracking !
      TeamFlarmIdTarget = GPS_INFO.FLARM_Traffic[index].ID;
      TeamFlarmTracking = true;
      TeammateCodeValid = false;
    }
}

static void OnTrackClicked(WindowControl * Sender)
{
  (void)Sender;
  SelectAsTeamTrack();
  wf->SetModalResult(mrOK);
}

static void OnSetCNClicked(WindowControl * Sender)
{
  (void)Sender;

  int index = wDetails->GetItemIndex();
  if (index != -1)
    {
      TCHAR newName[21];
      newName[0] = 0;
      dlgTextEntryShowModal(newName, 4);

      AddFlarmLookupItem(GPS_INFO.FLARM_Traffic[index].ID, newName, true);
    }
}


static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)lParam;
  (void)Sender;
  switch(wParam & 0xffff){
  case VK_LEFT:
  case '6':
    SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
    //      NextPage(-1);
    //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
  case VK_RIGHT:
  case '7':
    SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
    //      NextPage(+1);
    //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  Update();
  return 0;
}

static void OnListEnter(WindowControl * Sender,
			WndListFrame::ListInfo_t *ListInfo)
{
  SelectAsTeamTrack();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnTrackClicked),
  DeclareCallBackEntry(OnSetCNClicked),
  DeclareCallBackEntry(OnPaintDetailsListItem),
  DeclareCallBackEntry(OnDetailsListInfo),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};






void dlgFlarmTrafficShowModal(void){
  static bool first=true;
  if (first) {

    first=false;
  }

  if (InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgFlarmTraffic_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
			filename,
			hWndMainWindow,
			TEXT("IDR_XML_FLARMTRAFFIC_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgFlarmTraffic.xml"));
    wf = dlgLoadFromXML(CallBackTable,
			filename,
			hWndMainWindow,
			TEXT("IDR_XML_FLARMTRAFFIC"));
  }

  nTextLines = 0;

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wDetails = (WndListFrame*)wf->FindByName(TEXT("frmDetails"));
  wDetails->SetEnterCallback(OnListEnter);
  ASSERT(wDetails!=NULL);

  wDetailsEntry =
    (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmDetailsEntry"));
  ASSERT(wDetailsEntry!=NULL);
  wDetailsEntry->SetCanFocus(true);


  wDetails->SetBorderKind(BORDERLEFT);

  page = 0;

  wDetails->ResetList();
  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

