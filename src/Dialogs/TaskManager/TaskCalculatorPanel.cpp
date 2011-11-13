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

#include "TaskCalculatorPanel.hpp"
#include "Internal.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Interface.hpp"
#include "Protection.hpp"
#include "Units/Units.hpp"
#include "DataField/Float.hpp"
#include "Screen/SingleWindow.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "DeviceBlackboard.hpp"
#include "Screen/Layout.hpp"
#include "Form/Edit.hpp"
#include "Form/Button.hpp"
#include "Form/TabBar.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Icon.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Look.hpp"
#include "MainWindow.hpp"
#include "Language/Language.hpp"

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static TaskCalculatorPanel *instance;

void
TaskCalculatorPanel::GetCruiseEfficiency()
{
  cruise_efficiency = XCSoarInterface::Calculated().task_stats.cruise_efficiency;
}

void
TaskCalculatorPanel::Refresh()
{
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

  WndProperty* wp;

  // update outputs
  wp = (WndProperty*)form.FindByName(_T("prpAATEst"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat((
        common_stats.task_time_remaining +
        common_stats.task_time_elapsed) / 60);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)form.FindByName(_T("prpAATTime"));
  if (wp) {
    if (task_stats.has_targets) {
      DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
      df.SetAsFloat(
          protected_task_manager->GetOrderedTaskBehaviour().aat_min_time / 60);
      wp->RefreshDisplay();
    } else {
      wp->hide();
    }
  }

  fixed rPlanned = task_stats.total.solution_planned.IsDefined()
    ? task_stats.total.solution_planned.vector.distance
    : fixed_zero;

  wp = (WndProperty*)form.FindByName(_T("prpDistance"));
  if (wp != NULL && positive(rPlanned)) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserDistance(rPlanned));
    df.SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetVerticalSpeedName());
    df.Set(Units::ToUserVSpeed(CommonInterface::SettingsComputer().glide_polar_task.GetMC()));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpEffectiveMacCready"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetVerticalSpeedName());
    df.SetAsFloat(Units::ToUserVSpeed(emc));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpRange"));
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
    wp = (WndProperty*)form.FindByName(_T("prpSpeedRemaining"));
    assert(wp != NULL);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserTaskSpeed(
        task_stats.total.remaining_effective.get_speed()));
    df.SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  if (task_stats.total.travelled.IsDefined()) {
    wp = (WndProperty*)form.FindByName(_T("prpSpeedAchieved"));
    assert(wp != NULL);
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(Units::ToUserTaskSpeed(
        task_stats.total.travelled.get_speed()));
    df.SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)form.FindByName(_T("prpCruiseEfficiency"));
  if (wp) {
    DataFieldFloat *df = (DataFieldFloat *)wp->GetDataField();
    df->Set(task_stats.cruise_efficiency * fixed(100));
    wp->RefreshDisplay();
  }
}

static void
OnTargetClicked(gcc_unused WndButton &Sender)
{
  dlgTargetShowModal();
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  instance->Refresh();
}

static void
OnMacCreadyData(DataField *Sender, DataField::DataAccessKind_t Mode)
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
      instance->Refresh();
    }
    break;
  case DataField::daChange:
    MACCREADY = Units::ToSysVSpeed(df->GetAsFixed());
    ActionInterface::SetMacCready(MACCREADY);
    instance->Refresh();
    break;
  }
}

static void
OnCruiseEfficiencyData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;
  fixed clast = CommonInterface::SettingsComputer().glide_polar_task.GetCruiseEfficiency();
  (void)clast; // unused for now

  switch (Mode) {
  case DataField::daSpecial:
    instance->GetCruiseEfficiency();
    break;
  case DataField::daChange:
    instance->SetCruiseEfficiency(df.GetAsFixed() / 100);
    break;
  }
}

static void
OnWarningPaint(gcc_unused WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  if (instance->IsTaskModified()) {
    const TCHAR* message = _("Calculator excludes unsaved task changes!");
    canvas.select(Fonts::Title);
    const int textheight = canvas.text_height(message);

    const AirspaceLook &look = CommonInterface::main_window.look->map.airspace;
    const MaskedIcon *bmp = &look.intercept_icon;
    const int offsetx = bmp->GetSize().cx;
    const int offsety = canvas.get_height() - bmp->GetSize().cy;
    canvas.clear(COLOR_YELLOW);
    bmp->Draw(canvas, offsetx, offsety);
    canvas.set_background_color(COLOR_YELLOW);

    canvas.text(offsetx * 2 + Layout::Scale(2),
                (int)(canvas.get_height() - textheight) / 2,
                message);
  }
  else {
    canvas.clear(instance->GetLook().background_color);
  }
}

static gcc_constexpr_data CallBackTableEntry task_calculator_callbacks[] = {
  DeclareCallBackEntry(OnMacCreadyData),
  DeclareCallBackEntry(OnTargetClicked),
  DeclareCallBackEntry(OnCruiseEfficiencyData),
  DeclareCallBackEntry(OnWarningPaint),
  DeclareCallBackEntry(NULL)
};

void
TaskCalculatorPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  assert(protected_task_manager != NULL);

  instance = this;

  LoadWindow(task_calculator_callbacks, parent,
             Layout::landscape
             ? _T("IDR_XML_TASKCALCULATOR_L") : _T("IDR_XML_TASKCALCULATOR"));
}

void
TaskCalculatorPanel::Show(const PixelRect &rc)
{
  const GlidePolar& polar = CommonInterface::SettingsComputer().glide_polar_task;

  cruise_efficiency = polar.GetCruiseEfficiency();
  emc = XCSoarInterface::Calculated().task_stats.effective_mc;

  if (!XCSoarInterface::Calculated().common_stats.ordered_has_targets)
      ((WndButton *)form.FindByName(_T("prpRange")))->hide();

  wf.SetTimerNotify(OnTimerNotify);

  Refresh();

  XMLWidget::Show(rc);
}

void
TaskCalculatorPanel::Hide()
{
  wf.SetTimerNotify(NULL);
  XMLWidget::Hide();
}
