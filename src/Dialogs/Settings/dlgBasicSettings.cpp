// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/PickList.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/InternalLink.hpp"
#include "Look/DialogLook.hpp"
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Formatter/UserUnits.hpp"
#include "Atmosphere/Temperature.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/StaticString.hxx"

#include <chrono>
#include <cmath>

namespace {

/**
 * The upper end of the ballast row, in litres: the polar's maximum,
 * rounded up to the next 5 l, or 400 l when the polar has none.
 */
static double
BallastUiMax() noexcept
{
  constexpr double step = 5;
  double ui_max =
    CommonInterface::GetComputerSettings().polar.glide_polar_task
    .GetMaxBallast();
  if (ui_max < step)
    ui_max = 400;
  return step * std::ceil(ui_max / step);
}

/**
 * Flight Setup as one group of items.  Crew, ballast, bugs, QNH and
 * the forecast temperature open a list of stepped values.  Wing
 * loading always shows a value, or a badge naming the empty mass or
 * the wing area when one is missing.  Ballast stays on the list; a
 * glider with none carries a badge, and a missing dump time carries
 * one beside Empty when the volume is zero.  Altitude only displays
 * a value.
 * Dump and Close stay dialog buttons.
 * The forecast temperature is kept here until the dialog closes with
 * Close; Escape leaves the stored forecast alone.
 *
 * The dialog floats over the map, sized to this list, and is fitted
 * again when the list or the screen layout changes.
 */
class FlightSetup final {
  using BadgeStyle = GroupedListWidget::BadgeStyle;
  using ValueState = GroupedListWidget::ValueState;

  GroupedListWidget *list = nullptr;
  WidgetDialog *dialog = nullptr;
  Button *dump_button = nullptr;
  ButtonPanel *button_panel = nullptr;

  UI::PeriodicTimer timer{[this]{ OnTimer(); }};

  Temperature forecast;
  bool forecast_edited = false;

  struct Drawn {
    double crew_kg = 0;
    double ballast_l = 0;
    double wing = 0;
    double empty_mass = 0;
    double wing_area = 0;
    double bugs = 0;
    double qnh = 0;
    double altitude = 0;
    double temp_k = 0;
    bool show_ballast = false;
    bool show_altitude = false;
    bool no_dump_time = false;

    bool operator==(const Drawn &) const noexcept = default;
  };

  Drawn drawn;
  bool have_drawn = false;

public:
  FlightSetup() noexcept
    :forecast(CommonInterface::GetComputerSettings().forecast_temperature)
  {}

  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void SetDialog(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void SetDumpButton(Button *button) noexcept {
    dump_button = button;
  }

  void SetButtonPanel(ButtonPanel &panel) noexcept {
    button_panel = &panel;
  }

  void Refresh() noexcept;
  void SetButtons() noexcept;
  void Start() noexcept;
  void Stop() noexcept;
  void CommitForecast() noexcept;
  void FlipBallastTimer();

private:
  void OnTimer() noexcept;
  void EditCrew();
  void EditBallast();
  void EditPlane();
  void EditBugs();
  void EditQNH();
  void EditForecast();

  [[nodiscard]]
  Drawn Collect() const noexcept;

  void Build() noexcept;
};

FlightSetup::Drawn
FlightSetup::Collect() const noexcept
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const auto &polar = settings.polar.glide_polar_task;
  const NMEAInfo &basic = CommonInterface::Basic();

  Drawn next;
  next.crew_kg = polar.GetCrewMass();
  next.ballast_l = polar.GetBallastLitres();
  next.empty_mass = polar.GetEmptyMass();
  next.wing_area = polar.GetWingArea();
  next.wing = polar.GetWingLoading();
  next.bugs = settings.polar.bugs;
  next.qnh = settings.pressure.GetHectoPascal();
  next.temp_k = forecast.ToKelvin();
  next.show_ballast = polar.IsBallastable();
  next.no_dump_time = settings.plane.dump_time == 0;

  if (basic.pressure_altitude_available && settings.pressure_available) {
    next.show_altitude = true;
    next.altitude = settings.pressure.PressureAltitudeToQNHAltitude(
      basic.pressure_altitude);
  } else if (basic.baro_altitude_available) {
    next.show_altitude = true;
    next.altitude = basic.baro_altitude;
  }

  /* the old row ignored changes smaller than one metre */
  if (have_drawn && next.show_altitude && drawn.show_altitude &&
      std::fabs(next.altitude - drawn.altitude) < 1)
    next.altitude = drawn.altitude;

