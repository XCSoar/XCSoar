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
#include "DeviceBlackboard.hpp"
#include "ComputerSettings.hpp"
#include "Units/Units.hpp"
#include "Units/UnitsFormatter.hpp"
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
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
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

  GlidePolar glide_polar;
  bool glide_polar_modified;

  fixed last_altitude;

public:
  FlightSetupPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(78)),
     dump_button(NULL),
     glide_polar(CommonInterface::GetComputerSettings().glide_polar_task),
     glide_polar_modified(false),
     last_altitude(-2)
  {}

  void SetDumpButton(WndButton *_dump_button) {
    dump_button = _dump_button;
  }

  void SetButtons();
  void SetBallast();
  void SavePolar();
  void SetBallastTimer(bool active);
  void FlipBallastTimer();

  void SetBallastLitres(fixed ballast_litres) {
    glide_polar.SetBallastLitres(ballast_litres);
    glide_polar_modified = true;
    SetButtons();
    SetBallast();
  }

  void SetBugs(fixed bugs) {
    glide_polar.SetBugs(bugs);
    glide_polar_modified = true;
  }

  void ShowAltitude(fixed altitude);
  void RefreshAltitudeControl();
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
  dump_button->set_visible(glide_polar.HasBallast());

  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  dump_button->SetCaption(settings.ballast_timer_active
                          ? _("Stop") : _("Dump"));
}

void
FlightSetupPanel::SetBallast()
{
  WndProperty &ballast_control = GetControl(Ballast);
  if (glide_polar.IsBallastable()) {
    ballast_control.show();
    LoadValue(Ballast, glide_polar.GetBallastLitres());
  } else
    ballast_control.hide();

  WndProperty &wing_loading_control = GetControl(WingLoading);
  const fixed wl = glide_polar.GetWingLoading();
  if (positive(wl)) {
    wing_loading_control.show();
    LoadValue(WingLoading, wl);
  } else
    wing_loading_control.hide();
}

void
FlightSetupPanel::SavePolar()
{
  if (!glide_polar_modified)
    return;

  glide_polar_modified = false;
  CommonInterface::SetComputerSettings().glide_polar_task = glide_polar;

  if (protected_task_manager != NULL)
    protected_task_manager->SetGlidePolar(glide_polar);
}

void
FlightSetupPanel::SetBallastTimer(bool active)
{
  if (!glide_polar.HasBallast())
    active = false;

  ComputerSettings &settings = CommonInterface::SetComputerSettings();
  if (active == settings.ballast_timer_active)
    return;

  if (active)
    /* apply the new ballast settings before starting the timer */
    SavePolar();

  settings.ballast_timer_active = active;
  SetButtons();
}

void
FlightSetupPanel::FlipBallastTimer()
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  SetBallastTimer(!settings.ballast_timer_active);
}

void
FlightSetupPanel::ShowAltitude(fixed altitude)
{
  if (fabs(altitude - last_altitude) >= fixed_one) {
    last_altitude = altitude;
    LoadValue(Altitude, altitude, UnitGroup::ALTITUDE);
  }

  GetControl(Altitude).show();
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
    GetControl(Altitude).hide();
}

void
FlightSetupPanel::SetQNH(AtmosphericPressure qnh)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  settings_computer.pressure = qnh;
  settings_computer.pressure_available.Update(basic.clock);

  if (device_blackboard != NULL)
    device_blackboard->SetQNH(qnh);

  RefreshAltitudeControl();
}

void
FlightSetupPanel::OnTimer()
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  if (settings.ballast_timer_active && !glide_polar_modified) {
    /* get new GlidePolar values */
    glide_polar = CommonInterface::GetComputerSettings().glide_polar_task;

    /* display the new values on the screen */
    SetBallast();
  }

  RefreshAltitudeControl();
}

static void
OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode)
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
OnBugsData(DataField *_Sender, DataField::DataAccessKind_t Mode)
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
OnQnhData(DataField *_Sender, DataField::DataAccessKind_t Mode)
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

  AddFloat(_("Bugs"),
           _("How clean the glider is. Set to 0% for clean, larger numbers as the wings pick up bugs or gets wet.  50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           fixed_zero, fixed(50), fixed_one, false,
           (fixed_one - glide_polar.GetBugs()) * 100,
           OnBugsData);

  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                Units::GetFormatUserPressure(), Units::GetFormatUserPressure(),
                Units::ToUserPressure(Units::ToSysUnit(fixed(850), Unit::HECTOPASCAL)),
                Units::ToUserPressure(Units::ToSysUnit(fixed(1300), Unit::HECTOPASCAL)),
                Units::PressureStep(), false,
                Units::ToUserPressure(settings.pressure),
                OnQnhData);
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

  changed |= glide_polar_modified;

  SavePolar();

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
