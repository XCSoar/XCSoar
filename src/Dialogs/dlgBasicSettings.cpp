/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Protection.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "ComputerSettings.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "DataField/Float.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Form/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Compiler.h"

#include <math.h>

enum ControlIndex {
  Ballast,
  WingLoading,
  Bugs,
  QNH,
  Altitude,
  Temperature,
};

enum Actions {
  DUMP = 100,
};

class FlightSetupPanel : public RowFormWidget,
                         public ActionListener {
  WndButton *dump_button;

  PolarSettings &polar_settings;

  fixed last_altitude;

public:
  FlightSetupPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dump_button(NULL),
     polar_settings(CommonInterface::SetComputerSettings().polar),
     last_altitude(-2)
  {}

  void SetDumpButton(WndButton *_dump_button) {
    dump_button = _dump_button;
  }

  void SetButtons();
  void SetBallast();
  void SetBallastTimer(bool active);
  void FlipBallastTimer();

  void PublishPolarSettings() {
    if (protected_task_manager != NULL)
      protected_task_manager->SetGlidePolar(polar_settings.glide_polar_task);
  }

  void SetBallastLitres(fixed ballast_litres) {
    polar_settings.glide_polar_task.SetBallastLitres(ballast_litres);
    PublishPolarSettings();
    SetButtons();
    SetBallast();
  }

  void ShowAltitude(fixed altitude);
  void RefreshAltitudeControl();
  void SetBugs(fixed bugs);
  void SetQNH(AtmosphericPressure qnh);

  void OnTimer();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);

  virtual void OnAction(int id);
};

static FlightSetupPanel *instance;

void
FlightSetupPanel::SetButtons()
{
  dump_button->set_visible(polar_settings.glide_polar_task.HasBallast());

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  dump_button->SetCaption(settings.polar.ballast_timer_active
                          ? _("Stop") : _("Dump"));
}

void
FlightSetupPanel::SetBallast()
{
  const bool ballastable = polar_settings.glide_polar_task.IsBallastable();
  SetRowVisible(Ballast, ballastable);
  if (ballastable)
    LoadValue(Ballast, polar_settings.glide_polar_task.GetBallastLitres());

  const fixed wl = polar_settings.glide_polar_task.GetWingLoading();
  SetRowVisible(WingLoading, positive(wl));
  if (positive(wl))
    LoadValue(WingLoading, wl);

  if (device_blackboard != NULL) {
    const Plane &plane = CommonInterface::GetComputerSettings().plane;
    if (positive(plane.dry_mass)) {
      fixed fraction = polar_settings.glide_polar_task.GetBallast();
      fixed overload = (plane.dry_mass + fraction * plane.max_ballast) /
        plane.dry_mass;

      MessageOperationEnvironment env;
      device_blackboard->SetBallast(fraction, overload, env);
    }
  }
}

void
FlightSetupPanel::SetBallastTimer(bool active)
{
  if (!polar_settings.glide_polar_task.HasBallast())
    active = false;

  PolarSettings &settings = CommonInterface::SetComputerSettings().polar;
  if (active == settings.ballast_timer_active)
    return;

  settings.ballast_timer_active = active;
  SetButtons();
}

void
FlightSetupPanel::FlipBallastTimer()
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  SetBallastTimer(!settings.polar.ballast_timer_active);
}

void
FlightSetupPanel::ShowAltitude(fixed altitude)
{
  if (fabs(altitude - last_altitude) >= fixed_one) {
    last_altitude = altitude;
    LoadValue(Altitude, altitude, UnitGroup::ALTITUDE);
  }

  ShowRow(Altitude);
}

void
FlightSetupPanel::RefreshAltitudeControl()
{
  const NMEAInfo &basic = CommonInterface::Basic();
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  if (basic.pressure_altitude_available && settings_computer.pressure_available)
    ShowAltitude(settings_computer.pressure.PressureAltitudeToQNHAltitude(
                 basic.pressure_altitude));
  else if (basic.baro_altitude_available)
    ShowAltitude(basic.baro_altitude);
  else
    HideRow(Altitude);
}

void
FlightSetupPanel::SetBugs(fixed bugs) {
  polar_settings.SetBugs(bugs);
  PublishPolarSettings();

  if (device_blackboard != NULL) {
    MessageOperationEnvironment env;
    device_blackboard->SetBugs(bugs, env);
  }
}

void
FlightSetupPanel::SetQNH(AtmosphericPressure qnh)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  settings_computer.pressure = qnh;
  settings_computer.pressure_available.Update(basic.clock);

  if (device_blackboard != NULL) {
    MessageOperationEnvironment env;
    device_blackboard->SetQNH(qnh, env);
  }

  RefreshAltitudeControl();
}

