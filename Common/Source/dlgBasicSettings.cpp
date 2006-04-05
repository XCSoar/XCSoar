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

#include "statistics.h"

#include "externs.h"
#include "units.h"
#include "McReady.h"
#include "device.h"

#include "Atmosphere.h"

#include "WindowControls.h"
#include "dlgTools.h"
#include "Port.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static double INHg=0;

static void OnQnhData(DataField *Sender, DataField::DataAccessKind_t Mode){
  WndProperty* wp;

  switch(Mode){
    case DataField::daGet:
      Sender->Set(QNH);
    break;
    case DataField::daPut: 
    case DataField::daChange:
      QNH = Sender->GetAsFloat();
      INHg = (int)QNH;
      INHg = INHg*29.91/1013.2;

      VarioWriteSettings();
      wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
      if (wp) {
	wp->GetDataField()->
	  SetAsFloat(Units::ToUserAltitude(GPS_INFO.BaroAltitude));
	wp->RefreshDisplay();
      }
    break;
  }

}


// TODO: This isn't updating properly...
static void OnAltitudeData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      LockFlightData();
      Sender->Set(Units::ToUserAltitude(GPS_INFO.BaroAltitude));
      UnlockFlightData();
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
  }

}

static void OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode){
  WndProperty* wp;
  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = BALLAST;
      Sender->Set(BALLAST*100);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
        BALLAST = Sender->GetAsFloat()/100.0;
        GlidePolar::SetBallast();
        devPutBallast(devA(), BALLAST);
        devPutBallast(devB(), BALLAST);
	wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
	if (wp) {
	  wp->GetDataField()->
	    SetAsFloat(GlidePolar::BallastLitres);
	  wp->RefreshDisplay();
	}
      }
    break;
  }

}

static void OnBugsData(DataField *Sender, DataField::DataAccessKind_t Mode){

  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = BUGS;
      Sender->Set(BUGS*100);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
        BUGS = Sender->GetAsFloat()/100.0;
        GlidePolar::SetBallast();
        devPutBugs(devA(), BUGS);
        devPutBugs(devB(), BUGS);
      }
    break;
  }

}


static void OnTempData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = CuSonde::maxGroundTemperature;
      Sender->Set(CuSonde::maxGroundTemperature);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()) >= 1.0){
	CuSonde::setForecastTemperature(Sender->GetAsFloat());
      }
    break;
  }
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnBugsData),
  DeclearCallBackEntry(OnTempData),
  DeclearCallBackEntry(OnBallastData),
  DeclearCallBackEntry(OnAltitudeData),
  DeclearCallBackEntry(OnQnhData), 
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};


void dlgBasicSettingsShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, LocalPathS(TEXT("dlgBasicSettings.xml")), 
		      hWndMainWindow,
		      TEXT("IDR_XML_BASICSETTINGS"));

  WndProperty* wp;

  if (wf) {

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(
				     Units::ToUserAltitude(GPS_INFO.BaroAltitude));
      wp->GetDataField()->SetUnits(Units::GetAltitudeName());
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
    if (wp) {
      wp->GetDataField()->
	SetAsFloat(GlidePolar::BallastLitres);
      wp->RefreshDisplay();
    }

    wf->ShowModal();
    delete wf;
  }
  wf = NULL;

}


void WindowControlTest(void){

//  dlgBasicSettingsShowModal();

}

#endif
