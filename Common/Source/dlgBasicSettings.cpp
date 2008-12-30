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

#include "Statistics.h"

#include "externs.h"
#include "Units.h"
#include "McReady.h"
#include "Atmosphere.h"
#include "dlgTools.h"
#include "device.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static bool BallastTimerActive = false;

static void OnCloseClicked(WindowControl * Sender){
(void)Sender;
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

      devPutQNH(devAll(), QNH);
      AirspaceQnhChangeNotify(QNH);

      // VarioWriteSettings();

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


static void SetBallast(void) {
  WndProperty* wp;

  GlidePolar::SetBallast();
  devPutBallast(devA(), BALLAST);
  devPutBallast(devB(), BALLAST);
  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastPercent"));
  if (wp) {
    wp->GetDataField()->Set(BALLAST*100);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
  if (wp) {
    wp->GetDataField()->
      SetAsFloat(GlidePolar::BallastLitres);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpWingLoading"));
  if (wp) {
    wp->GetDataField()->
      SetAsFloat(GlidePolar::WingLoading);
    wp->RefreshDisplay();
  }
}

int BallastSecsToEmpty = 120;

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  static double BallastTimeLast = -1;

  if (BallastTimerActive) {
    if (GPS_INFO.Time > BallastTimeLast) {
      double BALLAST_last = BALLAST;
      double dt = GPS_INFO.Time - BallastTimeLast;
      double percent_per_second = 1.0/max(10.0, BallastSecsToEmpty);
      BALLAST -= dt*percent_per_second;
      if (BALLAST<0) {
	BallastTimerActive = false;
	BALLAST = 0.0;
      }
      if (fabs(BALLAST-BALLAST_last)>0.001) {
	SetBallast();
      }      
    }
    BallastTimeLast = GPS_INFO.Time;
  } else {
    BallastTimeLast = GPS_INFO.Time;
  }

  static double altlast = GPS_INFO.BaroAltitude;
  if (fabs(GPS_INFO.BaroAltitude-altlast)>1) {
    WndProperty* wp;
    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->
	SetAsFloat(Units::ToUserAltitude(GPS_INFO.BaroAltitude));
      wp->RefreshDisplay();
    }
  }
  altlast = GPS_INFO.BaroAltitude;

  return 0;
}


static void OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
  case DataField::daSpecial:
    if (BALLAST>0.01) {
      BallastTimerActive = !BallastTimerActive;
    } else {
      BallastTimerActive = false;
    }
    break;
  case DataField::daGet:
    lastRead = BALLAST;
    Sender->Set(BALLAST*100);
    break;
  case DataField::daChange:
  case DataField::daPut:
    if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
      lastRead = BALLAST = Sender->GetAsFloat()/100.0;
      SetBallast();
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
        lastRead = BUGS = Sender->GetAsFloat()/100.0;
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
        lastRead = Sender->GetAsFloat();
        CuSonde::setForecastTemperature(Sender->GetAsFloat());
      }
    break;
  }
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnBugsData),
  DeclareCallBackEntry(OnTempData),
  DeclareCallBackEntry(OnBallastData),
  DeclareCallBackEntry(OnAltitudeData),
  DeclareCallBackEntry(OnQnhData), 
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


void dlgBasicSettingsShowModal(void){

  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgBasicSettings.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_BASICSETTINGS"));

  WndProperty* wp;

  BallastTimerActive = false;

  if (wf) {

    wf->SetTimerNotify(OnTimerNotify);

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
    wp = (WndProperty*)wf->FindByName(TEXT("prpWingLoading"));
    if (wp) {
      if (GlidePolar::WingLoading>0.1) {
	wp->GetDataField()->
	  SetAsFloat(GlidePolar::WingLoading);
      } else {
	wp->SetVisible(false);
      }
      wp->RefreshDisplay();
    }

    wf->ShowModal();
    delete wf;
  }
  wf = NULL;

}

