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
#include "XCSoar.h"
#include "Utils.h"
#include "dlgTools.h"
#include "externs.h"
#include "InfoBoxLayout.h"

static WndForm *wf=NULL;


#include "TeamCodeCalculation.h"

static void Update()
{
  WndProperty* wp;
  TCHAR Text[100];
  double teammateBearing = CALCULATED_INFO.TeammateBearing;
  double teammateRange = CALCULATED_INFO.TeammateRange;

  if((TeamCodeRefWaypoint >=0)&&(WayPointList))
    {
      double Value = CALCULATED_INFO.TeammateBearing -  GPS_INFO.TrackBearing;

      if (Value < -180.0)
        Value += 360.0;
      else
        if (Value > 180.0)
          Value -= 360.0;

      if (Value > 1)
        _stprintf(Text, TEXT("%2.0f")TEXT(DEG)TEXT(">"), Value);
      else if (Value < -1)
        _stprintf(Text, TEXT("<%2.0f")TEXT(DEG), -Value);
      else
        _tcscpy(Text, TEXT("<>"));

    } else {
    _tcscpy(Text, TEXT("---"));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRelBearing"));
  if (wp) {
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

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

  wp = (WndProperty*)wf->FindByName(TEXT("prpOwnCode"));
  if (wp) {
    _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
    Text[5] = '\0';
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMateCode"));
  if (wp) {
    wp->SetText(TeammateCode);
    wp->RefreshDisplay();
  }
}


static void OnCodeClicked(WindowControl *Sender)
{
  TCHAR newTeammateCode[10];
  _tcsncpy(newTeammateCode, TeammateCode, 10);
  dlgTextEntryShowModal(newTeammateCode, 7);

  int i= _tcslen(newTeammateCode)-1;
  while (i>=0) {
    if (newTeammateCode[i]!=_T(' '))
      {
	break;
      }
    newTeammateCode[i]=0;
    i--;
  };


  _tcsncpy(TeammateCode, newTeammateCode, 10);
  if (_tcslen(TeammateCode)>0)
    {
      TeammateCodeValid = true;
    }
}

static void OnFlarmLockClicked(WindowControl * Sender)
{
  (void)Sender;

  dlgTextEntryShowModal(TeamFlarmCNTarget, 4);

  TeammateCodeValid = false;

  int flarmId = LookupFLARMDetails(TeamFlarmCNTarget);

  if (flarmId == 0)
    {
      MessageBoxX(gettext(TEXT("Unknown Competition Number")),
		  gettext(TEXT("Not Found")),
		  MB_OK|MB_ICONINFORMATION);

      TeamFlarmTracking = false;
      TeamFlarmIdTarget = 0;
      TeamFlarmCNTarget[0] = 0;
    }
  else
    {
      TeamFlarmIdTarget = flarmId;
      TeamFlarmTracking = true;
    }
}

static void OnCloseClicked(WindowControl * Sender)
{
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
  DeclareCallBackEntry(OnFlarmLockClicked),
  DeclareCallBackEntry(NULL)
};


void dlgTeamCodeShowModal(void)
{
  WndProperty* wp = NULL;
  WndButton *buttonCode = NULL;
  wf = NULL;

  if (InfoBoxLayout::landscape)
    {
      wf = dlgLoadFromXML(CallBackTable,
                          TEXT("dlgTeamCode_L.xml"),
			  hWndMainWindow,
			  TEXT("IDR_XML_TEAMCODE_L"));
      if (!wf) return;
    }
  else
    {
      wf = dlgLoadFromXML(CallBackTable,
                          TEXT("dlgTeamCode.xml"),
			  hWndMainWindow,
			  TEXT("IDR_XML_TEAMCODE"));
      if (!wf) return;
    }

  // set event for button
  buttonCode = ((WndButton *)wf->FindByName(TEXT("cmdSetCode")));
  if (buttonCode) {
    buttonCode->SetOnClickNotify(OnCodeClicked);
  }

  // Set unit for range
  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
  }

  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;

}
