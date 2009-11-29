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
#include "Blackboard.hpp"
#include "Units.hpp"
#include "InputEvents.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"

static WndForm *wf=NULL;


static void UpdateValues() {
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapLanding"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.FlapLanding) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.FlapLanding);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirbrakeExtended"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.AirbrakeLocked) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.AirbrakeLocked);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapPositive"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.FlapPositive) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.FlapPositive);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapNeutral"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.FlapNeutral) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.FlapNeutral);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpFlapNegative"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.FlapNegative) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.FlapNegative);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpGearExtended"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.GearExtended) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.GearExtended);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledge"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.Acknowledge) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.Acknowledge);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpRepeat"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.Repeat) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.Repeat);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedCommand"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.SpeedCommand) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.SpeedCommand);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchUp"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.UserSwitchUp) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.UserSwitchUp);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchMiddle"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.UserSwitchMiddle) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.UserSwitchMiddle);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUserSwitchDown"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.UserSwitchDown) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.UserSwitchDown);
      wp->RefreshDisplay();
    }
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpVarioCircling"));
  if (wp) {
    if (wp->GetDataField()->GetAsBoolean() !=
	XCSoarInterface::Basic().SwitchState.VarioCircling) {
      wp->GetDataField()->Set(XCSoarInterface::Basic().SwitchState.VarioCircling);
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
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};



void dlgSwitchesShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgSwitches.xml"),
		      XCSoarInterface::main_window,
		      TEXT("IDR_XML_SWITCHES"));
  if (wf == NULL)
    return;

  wf->SetTimerNotify(OnTimerNotify);

  UpdateValues();
  wf->ShowModal();

  delete wf;
}
