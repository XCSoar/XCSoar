// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
#include "Units/Group.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/InternalLink.hpp"
#include "Dialogs/Weather/MosmixTemperature.hpp"
#include "Weather/MOSMIX/AutoUpdate.hpp"
#include "time/BrokenDateTime.hpp"
#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <math.h>

#include <optional>

/* the enumerator Temperature below hides the class of the same
   name; give the class a second name while it is still
   reachable */
using TemperatureValue = Temperature;

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

  /**
   * The temperature a forecast put into the field, if one arrived.
   * Save() compares against it to tell a value the pilot typed from
   * one that was merely accepted.
   */
  std::optional<TemperatureValue> offered_temperature;

  /** fetches in the background; cancelled when this panel goes away */
  ForecastTemperatureFetcher forecast_fetcher;

  void ShowForecastTemperature() noexcept;

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
    ActionInterface::SetCrewMass(_crew_mass);
    SetBallast();
  }

  void SetBallast();
  void SetBallastTimer(bool active);
  void FlipBallastTimer();

  void SetBallastLitres(double ballast_litres) {
    ActionInterface::SetBallastLitres(ballast_litres);
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
}

void
FlightSetupPanel::SetBallastTimer(bool active)
{
  if (active && CommonInterface::GetComputerSettings().plane.dump_time == 0) {
    if (ShowMessageBox(_("Ballast dump time is 0 in plane profile.\n"
                         "Open Plane configuration now?"),
                       _("Flight Setup"),
                       MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
      HandleInternalLink("xcsoar://config/planes");

    return;
  }

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
  ActionInterface::SetBugs(bugs);
}

void
FlightSetupPanel::SetQNH(AtmosphericPressure qnh)
{
  ActionInterface::SetQNH(qnh, true);
  RefreshAltitudeControl();
}

void
FlightSetupPanel::OnTimer()
{
  const PolarSettings &settings = CommonInterface::GetComputerSettings().polar;

  if (settings.ballast_timer_active) {
    /* dump updates the polar on the process timer; push that to
       devices and refresh the dialog */
    ActionInterface::SetBallastLitres(
        polar_settings.glide_polar_task.GetBallastLitres());
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
                _("The day's maximum ground temperature. A dry adiabat "
                  "from it against the outside air temperature measured "
                  "while climbing gives the estimated thermal ceiling; "
                  "the cloud base needs a humidity probe as well. Both "
                  "are drawn on the temperature trace page of the "
                  "Analysis dialog, once the glider has climbed through "
                  "the layer."),
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

  /* Started rather than waited for: the dialog opens on the value it
     has, and the field changes under the pilot only if an answer
     turns up.  Nothing happens at all when one was already fetched
     today, or the pilot has set the value by hand today, or there is
     no fix to pick a station by. */
  const auto opened_with = settings.forecast_temperature;
  forecast_fetcher.Start([this, opened_with](TemperatureValue value){
    if (fabs(GetValueFloat(Temperature) - opened_with.ToUser()) > 0.01)
      /* the pilot has typed something in the meantime; a forecast
         does not get to overwrite that */
      return;

    offered_temperature = value;
    LoadValue(Temperature, value.ToKelvin(), UnitGroup::TEMPERATURE);
    ShowForecastTemperature();
  });
}

void
FlightSetupPanel::ShowForecastTemperature() noexcept
{
  auto &df = (DataFieldFloat &)GetDataField(Temperature);

  /* The row states the number and names where it came from:
     "18.6 degC  DWD MOSMIX".  A greyed field says the pilot did not
     set this, but not who did.

     DataFieldFloat renders its display format with (value, unit), so
     the unit symbol moves into the literal and the field's own unit is
     cleared -- the " %s" SetFormat() appends then formats to nothing.
     A unit symbol cannot carry a per cent sign, so the result is still
     a safe format string.

     One decimal, because that is what the forecast resolves to.
     Rounding it to the whole degree the selection list steps in is
     what made the same number appear twice in that list. */
  StaticString<64> format;
  format.Format("%%.1f %s  DWD MOSMIX", Units::GetTemperatureName());

  df.SetUnits("");
  df.SetFormat(format);

  /* Disabled rather than read-only.  A read-only row still takes the
     tap and answers it with WndProperty::ShowFullContent() -- a
     full-screen dialog holding one short number, which is not what
     pressing a field that says "not yours to set" should do.  A
     disabled row is skipped by WindowList::FindAt(), so the tap lands
     nowhere, and it cannot be reached with the cursor keys either. */
  SetRowEnabled(Temperature, false);
  GetControl(Temperature).RefreshDisplay();
}

bool
FlightSetupPanel::Save(bool &changed) noexcept
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  double forecast_temperature = settings.forecast_temperature.ToKelvin();
  if (SaveValue(Temperature, UnitGroup::TEMPERATURE, forecast_temperature)) {
    const auto value =
      TemperatureValue::FromKelvin(forecast_temperature);

    /* A value the pilot typed outranks the forecast for the rest of
       the day.  Merely closing the dialog on a value a forecast put
       there is not that, so the two are told apart by comparison. */
    if (!offered_temperature.has_value() ||
        fabs(offered_temperature->ToKelvin() - forecast_temperature) > 0.01) {
      if (const BrokenDate today = BrokenDateTime::NowUTC();
          today.IsPlausible())
        MOSMIX::RememberManualEntry(today);
    }

    settings.forecast_temperature = value;
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
