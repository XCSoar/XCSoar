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

#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "InfoBoxLayout.h"
#include "UtilsFLARM.hpp"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"

#include <assert.h>

#define MAXTITLE 200
#define MAXDETAILS 5000

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndOwnerDrawFrame *wDetailsEntry = NULL;

#define MAXLINES 100
#define MAXLISTS 20
static int DrawListIndex=0;
static int nTextLines=0;

static void Update(){

  //wDetails->ResetList();
  wDetails->Redraw();

}


static void
OnPaintDetailsListItem(WindowControl *Sender, Canvas &canvas)
{
  (void)Sender;
  if (DrawListIndex < FLARM_MAX_TRAFFIC){
    TCHAR tmp[100];
    TCHAR text[100];

    double range;
    double bear;

    DistanceBearing(XCSoarInterface::Basic().Location,
		    XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].Location,
		    &range,
		    &bear);

    wsprintf(tmp, TEXT("%3s %3ld %+3.1lf %5ld"),
	     XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].Name,
	     (int)(SPEEDMODIFY * XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].Speed),
#ifdef FLARM_AVERAGE
	     LIFTMODIFY * XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].Average30s,
#else
	     0.0,
#endif
	     (int)(ALTITUDEMODIFY * XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].Altitude)
	     );
    wsprintf(text, TEXT("%s %3.0lf %2.1lf"),
	     tmp,
	     bear,
	     (DISTANCEMODIFY * range));

    if (XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].ID != 0)
      canvas.text_opaque(2 * InfoBoxLayout::scale, 2 * InfoBoxLayout::scale,
                         text);
  }
}

int GetActiveFlarmTrafficCount()
{
  int count = 0;
  for (int i=0; i<FLARM_MAX_TRAFFIC; i++)
    {
      if (XCSoarInterface::Basic().FLARM_Traffic[i].ID!=0)
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
	if (XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].ID != 0)
	  {
	    if (LookupFLARMDetails(XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].ID) == NULL)
	      {
		// not existing en primary or secondary flarm id list
		((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetCaption(TEXT("Set CN"));
		((WndButton *)wf->FindByName(TEXT("cmdSetCN")))->SetVisible(true);
	      }
	    else
	      {
		// the id was found - is it from secondary list ?
		int index = LookupSecondaryFLARMId(XCSoarInterface::Basic().FLARM_Traffic[DrawListIndex].ID);

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
      if (XCSoarInterface::Basic().FLARM_Traffic[index].Name[0] == 0)
	{
	  XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
	}
      else
	{
	  // copy the 3 first chars from the name
	  for (int z = 0; z < 3; z++)
	    {
	      XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[z] = XCSoarInterface::Basic().FLARM_Traffic[index].Name[z];
	    }
	  XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[3] = 0;
	}
      // now tracking !
      XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = XCSoarInterface::Basic().FLARM_Traffic[index].ID;
      XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = true;
      XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;
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

      AddFlarmLookupItem(XCSoarInterface::Basic().FLARM_Traffic[index].ID, newName, true);
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
    ((WndButton *)wf->FindByName(TEXT("cmdPrev")))->set_focus();
    //      NextPage(-1);
    //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
  case VK_RIGHT:
  case '7':
    ((WndButton *)wf->FindByName(TEXT("cmdNext")))->set_focus();
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
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgFlarmTraffic_L.xml"),
			XCSoarInterface::main_window,
			TEXT("IDR_XML_FLARMTRAFFIC_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgFlarmTraffic.xml"),
			XCSoarInterface::main_window,
			TEXT("IDR_XML_FLARMTRAFFIC"));
  }

  nTextLines = 0;

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wDetails = (WndListFrame*)wf->FindByName(TEXT("frmDetails"));
  wDetails->SetEnterCallback(OnListEnter);
  assert(wDetails!=NULL);

  wDetailsEntry =
    (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmDetailsEntry"));
  assert(wDetailsEntry!=NULL);
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

