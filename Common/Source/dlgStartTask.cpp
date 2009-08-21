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
	Tobias Lohner <tobias@lohner-net.de>

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

//#include "Statistics.h"

#include "externs.h"
#include "Units.h"
#include "Utils.h"
#include "dlgTools.h"
#include "Process.h"

static WndForm *wf=NULL;

bool startIsValid = false;

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static void StartTaskAnyway(bool valid) {
  startIsValid = valid;
}


static void OnStartTaskAnywayClicked(WindowControl * Sender){
	(void)Sender;
        StartTaskAnyway(true);
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnStartTaskAnywayClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void dlgStartTaskShowModal(bool *validStart, double Time, double Speed, double Altitude){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgStartTask.xml"),
		      hWndMainWindow,
		      TEXT("IDR_XML_STARTTASK"));

  if (wf) {
    WndProperty* wp;

    TCHAR Temp[80];

    wp = (WndProperty*)wf->FindByName(TEXT("prpTime"));
    if (wp) {
      Units::TimeToText(Temp, (int)TimeLocal((int)Time));
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpSpeed"));
    if (wp) {
      _stprintf(Temp, TEXT("%.0f %s"),
                (double) TASKSPEEDMODIFY * Speed, Units::GetTaskSpeedName());
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      _stprintf(Temp, TEXT("%.0f %s"),
                (double) Altitude*ALTITUDEMODIFY, Units::GetAltitudeName());
      wp->SetText(Temp);
    }

    wf->ShowModal();

    delete wf;
  }
  wf = NULL;

  *validStart = startIsValid;
}

