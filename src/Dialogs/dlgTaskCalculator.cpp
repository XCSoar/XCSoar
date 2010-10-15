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
#include "Units.hpp"
#include "DataField/Float.hpp"
#include "Screen/SingleWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "Screen/Layout.hpp"

#include <math.h>
#include <algorithm>

using std::min;
using std::max;

static WndForm *wf = NULL;

static fixed emc;
static fixed cruise_efficiency;

static void
OnCancelClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrCancel);
}

static void
OnOKClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
GetCruiseEfficiency(void)
{
  cruise_efficiency = XCSoarInterface::Calculated().task_stats.cruise_efficiency;
}

static void
SetMC(fixed mc) {
  GlidePolar polar = protected_task_manager.get_glide_polar();
  polar.set_mc(mc);
  protected_task_manager.set_glide_polar(polar);
  device_blackboard.SetMC(mc);
}

static void
RefreshCalculator(void)
{
  WndProperty* wp;

  // update outputs
  wp = (WndProperty*)wf->FindByName(_T("prpAATEst"));
  if (wp) {
    wp->GetDataField()->SetAsFloat((
        XCSoarInterface::Calculated().common_stats.task_time_remaining +
        XCSoarInterface::Calculated().common_stats.task_time_elapsed) / 60);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(_T("prpAATTime"));
  if (wp) {
    if (XCSoarInterface::Calculated().task_stats.has_targets) {
      wp->GetDataField()->SetAsFloat(
          protected_task_manager.get_ordered_task_behaviour().aat_min_time / 60);
      wp->RefreshDisplay();
    } else {
      wp->hide();
    }
  }

  fixed rPlanned = XCSoarInterface::Calculated().task_stats.total.solution_planned.Vector.Distance;

  wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserDistance(rPlanned));
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetVerticalSpeedName());
    df.Set(Units::ToUserVSpeed(protected_task_manager.get_glide_polar().get_mc()));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEffectiveMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->GetDataField()->SetAsFloat(Units::ToUserVSpeed(emc));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    fixed rMax = XCSoarInterface::Calculated().task_stats.distance_max;
    fixed rMin = XCSoarInterface::Calculated().task_stats.distance_min;
    fixed range = (fixed_two * (rPlanned - rMin) / (rMax - rMin)) - fixed_one;
    wp->GetDataField()->SetAsFloat(range * fixed(100));
    wp->RefreshDisplay();
  }
/*
  fixed v1;
  if (XCSoarInterface::Calculated().TaskTimeToGo>0) {
    v1 = XCSoarInterface::Calculated().TaskDistanceToGo/
      XCSoarInterface::Calculated().TaskTimeToGo;
  } else {
    v1 = 0;
  }
  */

  wp = (WndProperty*)wf->FindByName(_T("prpSpeedRemaining"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserTaskSpeed(
        XCSoarInterface::Calculated().task_stats.total.remaining_effective.get_speed()));
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpSpeedAchieved"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserTaskSpeed(
        XCSoarInterface::Calculated().task_stats.total.travelled.get_speed()));
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpCruiseEfficiency"));
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(XCSoarInterface::Calculated().task_stats.cruise_efficiency * fixed(100));
    wp->RefreshDisplay();
  }
}

static void
OnTargetClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->hide();
  dlgTargetShowModal();
  wf->show();
}

static void OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  RefreshCalculator();
}

static void
OnMacCreadyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat *df = (DataFieldFloat *)Sender;

  fixed MACCREADY;
  switch (Mode) {
  case DataField::daSpecial:
    if (positive(XCSoarInterface::Calculated().timeCircling)) {
      MACCREADY = XCSoarInterface::Calculated().TotalHeightClimb /
                  XCSoarInterface::Calculated().timeCircling;
      df->Set(Units::ToUserVSpeed(MACCREADY));
      SetMC(MACCREADY);
      RefreshCalculator();
    }
    break;
  case DataField::daChange:
    MACCREADY = Units::ToSysVSpeed(Sender->GetAsFixed());
    SetMC(MACCREADY);
    RefreshCalculator();
    break;
  }
}

static void
OnCruiseEfficiencyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  fixed clast = protected_task_manager.get_glide_polar().get_cruise_efficiency();
  (void)clast; // unused for now

  switch (Mode) {
  case DataField::daSpecial:
    GetCruiseEfficiency();
#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast) > fixed_one / 100) {
      RefreshCalculator();
    }
#endif
    break;
  case DataField::daChange:
    cruise_efficiency = Sender->GetAsFixed() / 100;
#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast) > fixed_one / 100) {
      RefreshCalculator();
    }
#endif
    break;
  }
}


static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnMacCreadyData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnTargetClicked),
  DeclareCallBackEntry(OnCruiseEfficiencyData),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskCalculatorShowModal(SingleWindow &parent)
{
  if (!Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKCALCULATOR"));
  } else {
    wf = LoadDialog(CallBackTable,
                        parent,
                        _T("IDR_XML_TASKCALCULATOR_L"));
  }
  if (!wf)
    return;

  GlidePolar polar = protected_task_manager.get_glide_polar();

  fixed CRUISE_EFFICIENCY_enter = polar.get_cruise_efficiency();
  fixed MACCREADY_enter = protected_task_manager.get_glide_polar().get_mc();

  emc = XCSoarInterface::Calculated().task_stats.effective_mc;

  cruise_efficiency = CRUISE_EFFICIENCY_enter;

  RefreshCalculator();

  if (!XCSoarInterface::Calculated().common_stats.ordered_has_targets) {
    ((WndButton *)wf->FindByName(_T("prpRange")))->hide();
  }
  wf->SetTimerNotify(OnTimerNotify);

  if (wf->ShowModal() == mrCancel) {
    // todo: restore task settings.
    SetMC(MACCREADY_enter);
#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(CRUISE_EFFICIENCY_enter);
#endif
  }

  delete wf;
  wf = NULL;
}

