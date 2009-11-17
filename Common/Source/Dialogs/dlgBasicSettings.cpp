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
#include "Protection.hpp"
#include "Math/Pressure.h"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "McReady.h"
#include "Atmosphere.h"
#include "Device/device.h"
#include "DataField/Base.hpp"
#include "Airspace.h"
#include "MainWindow.hpp"

#include <math.h>

static WndForm *wf=NULL;

// static bool BallastTimerActive = false;

static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void OnBallastDump(WindowControl *Sender){
(void)Sender;
 XCSoarInterface::SetSettingsComputer().BallastTimerActive=
   !XCSoarInterface::SettingsComputer().BallastTimerActive;
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
	  SetAsFloat(Units::ToUserAltitude(XCSoarInterface::Basic().BaroAltitude));
	wp->RefreshDisplay();
      }
    break;
  }

}


// TODO bug: Check, this isn't updating properly?
static void OnAltitudeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      Sender->Set(Units::ToUserAltitude(XCSoarInterface::Basic().BaroAltitude));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
  }
}


static void SetBallast(void) {
  WndProperty* wp;

  GlidePolar::UpdatePolar(true, XCSoarInterface::SettingsComputer());

  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastPercent"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::GetBallast()*100);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
  if (wp) {
    wp->GetDataField()->
      SetAsFloat(GlidePolar::GetBallastLitres());
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpWingLoading"));
  if (wp) {
    wp->GetDataField()->
      SetAsFloat(GlidePolar::WingLoading);
    wp->RefreshDisplay();
  }
}

//int BallastSecsToEmpty = 120;

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
/*
  static double BallastTimeLast = -1;

  if (BallastTimerActive) {
    if (XCSoarInterface::Basic().Time > BallastTimeLast) {
      double BALLAST_last = BALLAST;
      double dt = XCSoarInterface::Basic().Time - BallastTimeLast;
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
    BallastTimeLast = XCSoarInterface::Basic().Time;
  } else {
    BallastTimeLast = XCSoarInterface::Basic().Time;
  }
*/

  SetBallast();

  static double altlast = XCSoarInterface::Basic().BaroAltitude;
  if (fabs(XCSoarInterface::Basic().BaroAltitude-altlast)>1) {
    WndProperty* wp;
    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->
	SetAsFloat(Units::ToUserAltitude(XCSoarInterface::Basic().BaroAltitude));
      wp->RefreshDisplay();
    }
  }
  altlast = XCSoarInterface::Basic().BaroAltitude;

  return 0;
}


static void OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;
  double BALLAST = GlidePolar::GetBallast();

  switch(Mode){
  case DataField::daSpecial:
    if (BALLAST>0.01) {
      XCSoarInterface::SetSettingsComputer().BallastTimerActive =
	!XCSoarInterface::SettingsComputer().BallastTimerActive;
    } else {
      XCSoarInterface::SetSettingsComputer().BallastTimerActive = false;
    }
    ((WndButton *)wf->FindByName(TEXT("buttonDumpBallast")))->SetVisible(!XCSoarInterface::SettingsComputer().BallastTimerActive);
    ((WndButton *)wf->FindByName(TEXT("buttonStopDump")))->SetVisible(XCSoarInterface::SettingsComputer().BallastTimerActive);
    break;
  case DataField::daGet:
    lastRead = BALLAST;
    Sender->Set(BALLAST*100);
    break;
  case DataField::daChange:
  case DataField::daPut:
    if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
      lastRead = BALLAST = Sender->GetAsFloat()/100.0;
      GlidePolar::SetBallast(BALLAST);
      SetBallast();
    }
    break;
  }
}

static void OnBugsData(DataField *Sender, DataField::DataAccessKind_t Mode){
  double BUGS = GlidePolar::GetBugs();
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
	GlidePolar::SetBugs(BUGS);
        GlidePolar::UpdatePolar(true, XCSoarInterface::SettingsComputer());
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
  DeclareCallBackEntry(OnBallastDump),
  DeclareCallBackEntry(NULL)
};


void dlgBasicSettingsShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgBasicSettings.xml"),
		      XCSoarInterface::main_window,
		      TEXT("IDR_XML_BASICSETTINGS"));

  WndProperty* wp;

//  BallastTimerActive = false;

  if (wf) {

    wf->SetTimerNotify(OnTimerNotify);

    ((WndButton *)wf->FindByName(TEXT("buttonDumpBallast")))->SetVisible(!XCSoarInterface::SettingsComputer().BallastTimerActive);
    ((WndButton *)wf->FindByName(TEXT("buttonStopDump")))->SetVisible(XCSoarInterface::SettingsComputer().BallastTimerActive);

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(
	       Units::ToUserAltitude(XCSoarInterface::Basic().BaroAltitude));
      wp->GetDataField()->SetUnits(Units::GetAltitudeName());
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
    if (wp) {
      wp->GetDataField()->
	SetAsFloat(GlidePolar::GetBallastLitres());
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

