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


#include "StdAfx.h"
#include "XCSoar.h"
#include "Utils.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "externs.h"


/*
void dosomethingwithteamcode() {
  if (wcslen(codeText) >= 3)
    {
      memset (TeammateCode, 0, sizeof(TCHAR[10]));
      GetDlgItemText(hDlg, IDC_TEAMCODE_TEXTBOX, TeammateCode, 5);

static WndForm *wf=NULL;
      
      teammateBearing = GetTeammateBearingFromRef(TeammateCode);
      teammateRange = GetTeammateRangeFromRef(TeammateCode);
      
      double destLat;
      double destLong;
      
      xXY_to_LL(WayPointList[TeamCodeRefWaypoint].Latitude, 
                WayPointList[TeamCodeRefWaypoint].Longitude,
                teammateBearing,
                teammateRange,
                &TeammateLatitude,
                &TeammateLongitude);
      
      TeammateCodeValid = true;
    }
  else
    {
      TeammateCodeValid = false;
    }
}    
*/


static WndForm *wf=NULL;
static WndButton *buttonCode=NULL;      

#include "TeamCodeCalculation.h"

static void Update() {

  double teammateBearing = CALCULATED_INFO.TeammateBearing;
  double teammateRange = CALCULATED_INFO.TeammateRange;

  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpBearing"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(teammateBearing);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(teammateRange*DISTANCEMODIFY);
    wp->RefreshDisplay();
  }

  TCHAR Text[100];
  wp = (WndProperty*)wf->FindByName(TEXT("prpCode"));
  if (wp) {
    _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
    Text[5] = '\0';
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

  _stprintf(Text,TEXT("Mate code: %s"), TeammateCode);
  buttonCode->SetCaption(Text);
}


static void OnCodeClicked(WindowControl *Sender) {
	(void)Sender;
  if (buttonCode) {
    dlgTextEntryShowModal(TeammateCode, 7);
  }
  int i= _tcslen(TeammateCode)-1;
  while (i>=0) {
    if (TeammateCode[i]!=_T(' ')) {
      break;
    }
    TeammateCode[i]=0;
    i--;
  };

  if (_tcslen(TeammateCode)>0) {
    TeammateCodeValid = true;
  }
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static int OnTimerNotify(WindowControl * Sender) {
	(void)Sender;
  Update();
  return 0;
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};


void dlgTeamCodeShowModal(void) {

  wf = NULL;
  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgTeamCode.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
		      
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_TEAMCODE"));
  if (!wf) return;

  buttonCode = ((WndButton *)wf->FindByName(TEXT("cmdCode")));
  if (buttonCode) {
    buttonCode->SetOnClickNotify(OnCodeClicked);
  }

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
  }

  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;

}
