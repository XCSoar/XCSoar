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
#include "externs.h"
#include "Units.h"
#include "device.h"
#include "InputEvents.h"
#include "WindowControls.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;


//
//

extern NMEA_INFO GPS_INFO;

static void UpdateValues() {
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapLanding"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.FlapLanding) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.FlapLanding);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirbrakeExtended"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.AirbrakeLocked) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.AirbrakeLocked);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapPositive"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.FlapPositive) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.FlapPositive);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapNeutral"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.FlapNeutral) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.FlapNeutral);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapNegative"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.FlapNegative) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.FlapNegative);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpGearExtended"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.GearExtended) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.GearExtended);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledge"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.Acknowledge) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.Acknowledge);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpRepeat"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.Repeat) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.Repeat);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedCommand"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.SpeedCommand) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.SpeedCommand);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchUp"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.UserSwitchUp) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.UserSwitchUp);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchMiddle"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.UserSwitchMiddle) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.UserSwitchMiddle);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchDown"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.UserSwitchDown) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.UserSwitchDown);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpVarioCircling"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	GPS_INFO.SwitchState.VarioCircling) {
      wp->GetDataField()->Set(GPS_INFO.SwitchState.VarioCircling);
      wp->RefreshDisplay();
    }
  }
}

static int OnTimerNotify(WindowControl * Sender) {
	(void)Sender;
  UpdateValues();
  return 0;
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};



void dlgSwitchesShowModal(void){

  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgSwitches.xml"));
  wf = dlgLoadFromXML(CallBackTable,

                      filename,
		      hWndMainWindow,
		      TEXT("IDR_XML_SWITCHES"));

  if (wf) {

    wf->SetTimerNotify(OnTimerNotify);

    UpdateValues();
    wf->ShowModal();

    delete wf;
  }
  wf = NULL;

}

