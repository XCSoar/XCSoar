/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Units.hpp"
#include "LocalTime.hpp"
#include "MainWindow.hpp"

static WndForm *wf=NULL;

bool startIsValid = false;

static void OnCloseClicked(WndButton &Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static void StartTaskAnyway(bool valid) {
  startIsValid = valid;
}


static void OnStartTaskAnywayClicked(WndButton &Sender){
	(void)Sender;
        StartTaskAnyway(true);
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnStartTaskAnywayClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void dlgStartTaskShowModal(bool *validStart, double Time, double Speed, double Altitude){
  wf = LoadDialog(CallBackTable,
		      XCSoarInterface::main_window,
		      _T("IDR_XML_STARTTASK"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  TCHAR Temp[80];

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  if (wp) {
    Units::TimeToTextHHMMSigned(Temp, (int)TimeLocal((int)Time));
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSpeed"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              Units::ToUserUnit(Speed, Units::TaskSpeedUnit),
              Units::GetTaskSpeedName());
    wp->SetText(Temp);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  if (wp) {
    _stprintf(Temp, _T("%.0f %s"),
              Units::ToUserUnit(Altitude, Units::AltitudeUnit),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  }

  wf->ShowModal();

  delete wf;

  *validStart = startIsValid;
}
