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
#include "UtilsFLARM.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "InfoBoxLayout.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"

static WndForm *wf=NULL;


#include "TeamCodeCalculation.h"

static void Update()
{
  WndProperty* wp;
  TCHAR Text[100];
  double teammateBearing = XCSoarInterface::Calculated().TeammateBearing;
  double teammateRange = XCSoarInterface::Calculated().TeammateRange;

  if (XCSoarInterface::SettingsComputer().TeamCodeRefWaypoint >= 0) {
      double Value = XCSoarInterface::Calculated().TeammateBearing -  XCSoarInterface::Basic().TrackBearing;

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
    _tcsncpy(Text,XCSoarInterface::Calculated().OwnTeamCode,5);
    Text[5] = '\0';
    wp->SetText(Text);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMateCode"));
  if (wp) {
    wp->SetText(XCSoarInterface::SettingsComputer().TeammateCode);
    wp->RefreshDisplay();
  }
}


static void OnCodeClicked(WindowControl *Sender)
{
  TCHAR newTeammateCode[10];
  _tcsncpy(newTeammateCode, XCSoarInterface::SettingsComputer().TeammateCode, 10);
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

  _tcsncpy(XCSoarInterface::SetSettingsComputer().TeammateCode, newTeammateCode, 10);
  if (_tcslen(XCSoarInterface::SettingsComputer().TeammateCode)>0) {
    XCSoarInterface::SetSettingsComputer().TeammateCodeValid = true;
  }
}

static void OnFlarmLockClicked(WindowControl * Sender)
{
  (void)Sender;

  dlgTextEntryShowModal(XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget, 4);

  XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;

  int flarmId = LookupFLARMDetails(XCSoarInterface::SettingsComputer().TeamFlarmCNTarget);

  if (flarmId == 0)
    {
      MessageBoxX(gettext(TEXT("Unknown Competition Number")),
		  gettext(TEXT("Not Found")),
		  MB_OK|MB_ICONINFORMATION);

      XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = false;
      XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = 0;
      XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
    }
  else
    {
      XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = flarmId;
      XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = true;
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
			  XCSoarInterface::main_window,
			  TEXT("IDR_XML_TEAMCODE_L"));
      if (!wf) return;
    }
  else
    {
      wf = dlgLoadFromXML(CallBackTable,
                          TEXT("dlgTeamCode.xml"),
			  XCSoarInterface::main_window,
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
