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
#include "Screen/Layout.hpp"
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
  using Callback = GroupedListWidget::Callback;
  using BadgeStyle = GroupedListWidget::BadgeStyle;

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
  void Refit() noexcept;
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

  void AddRow(const char *caption, const char *help, const char *text,
              bool hidden, Callback edit,
              const char *badge = nullptr,
              BadgeStyle badge_style = BadgeStyle::PRIMARY,
              const char *badge2 = nullptr,
              BadgeStyle badge_style2 = BadgeStyle::PRIMARY) noexcept;
};

static unsigned
PreferredClientWidth(const DialogLook &look) noexcept
{
  const unsigned loading =
    look.list.font->TextSize("Wing loading   000.0 kg/m2").width;
  const unsigned error = look.list.font->TextSize(
    "Wing loading   No empty mass or wing area (m2)").width;
  const unsigned text = std::max(loading, error);
  return text + 2 * Layout::VptScale(10) +
    2 * Layout::GetTextPadding();
}

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
FlightSetup::AddRow(const char *caption, const char *help,
                    const char *text, bool hidden,
                    Callback edit, const char *badge,
                    BadgeStyle badge_style,
                    const char *badge2,
                    BadgeStyle badge_style2) noexcept
{
  /* a tap on the label selects the row and draws an arrow.  A tap
     on the value opens it.  Without an action the row stays grey
     and the cursor may rest on it so its help can be read.  A badge
     on that row says why it cannot be used. */
  GroupedListWidget::ItemOptions options;
  options.help = help;
  options.hidden = hidden;
  options.value = text;
  options.badge_style = badge_style;
  options.badge2 = badge2;
  options.badge_style2 = badge_style2;
  options.chevron = static_cast<bool>(edit);
  options.label_selects = static_cast<bool>(edit);

  if (edit) {
    options.badge = badge;
    list->AddItem(caption, std::move(edit), options);
  } else {
    options.disabled = true;
    options.selectable_when_disabled = true;
    options.disabled_badge_label = badge;
    list->AddItem(caption, options);
  }
}

