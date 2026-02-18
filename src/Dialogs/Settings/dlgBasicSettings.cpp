// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Device/MultipleDevices.hpp"
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
#include "Units/Group.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

#include <math.h>

enum ControlIndex {
  Crew,
  Ballast,
  WingLoading,
  Bugs,
  QNH,
  Altitude,
  Temperature,
};

class FlightSetupPanel final
  : public RowFormWidget, DataFieldListener {
  UI::PeriodicTimer timer{[this]{ OnTimer(); }};

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
  void SetCrewMass(double _crew_mass) {
    polar_settings.glide_polar_task.SetCrewMass(_crew_mass);
    PublishPolarSettings();
    SetBallast();
    
    // Send to external devices
    if (backend_components->devices != nullptr) {
      MessageOperationEnvironment env;
      backend_components->devices->PutCrewMass(_crew_mass, env);
    }
  }

  void SetBallast();
  void SetBallastTimer(bool active);
  void FlipBallastTimer();

  void PublishPolarSettings() {
    backend_components->SetTaskPolar(polar_settings);
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
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    RowFormWidget::Show(rc);
    timer.Schedule(std::chrono::milliseconds(500));

    OnTimer();
    SetButtons();
    SetBallast();
  }

  void Hide() noexcept override {
    timer.Cancel();
    RowFormWidget::Hide();
  }

private:
  void OnTimer();

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
FlightSetupPanel::SetButtons()
{
  dump_button->SetEnabled(polar_settings.glide_polar_task.HasBallast());

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  dump_button->SetCaption(settings.polar.ballast_timer_active
                          ? _("Stop") : _("Dump"));
}

void
FlightSetupPanel::SetBallast()
{
  const bool ballastable = polar_settings.glide_polar_task.IsBallastable();
  SetRowVisible(Ballast, ballastable);
  if (ballastable) {
    WndProperty &control = GetControl(Ballast);
    auto *df = dynamic_cast<DataFieldFloat *>(control.GetDataField());
    if (df != nullptr) {
      const double db = 5;
      /* Use configured max_ballast if available, otherwise
         fall back to 400 L as a reasonable UI ceiling */
      double ui_max = polar_settings.glide_polar_task.GetMaxBallast();
      if (ui_max < db)
        ui_max = 400.0;
      df->SetMax(db * ceil(ui_max / db));
    }
    LoadValue(Ballast, polar_settings.glide_polar_task.GetBallastLitres());
  }

  const auto wl = polar_settings.glide_polar_task.GetWingLoading();
  SetRowVisible(WingLoading, wl > 0);
  if (wl > 0)
    LoadValue(WingLoading, wl, UnitGroup::WING_LOADING);

  if (backend_components->devices != nullptr) {
    const auto &polar = polar_settings.glide_polar_task;
    const double ref_mass = polar.GetReferenceMass();
    if (ref_mass > 0) {
      MessageOperationEnvironment env;
      backend_components->devices->PutBallast(polar.GetBallastFraction(),
                                              polar.GetBallastOverload(),
                                              env);
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

  if (backend_components->devices != nullptr) {
    MessageOperationEnvironment env;
    backend_components->devices->PutBugs(bugs, env);
  }
}

void
FlightSetupPanel::SetQNH(AtmosphericPressure qnh)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  settings_computer.pressure = qnh;
  settings_computer.pressure_available.Update(basic.clock);

  if (backend_components->devices != nullptr) {
    MessageOperationEnvironment env;
    backend_components->devices->PutQNH(qnh, env);
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
FlightSetupPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(Crew, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetCrewMass(Units::ToSysMass(dff.GetValue()));
  } else if (IsDataField(Ballast, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBallastLitres(dff.GetValue());
  } else if (IsDataField(Bugs, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBugs(1 - (dff.GetValue() / 100));
  } else if (IsDataField(QNH, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetQNH(Units::FromUserPressure(dff.GetValue()));
  }
}

void
FlightSetupPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const Plane &plane = CommonInterface::GetComputerSettings().plane;

  AddFloat(_("Crew"),
           _("All masses loaded to the glider beyond the empty weight including pilot and copilot, but not water ballast."),
           "%.0f %s", "%.0f",
           0, Units::ToUserMass(300), 5, false, UnitGroup::MASS,
           polar_settings.glide_polar_task.GetCrewMass(),
           this);

  const double db = 5;
  AddFloat(_("Ballast"),
           _("Ballast of the glider. Press \"Dump/Stop\" to toggle count-down of the ballast volume according to the dump rate specified in the configuration settings."),
           "%.0f l", "%.0f",
           0, db*ceil(plane.max_ballast/db), db, false, 0,
           this);

  WndProperty *wing_loading = AddFloat(_("Wing loading"),
                                       _("The current wing loading, calculated from the glider's empty weight, crew weight, and ballast."),
                                       "%.1f %s", "%.0f", 0,
                                       300, 5,
                                       false, UnitGroup::WING_LOADING,
                                       0);
  wing_loading->SetReadOnly(true);

  AddFloat(_("Bugs"), /* xgettext:no-c-format */
           _("How clean the glider is. Set to 0% for clean, larger numbers as the wings "
               "pick up bugs or get wet. 50% indicates the glider's sink rate is doubled."),
           "%.0f %%", "%.0f",
           0, 50, 1, false,
           (1 - polar_settings.bugs) * 100,
           this);

  WndProperty *wp;
  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration. This is set automatically if Vega is connected."),
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

  AddReadOnly(_("Altitude"), NULL, "%.0f %s",
              UnitGroup::ALTITUDE, 0);

  wp = AddFloat(_("Max. temp."),
                _("Set to forecast ground temperature. Used by convection estimator (temperature trace page of Analysis dialog)."),
                "%.0f %s", "%.0f",
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
FlightSetupPanel::Save(bool &changed) noexcept
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
dlgBasicSettingsShowModal()
{
  FlightSetupPanel *instance = new FlightSetupPanel();

  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  StaticString<128> caption(_("Flight Setup"));
  caption.append(" - ");
  caption.append(plane.polar_name);

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      caption, instance);
  instance->SetDumpButton(dialog.AddButton(_("Dump"), [instance](){
    instance->FlipBallastTimer();
  }));

  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
