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

#include "Dialogs/Task.hpp"
#include "Dialogs/dlgTaskManager.hpp"
#include "Dialogs/Internal.hpp"
#include "Protection.hpp"
#include "Units/Units.hpp"
#include "DataField/Float.hpp"
#include "Screen/SingleWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "Screen/Layout.hpp"
#include "Form/TabBar.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Icon.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Look.hpp"
#include "MainWindow.hpp"

#include <math.h>
#include <algorithm>

using std::min;
using std::max;

static WndForm *wf = NULL;
static TabBarControl* wTabBar = NULL;
static fixed emc;
static fixed cruise_efficiency;
static bool lazy_loaded = false;
static bool* task_modified = NULL;

static void
GetCruiseEfficiency(void)
{
  cruise_efficiency = XCSoarInterface::Calculated().task_stats.cruise_efficiency;
}

static void
RefreshCalculator(void)
{
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  WndProperty* wp;

  // update outputs
  wp = (WndProperty*)wf->FindByName(_T("prpAATEst"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat((
        common_stats.task_time_remaining +
        common_stats.task_time_elapsed) / 60);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(_T("prpAATTime"));
  if (wp) {
    if (task_stats.has_targets) {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      df.SetAsFloat(
          protected_task_manager->get_ordered_task_behaviour().aat_min_time / 60);
      wp->RefreshDisplay();
    } else {
      wp->hide();
    }
  }

  fixed rPlanned = task_stats.total.solution_planned.IsDefined()
    ? task_stats.total.solution_planned.vector.Distance
    : fixed_zero;

  wp = (WndProperty*)wf->FindByName(_T("prpDistance"));
  if (wp != NULL && positive(rPlanned)) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserDistance(rPlanned));
    df.SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetVerticalSpeedName());
    df.Set(Units::ToUserVSpeed(CommonInterface::SettingsComputer().glide_polar_task.GetMC()));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpEffectiveMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetVerticalSpeedName());
    df.SetAsFloat(Units::ToUserVSpeed(emc));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpRange"));
  if (wp != NULL && positive(rPlanned)) {
    wp->RefreshDisplay();
    fixed rMax = task_stats.distance_max;
    fixed rMin = task_stats.distance_min;
    fixed range = (fixed_two * (rPlanned - rMin) / (rMax - rMin)) - fixed_one;
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(range * fixed(100));
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

  if (task_stats.total.remaining_effective.IsDefined()) {
    wp = (WndProperty*)wf->FindByName(_T("prpSpeedRemaining"));
    assert(wp != NULL);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserTaskSpeed(
        task_stats.total.remaining_effective.get_speed()));
    df.SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  if (task_stats.total.travelled.IsDefined()) {
    wp = (WndProperty*)wf->FindByName(_T("prpSpeedAchieved"));
    assert(wp != NULL);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserTaskSpeed(
        task_stats.total.travelled.get_speed()));
    df.SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpCruiseEfficiency"));
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(task_stats.cruise_efficiency * fixed(100));
    wp->RefreshDisplay();
  }
}

void
pnlTaskCalculator::OnTargetClicked(gcc_unused WndButton &Sender)
{
  wf->hide();
  dlgTargetShowModal();
  wf->show();
}

void
pnlTaskCalculator::OnTimerNotify(gcc_unused WndForm &Sender)
{
  RefreshCalculator();
}

void
pnlTaskCalculator::OnMacCreadyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat *df = (DataFieldFloat *)Sender;

  fixed MACCREADY;
  switch (Mode) {
  case DataField::daSpecial:
    if (positive(XCSoarInterface::Calculated().time_climb)) {
      MACCREADY = XCSoarInterface::Calculated().total_height_gain /
                  XCSoarInterface::Calculated().time_climb;
      df->Set(Units::ToUserVSpeed(MACCREADY));
      ActionInterface::SetMacCready(MACCREADY);
      RefreshCalculator();
    }
    break;
  case DataField::daChange:
    MACCREADY = Units::ToSysVSpeed(df->GetAsFixed());
    ActionInterface::SetMacCready(MACCREADY);
    RefreshCalculator();
    break;
  }
}

void
pnlTaskCalculator::OnCruiseEfficiencyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;
  fixed clast = CommonInterface::SettingsComputer().glide_polar_task.GetCruiseEfficiency();
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
    cruise_efficiency = df.GetAsFixed() / 100;
#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(cruise_efficiency);
    if (fabs(cruise_efficiency-clast) > fixed_one / 100) {
      RefreshCalculator();
    }
#endif
    break;
  }
}

bool
pnlTaskCalculator::OnTabPreShow(gcc_unused TabBarControl::EventType EventType)
{
  if (!lazy_loaded) {
    lazy_loaded = true;
    const GlidePolar& polar = CommonInterface::SettingsComputer().glide_polar_task;

    fixed CRUISE_EFFICIENCY_enter = polar.GetCruiseEfficiency();
//    fixed MACCREADY_enter = protected_task_manager->get_glide_polar().GetMC();

    emc = XCSoarInterface::Calculated().task_stats.effective_mc;

    cruise_efficiency = CRUISE_EFFICIENCY_enter;

    if (!XCSoarInterface::Calculated().common_stats.ordered_has_targets) {
      ((WndButton *)wf->FindByName(_T("prpRange")))->hide();
    }
    wf->SetTimerNotify(pnlTaskCalculator::OnTimerNotify);
  }

  RefreshCalculator();

  return true;
}

void
pnlTaskCalculator::OnWarningPaint(gcc_unused WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  if (*task_modified) {
    const TCHAR* message = _("Calculator excludes unsaved task changes!");
    canvas.select(Fonts::Title);
    const int textheight = canvas.text_height(message);

    const AirspaceLook &look = CommonInterface::main_window.look->airspace;
    const MaskedIcon *bmp = &look.intercept_icon;
    const int offsetx = bmp->get_size().cx;
    const int offsety = canvas.get_height() - bmp->get_size().cy;
    canvas.clear(COLOR_YELLOW);
    bmp->draw(canvas, offsetx, offsety);
    canvas.set_background_color(COLOR_YELLOW);

    canvas.text(offsetx * 2 + Layout::Scale(2),
                (int)(canvas.get_height() - textheight) / 2,
                message);
  }
  else {
    canvas.clear(wf->GetLook().background_color);
  }
}

Window*
pnlTaskCalculator::Load(gcc_unused SingleWindow &parent, TabBarControl* _wTabBar,
    WndForm* _wf, bool* _task_modified)
{
  if (protected_task_manager == NULL)
    return NULL;

  assert(_wTabBar);
  wTabBar = _wTabBar;

  assert(_wf);
  wf = _wf;

  assert(_task_modified);
  task_modified = _task_modified;

  Window *wCalc =
      LoadWindow(dlgTaskManager::CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                 _T("IDR_XML_TASKCALCULATOR_L") : _T("IDR_XML_TASKCALCULATOR"));
  assert(wCalc);

  lazy_loaded = false;

#ifdef OLD_TASK
    GlidePolar::SetCruiseEfficiency(CRUISE_EFFICIENCY_enter);
#endif

    return wCalc;
}