  return next;
}

void
FlightSetup::Build() noexcept
{
  list->AddGroup(nullptr);

  list->AddValue(_("Crew"),
                 _("All masses loaded to the glider beyond the empty "
                   "weight including pilot and copilot, but not water "
                   "ballast."),
                 [this](ValueState &state) {
                   StaticString<64> text;
                   FormatUserMass(drawn.crew_kg, text.data(), true);
                   state.text = text.c_str();
                 },
                 [this]{ EditCrew(); });

  list->AddValue(_("Ballast"),
                 _("Ballast of the glider. Press \"Dump/Stop\" to toggle "
                   "count-down of the ballast volume according to the dump "
                   "rate specified in the configuration settings."),
                 [this](ValueState &state) {
                   state.disabled = !drawn.show_ballast;
                   if (!drawn.show_ballast) {
                     state.badge = _("No ballast");
                     return;
                   }

                   if (drawn.ballast_l > 0) {
                     StaticString<64> text;
                     text.Format("%.0f l", drawn.ballast_l);
                     state.text = text.c_str();
                     if (drawn.no_dump_time) {
                       state.badge = _("No dump time");
                       state.badge_style = BadgeStyle::WARNING;
                     }
                     return;
                   }

                   /* Empty is the right-hand badge.  No dump time
                      sits to its left. */
                   state.badge = _("Empty");
                   if (drawn.no_dump_time) {
                     state.badge2 = _("No dump time");
                     state.badge_style2 = BadgeStyle::WARNING;
                   }
                 },
                 [this]{ EditBallast(); });

  /* wing loading is mass divided by wing area; a missing input is a
     badge, so the row stays on the list */
  list->AddValue(_("Wing loading"), nullptr,
                 [this](ValueState &state) {
                   const bool have_mass = drawn.empty_mass > 0;
                   const bool have_area = drawn.wing_area > 0;
                   if (have_mass && have_area) {
                     StaticString<64> text;
                     FormatUserWingLoading(drawn.wing, text.data(),
                                           text.capacity(), true);
                     state.text = text.c_str();
                     state.help =
                       _("The current wing loading, calculated from the "
                         "glider's empty weight, crew weight, and ballast. "
                         "Select the row to open the plane profile.");
                     return;
                   }

                   if (!have_mass && !have_area)
                     state.badge = _("No empty mass or wing area (m²)");
                   else if (!have_mass)
                     state.badge = _("No empty mass");
                   else
                     state.badge = _("No wing area (m²)");
                   state.badge_style = BadgeStyle::DANGER;
                   state.help =
                     _("Wing loading needs the empty mass and the wing "
                       "area (m²). Without them the polar cannot be "
                       "calculated. Select the row to open the plane "
                       "profile.");
                 },
                 [this]{ EditPlane(); });

  list->AddValue(_("Bugs"),
                 /* xgettext:no-c-format */
                 _("How clean the glider is. Set to 0% for clean, larger "
                   "numbers as the wings pick up bugs or get wet. 50% "
                   "indicates the glider's sink rate is doubled."),
                 [this](ValueState &state) {
                   const double bugs_percent = (1 - drawn.bugs) * 100;
                   if (bugs_percent > 0.5) {
                     StaticString<64> text;
                     text.Format("%.0f %%", bugs_percent);
                     state.text = text.c_str();
                   } else
                     state.badge = _("Clean");
                 },
                 [this]{ EditBugs(); });

  list->AddValue(_("QNH"),
                 _("Area pressure for barometric altimeter calibration. "
                   "This is set automatically if Vega is connected."),
                 [this](ValueState &state) {
                   StaticString<64> text;
                   FormatUserPressure(
                     AtmosphericPressure::HectoPascal(drawn.qnh),
                     text.data(), true);
                   state.text = text.c_str();
                 },
                 [this]{ EditQNH(); });

  list->AddValue(_("Altitude"), nullptr,
                 [this](ValueState &state) {
                   state.hidden = !drawn.show_altitude;
                   if (!drawn.show_altitude)
                     return;

                   StaticString<64> text;
                   FormatUserAltitude(drawn.altitude, text.data(), true);
                   state.text = text.c_str();
                 });

  list->AddValue(_("Max. temp."),
                 _("Set to forecast ground temperature. Used by convection "
                   "estimator (temperature trace page of Analysis dialog)."),
                 [this](ValueState &state) {
                   StaticString<64> text;
                   FormatUserTemperature(drawn.temp_k, text.data(), true);
                   state.text = text.c_str();
                 },
                 [this]{ EditForecast(); });
}

void
FlightSetup::Refresh() noexcept
{
  drawn = Collect();
  have_drawn = true;

  if (list->GetItemCount() == 0)
    Build();

  SetButtons();
  if (list->UpdateValues() && dialog != nullptr)
    dialog->RefitList();
}

void
FlightSetup::SetButtons() noexcept
{
  if (dump_button == nullptr)
    return;

  const ComputerSettings &settings =
    CommonInterface::GetComputerSettings();
  const auto &polar = settings.polar;
  const bool dumping = polar.ballast_timer_active;
  const bool no_dump_time = settings.plane.dump_time == 0;

  /* the caption is the reason Dump will not start: a dump time of 0
     in the plane profile.  With ballast on board the button stays
     pressable and offers to open that profile */
  if (dumping)
    dump_button->SetCaption(_("Stop"));
  else if (no_dump_time)
    dump_button->SetCaption(_("No dump time"));
  else
    dump_button->SetCaption(_("Dump"));

  dump_button->SetEnabled(dumping ||
                          polar.glide_polar_task.HasBallast());

  if (button_panel != nullptr)
    button_panel->ReselectToFirstEnabled();
}

void
FlightSetup::Start() noexcept
{
  timer.Schedule(std::chrono::milliseconds(500));
  OnTimer();
}

void
FlightSetup::Stop() noexcept
{
  timer.Cancel();
}

void
FlightSetup::CommitForecast() noexcept
{
  if (!forecast_edited)
    return;

  CommonInterface::SetComputerSettings().forecast_temperature = forecast;
}

void
FlightSetup::FlipBallastTimer()
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  bool active = !settings.polar.ballast_timer_active;

