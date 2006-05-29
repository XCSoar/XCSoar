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

#include "externs.h"
#include "units.h"
#include "externs.h"
#include "Waypointparser.h"
#include "Logger.h"

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


static bool first = true;

static void UpdateValues() {
  static int extGPSCONNECT_last = extGPSCONNECT;
  static int NAVWarning_last = GPS_INFO.NAVWarning;
  static int SatellitesUsed_last = GPS_INFO.SatellitesUsed;
  static int VarioAvailable_last = GPS_INFO.VarioAvailable;
  static int FLARM_Available_last = GPS_INFO.FLARM_Available;
  static bool LoggerActive_last = LoggerActive;
  static bool DeclaredToDevice_last = DeclaredToDevice;
  static double SupplyBatteryVoltage_last = GPS_INFO.SupplyBatteryVoltage;

  if (first ||
      (extGPSCONNECT_last != extGPSCONNECT) ||
      (NAVWarning_last != GPS_INFO.NAVWarning) ||
      (SatellitesUsed_last != GPS_INFO.SatellitesUsed) ||
      (VarioAvailable_last != GPS_INFO.VarioAvailable) ||
      (FLARM_Available_last != GPS_INFO.FLARM_Available) ||
      (LoggerActive_last != LoggerActive) ||
      (DeclaredToDevice_last != DeclaredToDevice) ||
      (SupplyBatteryVoltage_last != GPS_INFO.SupplyBatteryVoltage)) {
    first = false;

    extGPSCONNECT_last = extGPSCONNECT;
    NAVWarning_last = GPS_INFO.NAVWarning;
    SatellitesUsed_last = GPS_INFO.SatellitesUsed;
    VarioAvailable_last = GPS_INFO.VarioAvailable;
    FLARM_Available_last = GPS_INFO.FLARM_Available;
    LoggerActive_last = LoggerActive;
    DeclaredToDevice_last = DeclaredToDevice;
    SupplyBatteryVoltage_last = GPS_INFO.SupplyBatteryVoltage;

  } else {
    return;
  }

  TCHAR Temp[80];

  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpGPS"));
  if (wp) {
    if (extGPSCONNECT) {
      if (GPS_INFO.NAVWarning) {
	wp->SetText(gettext(TEXT("Fix invalid")));
      } else {
	if (GPS_INFO.SatellitesUsed==0) {
	  wp->SetText(gettext(TEXT("No fix")));

	} else {
	  wp->SetText(gettext(TEXT("3D fix")));
	}
      }
      wp->RefreshDisplay();

      wp = (WndProperty*)wf->FindByName(TEXT("prpNumSat"));
      if (wp) {
	_stprintf(Temp,TEXT("%d"),GPS_INFO.SatellitesUsed);
	wp->SetText(Temp);
	wp->RefreshDisplay();
      }
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
      wp->RefreshDisplay();
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpVario"));
  if (wp) {
    if (GPS_INFO.VarioAvailable) {
      wp->SetText(gettext(TEXT("Connected")));
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  if (wp) {
    wp = (WndProperty*)wf->FindByName(TEXT("prpFLARM"));
    if (GPS_INFO.FLARM_Available) {
      wp->SetText(gettext(TEXT("Connected")));
    } else {
      wp->SetText(gettext(TEXT("Disconnected")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLogger"));
  if (wp) {
    if (LoggerActive) {
      wp->SetText(gettext(TEXT("ON")));
    } else {
      wp->SetText(gettext(TEXT("OFF")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDeclared"));
  if (wp) {
    if (DeclaredToDevice) {
      wp->SetText(gettext(TEXT("YES")));
    } else {
      wp->SetText(gettext(TEXT("NO")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBattery"));
  if (wp) {
    _stprintf(Temp,TEXT("%.1f V"),GPS_INFO.SupplyBatteryVoltage);
    wp->SetText(Temp);
    wp->RefreshDisplay();
  }
}


static int OnTimerNotify(WindowControl * Sender) {
  UpdateValues();
  return 0;
}


void dlgStatusSystemShowModal(void){

  first = true;

  wf = dlgLoadFromXML(NULL,
		      LocalPathS(TEXT("dlgStatusSystem.xml")),
		      hWndMainWindow,
		      TEXT("IDR_XML_STATUSSYSTEM"));
  if (!wf) return;

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wf->SetTimerNotify(OnTimerNotify);

  wf->SetLButtonUpNotify(OnFormLButtonUp);

  UpdateValues();

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

#endif
