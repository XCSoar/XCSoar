/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
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
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Event/Timer.hpp"
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

class FlightSetupPanel final
  : public RowFormWidget, DataFieldListener,
    private Timer,
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
  virtual void OnSpecial(DataField &df) override;

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
  if (fabs(altitude - last_altitude) >= fixed(1)) {
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

void
FlightSetupPanel::OnModified(DataField &df)
{
  if (IsDataField(Ballast, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBallastLitres(dff.GetAsFixed());
  } else if (IsDataField(Bugs, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetBugs(fixed(1) - (dff.GetAsFixed() / 100));
  } else if (IsDataField(QNH, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    SetQNH(Units::FromUserPressure(dff.GetAsFixed()));
  }
}

void
FlightSetupPanel::OnSpecial(DataField &df)
{
  if (IsDataField(Ballast, df))
    FlipBallastTimer();
}

void
FlightSetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  AddFloat(_("Ballast"),
           _("Ballast of the glider.  Increase this value if the pilot/cockpit load is greater than the reference pilot weight of the glide polar (typically 75kg).  Press ENTER on this field to toggle count-down of the ballast volume according to the dump rate specified in the configuration settings."),
           _T("%.0f l"), _T("%.0f"),
           fixed(0), fixed(500), fixed(5), false,
           fixed(0),
           this);

  AddReadOnly(_("Wing loading"), NULL, _T("%.1f kg/m2"), fixed(0));

  AddFloat(_("Bugs"), /* xgettext:no-c-format */
           _("How clean the glider is. Set to 0% for clean, larger numbers as the wings "
               "pick up bugs or gets wet.  50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           fixed(0), fixed(50), fixed(1), false,
           (fixed(1) - polar_settings.bugs) * 100,
           this);

  WndProperty *wp;
  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                GetUserPressureFormat(true), GetUserPressureFormat(),
                Units::ToUserPressure(Units::ToSysUnit(fixed(850), Unit::HECTOPASCAL)),
                Units::ToUserPressure(Units::ToSysUnit(fixed(1300), Unit::HECTOPASCAL)),
                GetUserPressureStep(), false,
                Units::ToUserPressure(settings.pressure), this);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetPressureName());
    wp->RefreshDisplay();
  }

  AddReadOnly(_("Altitude"), NULL, _T("%.0f %s"),
              UnitGroup::ALTITUDE, fixed(0));

  wp = AddFloat(_("Max. temp."),
                _("Set to forecast ground temperature.  Used by convection estimator (temperature trace page of Analysis dialog)"),
                _T("%.0f %s"), _T("%.0f"),
                Units::ToUserTemperature(CelsiusToKelvin(fixed(-50))),
                Units::ToUserTemperature(CelsiusToKelvin(fixed(60))),
                fixed(1), false,
                Units::ToUserTemperature(settings.forecast_temperature));
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
