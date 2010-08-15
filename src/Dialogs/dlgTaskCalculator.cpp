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
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "Units.hpp"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"

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

  wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Units::ToUserDistance(
        XCSoarInterface::Calculated().task_stats.total.solution_planned.Vector.Distance));
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEffectiveMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->GetDataField()->SetAsFloat(Units::ToUserVSpeed(emc));
    wp->RefreshDisplay();
  }

  /*
  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    wp->set_visible(task.getSettings().AATEnabled &&
                    task.ValidTaskPoint(task.getActiveIndex() + 1));
    wp->GetDataField()->SetAsFloat(Range*100.0);
    wp->RefreshDisplay();
  }

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
    wp->GetDataField()->SetAsFloat(cruise_efficiency * 100);
    wp->RefreshDisplay();
  }
}

static void
DoOptimise(void)
{
  // should do a GUI::ExchangeBlackboard() here and use local storage

  targetManipEvent.trigger();
  // ... OLD_TASK \todo
  targetManipEvent.reset();
}

static void
OnTargetClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->hide();
#ifdef OLD_TASK
  dlgTarget();
  // find start value for range (it may have changed)
  Range = task.AdjustAATTargets(2.0);
  RefreshCalculator();
#endif
  wf->show();
}


static void
OnMacCreadyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  fixed MACCREADY;
  switch (Mode) {
  case DataField::daSpecial:
    if (positive(XCSoarInterface::Calculated().timeCircling)) {
      MACCREADY = XCSoarInterface::Calculated().TotalHeightClimb /
                  XCSoarInterface::Calculated().timeCircling;
      Sender->Set(Units::ToUserVSpeed(MACCREADY));
#ifdef OLD_TASK
      GlidePolar::SetMacCready(MACCREADY);
#endif
      RefreshCalculator();
    }
    break;
  case DataField::daGet:
    Sender->Set(Units::ToUserVSpeed(protected_task_manager.get_glide_polar().get_mc()));
    break;
  case DataField::daPut:
  case DataField::daChange:
    MACCREADY = Units::ToSysVSpeed(Sender->GetAsFixed());
#ifdef OLD_TASK
    GlidePolar::SetMacCready(MACCREADY);
#endif
    RefreshCalculator();
    break;
  }
}

static void
OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  fixed rthis;
  switch (Mode) {
  case DataField::daSpecial:
    DoOptimise();
    break;
  case DataField::daGet:
    //      Sender->Set(Range*100.0);
    break;
  case DataField::daPut:
  case DataField::daChange:
    rthis = Sender->GetAsFixed() / 100;
#ifdef OLD_TASK
    if (fabs(Range - rthis) > fixed_one / 100) {
      Range = rthis;
      task.AdjustAATTargets(Range);
      RefreshCalculator();
    }
#endif
    break;
  }
}

static void
OnCruiseEfficiencyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  fixed clast = protected_task_manager.get_glide_polar().get_cruise_efficiency();
  (void)clast; // unused for now

  switch (Mode) {
  case DataField::daGet:
    break;
  case DataField::daSpecial:
    GetCruiseEfficiency();
#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast) > fixed_one / 100) {
      RefreshCalculator();
    }
#endif
    break;
  case DataField::daPut:
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

static void
OnOptimiseClicked(WindowControl * Sender)
{
  DoOptimise();
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnMacCreadyData),
  DeclareCallBackEntry(OnRangeData),
  DeclareCallBackEntry(OnOKClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnOptimiseClicked),
  DeclareCallBackEntry(OnTargetClicked),
  DeclareCallBackEntry(OnCruiseEfficiencyData),
  DeclareCallBackEntry(NULL)
};

void
dlgTaskCalculatorShowModal(SingleWindow &parent)
{
  wf = dlgLoadFromXML(CallBackTable,
		                  XCSoarInterface::main_window, _T("IDR_XML_TASKCALCULATOR"));

  if (!wf)
    return;

  GlidePolar polar = protected_task_manager.get_glide_polar();

  fixed CRUISE_EFFICIENCY_enter = polar.get_cruise_efficiency();

  emc = XCSoarInterface::Calculated().task_stats.effective_mc;

  cruise_efficiency = CRUISE_EFFICIENCY_enter;

  // find start value for range
#ifdef OLD_TASK
  Range = task.AdjustAATTargets(2.0);
#endif

  RefreshCalculator();

#ifdef OLD_TASK
  if (!task.getSettings().AATEnabled ||
      !task.ValidTaskPoint(task.getActiveIndex() + 1))
    ((WndButton *)wf->FindByName(_T("Optimise")))->hide();

  if (!task.ValidTaskPoint(task.getActiveIndex()))
    ((WndButton *)wf->FindByName(_T("Target")))->hide();
#endif

  if (wf->ShowModal() == mrCancel) {
    // todo: restore task settings.
#ifdef OLD_TASK
    GlidePolar::SetMacCready(MACCREADY_enter);
    GlidePolar::SetCruiseEfficiency(CRUISE_EFFICIENCY_enter);
#endif
  }

  delete wf;
  wf = NULL;
}

