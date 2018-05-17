/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Blackboard/DeviceBlackboard.hpp"
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
#include "Units/Group.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Event/Timer.hpp"

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

class FlightSetupPanel final
  : public RowFormWidget, DataFieldListener,
    private Timer,
    public ActionListener {
  Button *dump_button;

  PolarSettings &polar_settings;

  double last_altitude;

public:
  FlightSetupPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dump_button(NULL),
     polar_settings(CommonInterface::SetComputerSettings().polar),
     last_altitude(-2)
  {}

  void SetDumpButton(Button *_dump_button) {
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

  void SetBallastLitres(double ballast_litres) {
    polar_settings.glide_polar_task.SetBallastLitres(ballast_litres);
    PublishPolarSettings();
    SetButtons();
    SetBallast();
  }

  void ShowAltitude(double altitude);
  void RefreshAltitudeControl();
  void SetBugs(double bugs);
  void SetQNH(AtmosphericPressure qnh);

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  virtual void Show(const PixelRect &rc) override {
    RowFormWidget::Show(rc);
    Timer::Schedule(500);

    OnTimer();
    SetButtons();
    SetBallast();
  }

  virtual void Hide() override {
    Timer::Cancel();
    RowFormWidget::Hide();
  }

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

private:
  /* virtual methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

  /* virtual methods from Timer */
  virtual void OnTimer() override;
};

void
FlightSetupPanel::SetButtons()
{
  dump_button->SetVisible(polar_settings.glide_polar_task.HasBallast());

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

  const auto wl = polar_settings.glide_polar_task.GetWingLoading();
  SetRowVisible(WingLoading, wl > 0);
  if (wl > 0)
    LoadValue(WingLoading, wl, UnitGroup::WING_LOADING);

  if (device_blackboard != NULL) {
    const Plane &plane = CommonInterface::GetComputerSettings().plane;
    if (plane.dry_mass > 0) {
      auto fraction = polar_settings.glide_polar_task.GetBallast();
      auto overload = (plane.dry_mass + fraction * plane.max_ballast) /
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
FlightSetupPanel::ShowAltitude(double altitude)
{
  if (fabs(altitude - last_altitude) >= 1) {
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
FlightSetupPanel::SetBugs(double bugs) {
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

void
FlightSetupPanel::OnModified(DataField &df)
{
  if (IsDataField(Ballast, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBallastLitres(dff.GetAsFixed());
  } else if (IsDataField(Bugs, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBugs(1 - (dff.GetAsFixed() / 100));
  } else if (IsDataField(QNH, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetQNH(Units::FromUserPressure(dff.GetAsFixed()));
  }
}

void
FlightSetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const Plane &plane = CommonInterface::GetComputerSettings().plane;

  const double db = 5;
  AddFloat(_("Ballast"),
           _("Ballast of the glider.  Increase this value if the pilot/cockpit load is greater than the reference pilot weight of the glide polar (typically 75kg).  Press ENTER on this field to toggle count-down of the ballast volume according to the dump rate specified in the configuration settings."),
           _T("%.0f l"), _T("%.0f"),
           0, db*ceil(plane.max_ballast/db), db, false, 0,
           this);

  WndProperty *wing_loading = AddFloat(_("Wing loading"), nullptr,
                                       _T("%.1f %s"), _T("%.0f"), 0,
                                       300, 5,
                                       false, UnitGroup::WING_LOADING,
                                       0);
  wing_loading->SetReadOnly(true);

  AddFloat(_("Bugs"), /* xgettext:no-c-format */
           _("How clean the glider is. Set to 0% for clean, larger numbers as the wings "
               "pick up bugs or gets wet.  50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           0, 50, 1, false,
           (1 - polar_settings.bugs) * 100,
           this);

  WndProperty *wp;
  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                GetUserPressureFormat(true), GetUserPressureFormat(),
                Units::ToUserPressure(Units::ToSysUnit(850, Unit::HECTOPASCAL)),
                Units::ToUserPressure(Units::ToSysUnit(1300, Unit::HECTOPASCAL)),
                GetUserPressureStep(), false,
                Units::ToUserPressure(settings.pressure), this);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetPressureName());
    wp->RefreshDisplay();
  }

  AddReadOnly(_("Altitude"), NULL, _T("%.0f %s"),
              UnitGroup::ALTITUDE, 0);

  wp = AddFloat(_("Max. temp."),
                _("Set to forecast ground temperature.  Used by convection estimator (temperature trace page of Analysis dialog)"),
                _T("%.0f %s"), _T("%.0f"),
                Temperature::FromCelsius(-50).ToUser(),
                Temperature::FromCelsius(60).ToUser(),
                1, false,
                settings.forecast_temperature.ToUser());
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetTemperatureName());
    wp->RefreshDisplay();
  }
}

bool
FlightSetupPanel::Save(bool &changed)
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  double forecast_temperature = settings.forecast_temperature.ToKelvin();
  if (SaveValue(Temperature, UnitGroup::TEMPERATURE, forecast_temperature)) {
    settings.forecast_temperature = Temperature::FromKelvin(forecast_temperature);
    changed = true;
  }

  return true;
}

void
FlightSetupPanel::OnAction(int id)
{
  if (id == DUMP)
    FlipBallastTimer();
}

void
dlgBasicSettingsShowModal()
{
  FlightSetupPanel *instance = new FlightSetupPanel();

  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  StaticString<128> caption(_("Flight Setup"));
  caption.append(_T(" - "));
  caption.append(plane.polar_name);

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), caption, instance);
  instance->SetDumpButton(dialog.AddButton(_("Dump"), *instance, DUMP));
  dialog.AddButton(_("OK"), mrOK);

  dialog.ShowModal();
}
