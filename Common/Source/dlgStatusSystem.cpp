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

#include "stdafx.h"

#include "externs.h"
#include "units.h"
#include "externs.h"
#include "Waypointparser.h"

#include "WindowControls.h"
#include "dlgTools.h"

extern HWND   hWndMainWindow;
extern BOOL extGPSCONNECT;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int OnFormLButtonUp(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  static int nignore = 0;
#ifndef GNAV
  if (nignore) {
    nignore = 0;
    wf->SetModalResult(mrOK);
    return(0);
  }
  nignore++;
#else
  wf->SetModalResult(mrOK);
#endif
  return(0);
}


void dlgStatusSystemShowModal(void){

  WndProperty *wp;

  TCHAR Temp[1000];

  wf = dlgLoadFromXML(NULL, "\\NOR Flash\\dlgStatusSystem.xml", hWndMainWindow);
  if (!wf) return;

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wf->SetLButtonUpNotify(OnFormLButtonUp);

  wp = (WndProperty*)wf->FindByName(TEXT("prpGPS"));
  if (extGPSCONNECT) {
    if (GPS_INFO.NAVWarning) {
      wp->SetText(gettext(TEXT("Fix invalid")));
    } else {
      wp->SetText(gettext(TEXT("3D fix")));
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpNumSat"));
    _stprintf(Temp,TEXT("%d"),GPS_INFO.SatellitesUsed);
    wp->SetText(Temp);

  } else {
    wp->SetText(gettext(TEXT("Disconnected")));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVario"));
  if (GPS_INFO.VarioAvailable) {
    wp->SetText(gettext(TEXT("Connected")));
  } else {
    wp->SetText(gettext(TEXT("Disconnected")));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFLARM"));
  if (GPS_INFO.FLARM_Available) {
    wp->SetText(gettext(TEXT("Connected")));
  } else {
    wp->SetText(gettext(TEXT("Disconnected")));
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLogger"));
  if (LoggerActive) {
    wp->SetText(gettext(TEXT("ON")));
  } else {
    wp->SetText(gettext(TEXT("OFF")));
  }

  // JMW TODO: current time and battery

  wp = (WndProperty*)wf->FindByName(TEXT("prpBattery"));
  _stprintf(Temp,TEXT("%0.f%%"),GPS_INFO.SupplyBatteryVoltage); // TODO
  wp->SetText(Temp);

  wf->ShowModal();

  delete wf;

  wf = NULL;

}



