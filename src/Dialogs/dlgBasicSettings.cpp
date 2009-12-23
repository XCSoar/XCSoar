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

#include "Task/TaskManager.hpp"

#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "MacCready.h"
#include "Atmosphere.h"
#include "Device/device.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"

#include <math.h>

static WndForm *wf=NULL;

static bool changed = false;

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


static void OnQnhData(DataField *Sender, DataField::DataAccessKind_t Mode){
  WndProperty* wp;

  switch(Mode){
    case DataField::daGet:
      Sender->Set(FIXED_DOUBLE(XCSoarInterface::Basic().pressure.get_QNH()));
    break;
    case DataField::daPut:
    case DataField::daChange:
      fixed QNH = Sender->GetAsFloat();

#ifdef OLD_TASK
      XCSoarInterface::Basic().pressure.set_QNH(QNH);
      AllDevicesPutQNH(XCSoarInterface::Basic().pressure);
      airspace_database.set_flight_levels(XCSoarInterface::Basic().pressure);
#endif
      wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
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

GlidePolar* glide_polar = NULL;


static void SetAltitude()
{
  static double altlast = -2;
  if (fabs(XCSoarInterface::Basic().BaroAltitude-altlast)>1) {
    WndProperty* wp;
    wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
    if (wp) {
      if (!XCSoarInterface::Basic().BaroAltitudeAvailable) {
        wp->hide();
      } else {
        wp->GetDataField()->
          SetAsFloat(Units::ToUserAltitude(XCSoarInterface::Basic().BaroAltitude));
        wp->RefreshDisplay();
      }
    }
  }
  altlast = XCSoarInterface::Basic().BaroAltitude;
}


static void SetBallast(void) 
{
  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpBallastPercent"));
  if (wp) {
    if (glide_polar->is_ballastable()) {
      wp->GetDataField()->SetAsFloat(glide_polar->get_ballast()*100);
    } else {
      wp->hide();
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpBallastLitres"));
  if (wp) {
    if (glide_polar->is_ballastable()) {
      wp->GetDataField()->SetAsFloat(glide_polar->get_ballast_litres());
    } else {
      wp->hide();
    }
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpWingLoading"));
  if (wp) {
    if (fixed wl = glide_polar->get_wing_loading() > fixed_zero) {
      wp->GetDataField()->SetAsFloat(wl);
    } else {
      wp->hide();
    }
    wp->RefreshDisplay();
  }
}

static int OnTimerNotify(WindowControl * Sender) 
{
  (void)Sender;
  SetBallast();
  SetAltitude();
  return 0;
}

static void SetButtons()
{
  WndButton* wb;

  if ((wb = (WndButton *)wf->FindByName(_T("buttonDumpBallast"))) != NULL) {
    wb->set_visible(!XCSoarInterface::SettingsComputer().BallastTimerActive && 
                    glide_polar->is_ballastable());
  }
  if ((wb = (WndButton *)wf->FindByName(_T("buttonStopDump"))) != NULL) {
    wb->set_visible(XCSoarInterface::SettingsComputer().BallastTimerActive &&
                    glide_polar->is_ballastable());
  }
}

static void OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
  case DataField::daSpecial:
    if (glide_polar->get_ballast()>0.01) {
      XCSoarInterface::SetSettingsComputer().BallastTimerActive =
	!XCSoarInterface::SettingsComputer().BallastTimerActive;
    } else {
      XCSoarInterface::SetSettingsComputer().BallastTimerActive = false;
    }
    SetButtons();
    break;
  case DataField::daGet:
    lastRead = glide_polar->get_ballast();
    Sender->SetAsFloat(glide_polar->get_ballast()*100);
    break;
  case DataField::daChange:
  case DataField::daPut:
    if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
      lastRead = Sender->GetAsFloat()/100.0;
      glide_polar->set_ballast(lastRead);
      changed = true;
      SetBallast();
    }
    break;
  }
}

static void OnBugsData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = glide_polar->get_bugs();
      Sender->Set(lastRead*100);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
        lastRead = Sender->GetAsFloat()/100.0;
        glide_polar->set_bugs(lastRead);
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


void dlgBasicSettingsShowModal() 
{
  GlidePolar gp_copy = task_manager.get_glide_polar();
  glide_polar = &gp_copy;

  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgBasicSettings.xml"),
		      XCSoarInterface::main_window,
		      _T("IDR_XML_BASICSETTINGS"));
  if (wf == NULL)
    return;

  changed = false;

  wf->SetTimerNotify(OnTimerNotify);

  OnTimerNotify(NULL);

  if ((wf->ShowModal() == mrOK) && changed) {
    task_manager.set_glide_polar(gp_copy);
  }

  delete wf;
}