  if (active && settings.plane.dump_time == 0) {
    if (ShowMessageBox(_("Ballast dump time is 0 in plane profile.\n"
                         "Open Plane configuration now?"),
                       _("Flight Setup"),
                       MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
      HandleInternalLink("xcsoar://config/planes");

    return;
  }

  if (active &&
      !settings.polar.glide_polar_task.HasBallast())
    active = false;

  PolarSettings &polar = CommonInterface::SetComputerSettings().polar;
  if (active == polar.ballast_timer_active)
    return;

  polar.ballast_timer_active = active;
  SetButtons();
}

void
FlightSetup::OnTimer() noexcept
{
  const auto &polar = CommonInterface::GetComputerSettings().polar;
  if (polar.ballast_timer_active) {
    /* dump updates the polar on the process timer; push that to
       devices and refresh the dialog */
    ActionInterface::SetBallastLitres(
      polar.glide_polar_task.GetBallastLitres());
  }

  if (!have_drawn || Collect() != drawn)
    Refresh();
  else
    SetButtons();
}

void
FlightSetup::EditCrew()
{
  const auto &polar =
    CommonInterface::GetComputerSettings().polar.glide_polar_task;
  double value = Units::ToUserMass(polar.GetCrewMass());
  const double max_value = Units::ToUserMass(300);
  if (!PickList(_("Crew"), value, 0, max_value, 5,
                [](double v, StaticString<64> &text) {
                  FormatUserMass(Units::ToSysMass(v), text.data(), true);
                }))
    return;

  ActionInterface::SetCrewMass(Units::ToSysMass(value));
  Refresh();
}

void
FlightSetup::EditPlane()
{
  HandleInternalLink("xcsoar://config/planes");
  Refresh();
}

void
FlightSetup::EditBallast()
{
  const auto &polar =
    CommonInterface::GetComputerSettings().polar.glide_polar_task;
  double value = polar.GetBallastLitres();
  if (!PickList(_("Ballast"), value, 0, BallastUiMax(), 5,
                [](double v, StaticString<64> &text) {
                  text.Format("%.0f l", v);
                }))
    return;

  ActionInterface::SetBallastLitres(value);
  Refresh();
}

void
FlightSetup::EditBugs()
{
  const auto &polar = CommonInterface::GetComputerSettings().polar;
  double value = (1 - polar.bugs) * 100;
  if (!PickList(_("Bugs"), value, 0, 50, 1,
                [](double v, StaticString<64> &text) {
                  text.Format("%.0f %%", v);
                }))
    return;

  ActionInterface::SetBugs(1 - (value / 100));
  Refresh();
}

void
FlightSetup::EditQNH()
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  double value = Units::ToUserPressure(settings.pressure);
  const double min_value =
    Units::ToUserPressure(Units::ToSysUnit(850, Unit::HECTOPASCAL));
  const double max_value =
    Units::ToUserPressure(Units::ToSysUnit(1300, Unit::HECTOPASCAL));

  if (!PickList(_("QNH"), value, min_value, max_value,
                GetUserPressureStep(),
                [](double v, StaticString<64> &text) {
                  FormatUserPressure(Units::FromUserPressure(v),
                                     text.data(), true);
                }))
    return;

  ActionInterface::SetQNH(Units::FromUserPressure(value), true);
  Refresh();
}

void
FlightSetup::EditForecast()
{
  double value = forecast.ToUser();
  const double min_value = Temperature::FromCelsius(-50).ToUser();
  const double max_value = Temperature::FromCelsius(60).ToUser();
  if (!PickList(_("Max. temp."), value, min_value, max_value, 1,
                [](double v, StaticString<64> &text) {
                  FormatUserTemperature(Temperature::FromUser(v).ToKelvin(),
                                        text.data(), true);
                }))
    return;

  forecast = Temperature::FromUser(value);
  forecast_edited = true;
  Refresh();
}

} // namespace

void
dlgBasicSettingsShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  StaticString<128> caption(_("Flight Setup"));
  caption.append(" - ");
  caption.append(plane.polar_name);

  auto *list = new GroupedListWidget(look);
  FlightSetup setup;
  setup.SetList(*list);

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, caption, list);
  setup.SetDialog(dialog);
  setup.SetDumpButton(dialog.AddButton(_("Dump"), [&setup]{
    setup.FlipBallastTimer();
  }));
  dialog.AddButton(_("Close"), mrOK);

  setup.SetButtonPanel(dialog.GetButtonPanel());
  setup.SetButtons();
  dialog.PrepareFloatingList();
  setup.Refresh();

  setup.Start();
  const int result = dialog.ShowModal();
  setup.Stop();

  if (result == mrOK)
    setup.CommitForecast();
}