void
FlightSetupPanel::OnTimer()
{
  const PolarSettings &settings = CommonInterface::GetComputerSettings().polar;

  if (settings.ballast_timer_active) {
    /* display the new values on the screen */
    SetBallast();
  }

  RefreshAltitudeControl();
}

static void
OnBallastData(DataField *Sender, DataField::DataAccessMode Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;

  switch (Mode) {
  case DataField::daSpecial:
    instance->FlipBallastTimer();
    break;

  case DataField::daChange:
    instance->SetBallastLitres(df.GetAsFixed());
    break;
  }
}

static void
OnBugsData(DataField *_Sender, DataField::DataAccessMode Mode)
{
  DataFieldFloat *Sender = (DataFieldFloat *)_Sender;

  switch (Mode) {
  case DataField::daChange:
    instance->SetBugs(fixed_one - (Sender->GetAsFixed() / 100));
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnQnhData(DataField *_Sender, DataField::DataAccessMode Mode)
{
  DataFieldFloat *Sender = (DataFieldFloat *)_Sender;

  switch (Mode) {
  case DataField::daChange:
    instance->SetQNH(Units::FromUserPressure(Sender->GetAsFixed()));
    break;

  case DataField::daSpecial:
    return;
  }
}

void
FlightSetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  AddFloat(_("Ballast"),
           _("Ballast of the glider.  Increase this value if the pilot/cockpit load is greater than the reference pilot weight of the glide polar (typically 75kg).  Press ENTER on this field to toggle count-down of the ballast volume according to the dump rate specified in the configuration settings."),
           _T("%.0f l"), _T("%.0f"),
           fixed_zero, fixed(500), fixed(5), false,
           fixed_zero,
           OnBallastData);

  WndProperty *wp =
    AddFloat(_("Wing loading"), NULL,
             _T("%.1f kg/m2"), _T("%.1f"),
             fixed_zero, fixed_zero, fixed_zero, false,
             fixed_zero);
  wp->SetReadOnly();

  AddFloat(_("Bugs"), /* xgettext:no-c-format */
           _("How clean the glider is. Set to 0% for clean, larger numbers as the wings "
               "pick up bugs or gets wet.  50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           fixed_zero, fixed(50), fixed_one, false,
           (fixed_one - polar_settings.bugs) * 100,
           OnBugsData);

  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                GetUserPressureFormat(), GetUserPressureFormat(),
                Units::ToUserPressure(Units::ToSysUnit(fixed(850), Unit::HECTOPASCAL)),
                Units::ToUserPressure(Units::ToSysUnit(fixed(1300), Unit::HECTOPASCAL)),
                GetUserPressureStep(), false,
                Units::ToUserPressure(settings.pressure), OnQnhData);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetPressureName());
    wp->RefreshDisplay();
  }

  wp = AddFloat(_("Altitude"), NULL,
                _T("%.0f %s"), _T("%.0f"),
                fixed_zero, fixed_zero, fixed_zero, false,
                UnitGroup::ALTITUDE, fixed_zero);
  wp->SetReadOnly();

  wp = AddFloat(_("Max. temp."),
                _("Set to forecast ground temperature.  Used by convection estimator (temperature trace page of Analysis dialog)"),
                _T("%.0f %s"), _T("%.0f"),
                Units::ToUserTemperature(CelsiusToKelvin(fixed(-50))),
                Units::ToUserTemperature(CelsiusToKelvin(fixed(60))),
                fixed_one, false,
                Units::ToUserTemperature(settings.forecast_temperature));
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetTemperatureName());
    wp->RefreshDisplay();
  }

  OnTimer();
  SetButtons();
  SetBallast();
}

bool
FlightSetupPanel::Save(bool &changed, bool &require_restart)
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  changed |= SaveValue(Temperature, UnitGroup::TEMPERATURE,
                       settings.forecast_temperature);

  return true;
}

void
FlightSetupPanel::OnAction(int id)
{
  if (id == DUMP)
    FlipBallastTimer();
}

/**
 * This function is called repeatedly by the timer and updates the
 * current altitude and ballast. The ballast can change without user
 * input due to the dump function.
 */
static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  instance->OnTimer();
}

void
dlgBasicSettingsShowModal()
{
  instance = new FlightSetupPanel();

  WidgetDialog dialog(_("Flight Setup"), instance);
  dialog.SetTimerNotify(OnTimerNotify);
  instance->SetDumpButton(dialog.AddButton(_("Dump"), instance, DUMP));
  dialog.AddButton(_("OK"), mrOK);

  dialog.ShowModal();
}
