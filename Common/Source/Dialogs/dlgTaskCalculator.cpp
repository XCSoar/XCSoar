/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "XCSoar.h"
#include "Protection.hpp"
#include <math.h>
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "McReady.h"
#include "Units.hpp"
#include "Calculations.h" // TODO danger! RefreshTaskStatistics()
#include "Dialogs/dlgTools.h"
#include "GlideSolvers.hpp"
#include "Dialogs.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"

static WndForm *wf=NULL;

static double emc= 0.0;
static double cruise_efficiency= 1.0;


static void OnCancelClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancel);
}

static void OnOKClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static double Range = 0;


static void GetCruiseEfficiency(void) {
  if ((XCSoarInterface::Calculated().Flying) && (XCSoarInterface::Calculated().TaskStartTime>0) &&
      !(XCSoarInterface::Calculated().FinalGlide &&
	(XCSoarInterface::Calculated().TaskAltitudeDifference>0))) {

    cruise_efficiency = EffectiveCruiseEfficiency(&XCSoarInterface::Basic(), &XCSoarInterface::Calculated());
  } else {
    cruise_efficiency = GlidePolar::GetCruiseEfficiency();
  }
}

static void RefreshCalculator(void) {
  WndProperty* wp;

  task.RefreshTask(XCSoarInterface::SettingsComputer());
  RefreshTaskStatistics();

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    double dd = XCSoarInterface::Calculated().TaskTimeToGo;
    if ((XCSoarInterface::Calculated().TaskStartTime>0.0)&&(XCSoarInterface::Calculated().Flying)) {
      dd += XCSoarInterface::Basic().Time-XCSoarInterface::Calculated().TaskStartTime;
    }
    dd= min(24.0*60.0,dd/60.0);
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATTime"));
  if (wp) {
    if (!AATEnabled) {
      wp->SetVisible(false);
    } else {
      wp->GetDataField()->SetAsFloat(AATTaskLength);
    }
      wp->RefreshDisplay();
  }

  double d1 = (XCSoarInterface::Calculated().TaskDistanceToGo
	       +XCSoarInterface::Calculated().TaskDistanceCovered);
  if (AATEnabled && (d1==0.0)) {
    d1 = XCSoarInterface::Calculated().AATTargetDistance;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(d1*DISTANCEMODIFY);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEffectiveMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->GetDataField()->SetAsFloat(emc*LIFTMODIFY);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    if (!AATEnabled || !task.ValidTaskPoint(ActiveTaskPoint+1)) {
      wp->SetVisible(false);
    } else {
      wp->SetVisible(true);
    }
    wp->GetDataField()->SetAsFloat(Range*100.0);
    wp->RefreshDisplay();
  }

  double v1;
  if (XCSoarInterface::Calculated().TaskTimeToGo>0) {
    v1 = XCSoarInterface::Calculated().TaskDistanceToGo/
      XCSoarInterface::Calculated().TaskTimeToGo;
  } else {
    v1 = 0;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedRemaining"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(v1*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedAchieved"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(XCSoarInterface::Calculated().TaskSpeed*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpCruiseEfficiency"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(cruise_efficiency*100.0);
    wp->RefreshDisplay();
  }
}

static void DoOptimise(void) {
  double myrange= Range;
  double RangeLast= Range;
  double deltaTlast = 0;
  int steps = 0;
  if (!AATEnabled) return;

  // should do a GUI::ExchangeBlackboard() here and use local storage

  mutexTaskData.Lock();
  targetManipEvent.trigger();
  do {
    myrange = Range;
    task.AdjustAATTargets(Range);
    task.RefreshTask(XCSoarInterface::SettingsComputer());
    RefreshTaskStatistics();
    double deltaT = XCSoarInterface::Calculated().TaskTimeToGo;
    if ((XCSoarInterface::Calculated().TaskStartTime>0.0)&&(XCSoarInterface::Calculated().Flying)) {
      deltaT += XCSoarInterface::Basic().Time-XCSoarInterface::Calculated().TaskStartTime;
    }
    deltaT= min(24.0*60.0,deltaT/60.0)-AATTaskLength-5;

    double dRdT = 0.001;
    if (steps>0) {
      if (fabs(deltaT-deltaTlast)>0.01) {
        dRdT = min(0.5,(Range-RangeLast)/(deltaT-deltaTlast));
        if (dRdT<=0.0) {
          // error, time decreases with increasing range!
          // or, no effect on time possible
          break;
        }
      } else {
        // minimal change for whatever reason
        // or, no effect on time possible, e.g. targets locked
        break;
      }
    }
    RangeLast = Range;
    deltaTlast = deltaT;

    if (fabs(deltaT)>0.25) {
      // more than 15 seconds error
      Range -= dRdT*deltaT;
      Range = max(-1.0, min(Range,1.0));
    } else {
      break;
    }

  } while (steps++<25);

  Range = myrange;
  task.AdjustAATTargets(Range);
  RefreshCalculator();

  targetManipEvent.reset();

  mutexTaskData.Unlock();
}


static void OnTargetClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetVisible(false);
  dlgTarget();
  // find start value for range (it may have changed)
  Range = task.AdjustAATTargets(2.0);
  RefreshCalculator();
  wf->SetVisible(true);
}