void
FlightSetup::Refresh() noexcept
{
  const Drawn next = Collect();

  list->Clear();

  list->AddGroup(nullptr);

  StaticString<128> text;

  FormatUserMass(next.crew_kg, text.data(), true);
  AddRow(_("Crew"),
         _("All masses loaded to the glider beyond the empty "
           "weight including pilot and copilot, but not water ballast."),
         text.c_str(), false, [this]{ EditCrew(); });

  const char *ballast_help =
    _("Ballast of the glider. Press \"Dump/Stop\" to toggle "
      "count-down of the ballast volume according to the dump "
      "rate specified in the configuration settings.");
  const char *ballast_value = nullptr;
  const char *ballast_badge = nullptr;
  const char *ballast_badge2 = nullptr;
  auto ballast_style = BadgeStyle::PRIMARY;
  auto ballast_style2 = BadgeStyle::PRIMARY;
  if (next.show_ballast && next.ballast_l > 0) {
    text.Format("%.0f l", next.ballast_l);
    ballast_value = text.c_str();
  }
  if (!next.show_ballast)
    ballast_badge = _("No ballast");
  else if (ballast_value == nullptr) {
    /* Empty is the right-hand badge.  No dump time sits to its left. */
    ballast_badge = _("Empty");
    if (next.no_dump_time) {
      ballast_badge2 = _("No dump time");
      ballast_style2 = BadgeStyle::WARNING;
    }
  } else if (next.no_dump_time) {
    ballast_badge = _("No dump time");
    ballast_style = BadgeStyle::WARNING;
  }

  Callback ballast_edit;
  if (next.show_ballast)
    ballast_edit = [this]{ EditBallast(); };
  AddRow(_("Ballast"), ballast_help, ballast_value, false,
         std::move(ballast_edit), ballast_badge, ballast_style,
         ballast_badge2, ballast_style2);

  /* wing loading is mass divided by wing area; a missing input is a
     badge, so the row stays on the list */
  const bool have_mass = next.empty_mass > 0;
  const bool have_area = next.wing_area > 0;
  const char *wing_help;
  const char *wing_badge = nullptr;
  if (have_mass && have_area) {
    FormatUserWingLoading(next.wing, text.data(), text.capacity(), true);
    wing_help = _("The current wing loading, calculated from the glider's "
                  "empty weight, crew weight, and ballast. "
                  "Select the row to open the plane profile.");
  } else {
    text.clear();
    if (!have_mass && !have_area)
      wing_badge = _("No empty mass or wing area (m²)");
    else if (!have_mass)
      wing_badge = _("No empty mass");
    else
      wing_badge = _("No wing area (m²)");
    wing_help = _("Wing loading needs the empty mass and the wing area (m²). "
                  "Without them the polar cannot be calculated. "
                  "Select the row to open the plane profile.");
  }
  const auto wing_style = wing_badge != nullptr
    ? BadgeStyle::DANGER
    : BadgeStyle::PRIMARY;
  AddRow(_("Wing loading"), wing_help,
         wing_badge != nullptr ? nullptr : text.c_str(), false,
         [this]{ EditPlane(); }, wing_badge, wing_style);

  const char *bugs_help =
    /* xgettext:no-c-format */
    _("How clean the glider is. Set to 0% for clean, larger "
      "numbers as the wings pick up bugs or get wet. 50% "
      "indicates the glider's sink rate is doubled.");
  const double bugs_percent = (1 - next.bugs) * 100;
  if (bugs_percent > 0.5) {
    text.Format("%.0f %%", bugs_percent);
    AddRow(_("Bugs"), bugs_help, text.c_str(), false,
           [this]{ EditBugs(); });
  } else
    AddRow(_("Bugs"), bugs_help, nullptr, false,
           [this]{ EditBugs(); }, _("Clean"));

  FormatUserPressure(AtmosphericPressure::HectoPascal(next.qnh),
                     text.data(), true);
  AddRow(_("QNH"),
         _("Area pressure for barometric altimeter calibration. "
           "This is set automatically if Vega is connected."),
         text.c_str(), false, [this]{ EditQNH(); });

  if (next.show_altitude)
    FormatUserAltitude(next.altitude, text.data(), true);
  else
    text.clear();
  AddRow(_("Altitude"), nullptr, text.c_str(), !next.show_altitude, {});

  FormatUserTemperature(next.temp_k, text.data(), true);
  AddRow(_("Max. temp."),
         _("Set to forecast ground temperature. Used by convection "
           "estimator (temperature trace page of Analysis dialog)."),
         text.c_str(), false, [this]{ EditForecast(); });

  drawn = next;
  have_drawn = true;

  SetButtons();
  list->UpdateLayout();
  Refit();
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
FlightSetup::Refit() noexcept
{
  if (dialog != nullptr)
    dialog->FitToList(dialog->GetParentClientRect(),
                      PreferredClientWidth(dialog->GetLook()));
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

  const PixelRect rc{Layout::Scale(PixelSize{220u, 220u})};
  WidgetDialog dialog(UIGlobals::GetMainWindow(), look, rc,
                           caption, list);
  setup.SetDialog(dialog);
  setup.SetDumpButton(dialog.AddButton(_("Dump"), [&setup]{
    setup.FlipBallastTimer();
  }));
  dialog.AddButton(_("Close"), mrOK);

  setup.SetButtonPanel(dialog.GetButtonPanel());
  setup.SetButtons();
  dialog.EnableCursorSelection();
  dialog.ResyncButtonPanelSelection();
  list->SetActionBar(dialog.GetButtonPanel());
  list->SetCursorCallback([&setup](int){
    setup.Refit();
  });

  dialog.PrepareWidget();
  setup.Refresh();

  setup.Start();
  const int result = dialog.ShowModal();
  setup.Stop();

  if (result == mrOK)
    setup.CommitForecast();
}