static void OnMacCreadyData(DataField *Sender,
			    DataField::DataAccessKind_t Mode){
  double MACCREADY;
  switch(Mode){
  case DataField::daSpecial:
    if (XCSoarInterface::Calculated().timeCircling>0) {
      MACCREADY = XCSoarInterface::Calculated().TotalHeightClimb
	/XCSoarInterface::Calculated().timeCircling;
      Sender->Set(MACCREADY*LIFTMODIFY);
      GlidePolar::SetMacCready(MACCREADY);
      RefreshCalculator();
    }
    break;
  case DataField::daGet:
    Sender->Set(GlidePolar::GetMacCready()*LIFTMODIFY);
    break;
  case DataField::daPut:
  case DataField::daChange:
    MACCREADY = Sender->GetAsFloat()/LIFTMODIFY;
    GlidePolar::SetMacCready(MACCREADY);
    RefreshCalculator();
    break;
  }
}


static void OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  double rthis;
  switch(Mode){
  case DataField::daSpecial:
    DoOptimise();
    break;
  case DataField::daGet:
    //      Sender->Set(Range*100.0);
    break;
  case DataField::daPut:
  case DataField::daChange:
    rthis = Sender->GetAsFloat()/100.0;
    if (fabs(Range-rthis)>0.01) {
      Range = rthis;
      task.AdjustAATTargets(Range);
      RefreshCalculator();
    }
    break;
  }
}


static void OnCruiseEfficiencyData(DataField *Sender, DataField::DataAccessKind_t Mode) {
  double clast = GlidePolar::GetCruiseEfficiency();
  switch(Mode){
  case DataField::daGet:
    break;
  case DataField::daSpecial:
    GetCruiseEfficiency();
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast)>0.01) {
      RefreshCalculator();
    }
    break;
  case DataField::daPut:
  case DataField::daChange:
    cruise_efficiency = Sender->GetAsFloat()/100.0;
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast)>0.01) {
      RefreshCalculator();
    }
    break;
  }
}



static void OnOptimiseClicked(WindowControl * Sender){
  DoOptimise();
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnMacCreadyData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnOptimiseClicked),
  DeclareCallBackEntry(OnTargetClicked),
  DeclareCallBackEntry(OnCruiseEfficiencyData),
  DeclareCallBackEntry(NULL)
};

void dlgTaskCalculatorShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgTaskCalculator.xml"),
		      XCSoarInterface::main_window,
		      TEXT("IDR_XML_TASKCALCULATOR"));

  if (!wf) return;

  double MACCREADY_enter = GlidePolar::GetMacCready();
  double CRUISE_EFFICIENCY_enter = GlidePolar::GetCruiseEfficiency();

  emc = EffectiveMacCready(&XCSoarInterface::Basic(), 
			   &XCSoarInterface::Calculated());

  cruise_efficiency = CRUISE_EFFICIENCY_enter;

  // find start value for range
  Range = task.AdjustAATTargets(2.0);

  RefreshCalculator();

  if (!AATEnabled || !task.ValidTaskPoint(ActiveTaskPoint+1)) {
    ((WndButton *)wf->FindByName(TEXT("Optimise")))->SetVisible(false);
  }
  if (!task.ValidTaskPoint(ActiveTaskPoint)) {
    ((WndButton *)wf->FindByName(TEXT("Target")))->SetVisible(false);
  }

  if (wf->ShowModal() == mrCancel) {
    // todo: restore task settings.
    GlidePolar::SetMacCready(MACCREADY_enter);
    GlidePolar::SetCruiseEfficiency(CRUISE_EFFICIENCY_enter);
  }
  delete wf;
  wf = NULL;
}

