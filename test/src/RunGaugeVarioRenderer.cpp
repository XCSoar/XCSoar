// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "Asset.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Blackboard/InterfaceBlackboard.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/SwitchState.hpp"
#include "Look/VarioLook.hpp"
#include "Look/VarioBarLook.hpp"
#include "Look/InfoBoxLook.hpp"
#include "Look/Colors.hpp"
#include "Screen/Layout.hpp"
#include "Form/Button.hpp"
#include "Units/Units.hpp"
#include "time/Stamp.hpp"

#include <algorithm>
#include <cmath>

namespace {

/** Simple cross-country profile for the vario harness. */
struct FlightSimulator {
  double time_s = 0;

  /** Smoothed values updated each tick. */
  double netto_average = 0;
  double thermal_average = 0;
  double last_thermal_smooth = 1.6;

  /** Manual MC value when #ComputerSettings::task::auto_mc is false. */
  double user_mc = 1.5;

  static constexpr double DT = 0.1;
  static constexpr double CRUISE_DURATION = 38;
  static constexpr double ENTER_DURATION = 4;
  static constexpr double THERMAL_DURATION = 42;
  static constexpr double LEAVE_DURATION = 4;
  static constexpr double CYCLE = CRUISE_DURATION + ENTER_DURATION +
    THERMAL_DURATION + LEAVE_DURATION;

  [[gnu::const]]
  static double SmoothStep(double t) noexcept
  {
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
  }

  [[gnu::pure]]
  static double Turbulence(double time_s) noexcept
  {
    return 0.12 * std::sin(time_s * 4.7)
      + 0.07 * std::sin(time_s * 11.3 + 1.1);
  }

  void Advance() noexcept
  {
    time_s += DT;

    const double phase = std::fmod(time_s, CYCLE);
    const unsigned thermal_index = unsigned(time_s / CYCLE);
    const double thermal_lift = 1.4 + 0.9 * std::sin(thermal_index * 1.7 + 0.4);

    enum class Mode {
      CRUISE,
      ENTER,
      THERMAL,
      LEAVE,
    };

    Mode mode = Mode::CRUISE;
    double mode_blend = 0;

    if (phase < CRUISE_DURATION) {
      mode = Mode::CRUISE;
    } else if (phase < CRUISE_DURATION + ENTER_DURATION) {
      mode = Mode::ENTER;
      mode_blend = SmoothStep((phase - CRUISE_DURATION) / ENTER_DURATION);
    } else if (phase < CRUISE_DURATION + ENTER_DURATION + THERMAL_DURATION) {
      mode = Mode::THERMAL;
    } else {
      mode = Mode::LEAVE;
      mode_blend = SmoothStep((phase - CRUISE_DURATION - ENTER_DURATION -
                               THERMAL_DURATION) / LEAVE_DURATION);
    }

    const bool circling = mode == Mode::THERMAL ||
      (mode == Mode::ENTER && mode_blend > 0.35) ||
      (mode == Mode::LEAVE && mode_blend < 0.65);

    static constexpr double cruise_sink = -0.85;
    static constexpr double turn_sink = 0.95;
    const double cruise_ias = 27.5;
    const double thermal_ias = 22.0;

    double netto = cruise_sink;
    double brutto = cruise_sink;
    double ias = cruise_ias;
    double mc = 1.2 + 0.8 * std::sin(time_s * 0.03);
    double v_stf = mc * 0.6;

    switch (mode) {
    case Mode::CRUISE:
      netto = brutto = cruise_sink + Turbulence(time_s);
      break;

    case Mode::ENTER:
      netto = cruise_sink + (thermal_lift - cruise_sink) * mode_blend
        + Turbulence(time_s);
      brutto = netto - turn_sink * mode_blend;
      ias = cruise_ias + (thermal_ias - cruise_ias) * mode_blend;
      v_stf *= 1.0 - mode_blend;
      break;

    case Mode::THERMAL:
      netto = thermal_lift + Turbulence(time_s) * 0.5;
      brutto = netto - turn_sink;
      ias = thermal_ias;
      mc = 0.8;
      v_stf = -1.2;
      break;

    case Mode::LEAVE:
      netto = thermal_lift + (cruise_sink - thermal_lift) * mode_blend
        + Turbulence(time_s);
      brutto = netto - turn_sink * (1.0 - mode_blend);
      ias = thermal_ias + (cruise_ias - thermal_ias) * mode_blend;
      v_stf = -1.2 + (mc * 0.6 + 1.2) * mode_blend;
      break;
    }

    constexpr double avg_alpha = 0.06;
    if (circling) {
      thermal_average += (netto - thermal_average) * avg_alpha;
      netto_average = thermal_average;
    } else {
      netto_average += (netto - netto_average) * avg_alpha;
      last_thermal_smooth += (thermal_average - last_thermal_smooth) * 0.008;
      thermal_average += (0 - thermal_average) * 0.02;
    }

    const TimeStamp now{FloatDuration{time_s}};

    MoreData basic{};
    DerivedInfo calculated{};

    basic.brutto_vario = brutto;
    basic.brutto_vario_available.Update(now);
    basic.indicated_airspeed = ias;
    basic.airspeed_available.Update(now);
    basic.total_energy_vario = brutto;
    basic.total_energy_vario_available.Update(now);

    calculated.average = thermal_average;
    calculated.netto_average = netto_average;
    calculated.last_thermal_average_smooth = last_thermal_smooth;
    calculated.current_thermal.lift_rate = circling ? netto : 0;
    calculated.flight.flying = true;
    calculated.circling = circling;
    basic.switch_state.flight_mode = circling
      ? SwitchState::FlightMode::CIRCLING
      : SwitchState::FlightMode::CRUISE;
    calculated.V_stf = v_stf;

    ComputerSettings &computer = blackboard.SetComputerSettings();
    if (computer.task.auto_mc)
      computer.polar.glide_polar_task.SetMC(mc);
    else
      computer.polar.glide_polar_task.SetMC(user_mc);
    computer.polar.glide_polar_task.SetBallastFraction(0.18 +
      0.12 * std::sin(time_s * 0.015));
    computer.polar.bugs = 0.82 + 0.08 * std::cos(time_s * 0.008);

    blackboard.ReadBlackboardBasic(basic);
    blackboard.ReadBlackboardCalculated(calculated);
  }

  InterfaceBlackboard &blackboard;

  explicit FlightSimulator(InterfaceBlackboard &_blackboard) noexcept
    :blackboard(_blackboard) {}

  void AdjustMC(double delta) noexcept
  {
    ComputerSettings &computer = blackboard.SetComputerSettings();
    computer.task.auto_mc = false;
    user_mc = std::clamp(user_mc + delta, 0.0, 5.0);
    computer.polar.glide_polar_task.SetMC(user_mc);
  }

  void SyncUserMcFromPolar() noexcept
  {
    user_mc = blackboard.GetComputerSettings().polar.glide_polar_task.GetMC();
  }
};

} // namespace

static void
InitBlackboard(InterfaceBlackboard &blackboard) noexcept
{
  ComputerSettings &computer = blackboard.SetComputerSettings();
  computer.polar.glide_polar_task = GlidePolar(2.5);
  computer.polar.glide_polar_task.SetBallastFraction(0.35);
  computer.polar.bugs = 0.85;
  computer.polar.glide_polar_task.SetMC(1.5);
  computer.task.auto_mc = true;

  UISettings &ui = blackboard.SetUISettings();
  ui.info_boxes.geometry = InfoBoxSettings::Geometry::RIGHT_9_VARIO;
  ui.info_boxes.scale_title_font = 100;
  ui.info_boxes.use_colors = true;
  ui.info_boxes.theme = InfoBoxSettings::Theme::DARK;

  VarioSettings &vario = ui.vario;
  vario.show_average = true;
  vario.show_mc = true;
  vario.show_speed_to_fly = true;
  vario.show_ballast = true;
  vario.show_bugs = true;
  vario.show_gross = true;
  vario.show_average_needle = true;
  vario.show_thermal_average_needle = true;
}

[[gnu::pure]]
static InfoBoxSettings::Geometry
SelectVarioGeometry(PixelSize screen_size) noexcept
{
  if (screen_size.width > screen_size.height)
    return InfoBoxSettings::Geometry::RIGHT_9_VARIO;

  return InfoBoxSettings::Geometry::BOTTOM_8_VARIO;
}

/** Match #Look::InitialiseConfigured vario path for the harness blackboard. */
static void
InitialiseVarioLook(VarioLook &vario_look, VarioBarLook &vario_bar_look,
                    InfoBoxLook &info_box_look,
                    const UISettings &ui, unsigned infobox_width) noexcept
{
  const bool dark = ui.info_boxes.theme == InfoBoxSettings::Theme::DARK;

  info_box_look.Initialise(dark, ui.info_boxes.use_colors, infobox_width,
                           ui.info_boxes.scale_title_font);
  vario_look.Initialise(dark, ui.info_boxes.use_colors, infobox_width,
                        info_box_look.title_font);
  vario_bar_look.Initialise(vario_look.label_font);
}

static void
ReinitialiseVarioLook(VarioLook &vario_look, InfoBoxLook &info_box_look,
                      const UISettings &ui, unsigned infobox_width) noexcept
{
  info_box_look.ReinitialiseLayout(infobox_width, ui.info_boxes.scale_title_font);
  vario_look.ReinitialiseLayout(infobox_width, ui.info_boxes.scale_title_font,
                                std::max(1u, infobox_width / 2u), 32,
                                Layout::Scale(8));
}

class VarioHarnessWindow final : public ContainerWindow {
  InterfaceBlackboard &blackboard;
  FlightSimulator *flight = nullptr;
  InfoBoxLook info_box_look;
  VarioLook vario_look;
  VarioBarLook vario_bar_look;
  GaugeVario *gauge = nullptr;
  InfoBoxLayout::Layout layout{};
  Button mc_down_button;
  Button mc_up_button;
  Button mc_mode_button;
  Button speed_mode_button;
  Button theme_button;
  bool buttons_created = false;

  [[gnu::pure]]
  PixelRect GetVarioArea(PixelRect client) const noexcept
  {
    client.bottom -= int(Layout::GetMaximumControlHeight());
    return client;
  }

  void LayoutButtons(PixelRect client) noexcept
  {
    if (!buttons_created)
      return;

    const int bar_top = client.bottom - int(Layout::GetMaximumControlHeight());
    const unsigned button_w = Layout::Scale(56);
    int x = client.left;

    mc_down_button.Move({x, bar_top, x + int(button_w), client.bottom});
    x += int(button_w);
    mc_up_button.Move({x, bar_top, x + int(button_w), client.bottom});
    x += int(button_w);
    mc_mode_button.Move({x, bar_top, x + int(button_w), client.bottom});
    x += int(button_w);
    speed_mode_button.Move({x, bar_top, x + int(button_w), client.bottom});
    x += int(button_w);
    theme_button.Move({x, bar_top, x + int(button_w) * 2, client.bottom});
  }

  void UpdateControlCaptions() noexcept
  {
    if (!buttons_created)
      return;

    UpdateThemeCaption();

    if (mc_mode_button.IsDefined()) {
      const bool auto_mc = blackboard.GetComputerSettings().task.auto_mc;
      mc_mode_button.SetCaption(auto_mc ? "MC manual" : "MC auto");
    }

    if (speed_mode_button.IsDefined()) {
      const bool stf = blackboard.GetUISettings().vario.show_speed_to_fly;
      speed_mode_button.SetCaption(stf ? "Vario" : "SC");
    }
  }

  void ApplyTheme() noexcept
  {
    const UISettings &ui = blackboard.GetUISettings();
    const unsigned width = layout.control_size.width > 0
      ? layout.control_size.width
      : std::max(1u, unsigned(GetVarioArea(GetClientRect()).GetWidth()));
    InitialiseVarioLook(vario_look, vario_bar_look, info_box_look, ui, width);
    UpdateControlCaptions();
    if (gauge != nullptr)
      gauge->Invalidate();
    Invalidate();
  }

  void OnMCAdjust(double delta) noexcept
  {
    if (flight != nullptr)
      flight->AdjustMC(delta);
    UpdateControlCaptions();
    InvalidateGauge();
  }

  void OnToggleMcMode() noexcept
  {
    ComputerSettings &computer = blackboard.SetComputerSettings();
    computer.task.auto_mc = !computer.task.auto_mc;
    if (!computer.task.auto_mc && flight != nullptr)
      flight->SyncUserMcFromPolar();
    UpdateControlCaptions();
    InvalidateGauge();
  }

  void OnToggleSpeedMode() noexcept
  {
    VarioSettings &vario = blackboard.SetUISettings().vario;
    vario.show_speed_to_fly = !vario.show_speed_to_fly;
    UpdateControlCaptions();
    InvalidateGauge();
  }

  void OnToggleTheme() noexcept
  {
    UISettings &ui = blackboard.SetUISettings();
    ui.info_boxes.theme =
      ui.info_boxes.theme == InfoBoxSettings::Theme::DARK
      ? InfoBoxSettings::Theme::LIGHT
      : InfoBoxSettings::Theme::DARK;
    ApplyTheme();
  }

  void CreateButtons(PixelRect client) noexcept
  {
    if (buttons_created)
      return;

    const int bar_top = client.bottom - int(Layout::GetMaximumControlHeight());
    const unsigned button_w = Layout::Scale(56);
    WindowStyle style;
    int x = client.left;

    mc_down_button.Create(*this, *button_look, "MC -",
                          {x, bar_top, x + int(button_w), client.bottom},
                          style, [this]{ OnMCAdjust(-0.2); });
    x += int(button_w);
    mc_up_button.Create(*this, *button_look, "MC +",
                        {x, bar_top, x + int(button_w), client.bottom},
                        style, [this]{ OnMCAdjust(+0.2); });
    x += int(button_w);
    mc_mode_button.Create(*this, *button_look, "MC auto",
                          {x, bar_top, x + int(button_w), client.bottom},
                          style, [this]{ OnToggleMcMode(); });
    x += int(button_w);
    speed_mode_button.Create(*this, *button_look, "Vario",
                             {x, bar_top, x + int(button_w), client.bottom},
                             style, [this]{ OnToggleSpeedMode(); });
    x += int(button_w);
    theme_button.Create(*this, *button_look, "Theme",
                        {x, bar_top, x + int(button_w) * 2, client.bottom},
                        style, [this]{ OnToggleTheme(); });
    buttons_created = true;
    UpdateControlCaptions();
  }

  void UpdateThemeCaption() noexcept
  {
    if (!theme_button.IsDefined())
      return;

    const bool dark =
      blackboard.GetUISettings().info_boxes.theme ==
      InfoBoxSettings::Theme::DARK;
    theme_button.SetCaption(dark ? "Light theme" : "Dark theme");
  }

  void Relayout() noexcept
  {
    const PixelRect client = GetClientRect();
    const UISettings &ui = blackboard.GetUISettings();
    layout = InfoBoxLayout::Calculate(GetVarioArea(client),
                                      SelectVarioGeometry(client.GetSize()));
    assert(layout.HasVario());

    if (gauge == nullptr) {
      InitialiseVarioLook(vario_look, vario_bar_look, info_box_look, ui,
                          layout.control_size.width);
      gauge = new GaugeVario(blackboard, *this, vario_look, vario_bar_look,
                             layout.vario);
      gauge->SetDebugOverlay(true);
      gauge->Show();
    } else {
      ReinitialiseVarioLook(vario_look, info_box_look, ui,
                            layout.control_size.width);
      gauge->Move(layout.vario);
    }

    LayoutButtons(client);
    gauge->Invalidate();
  }

public:
  explicit VarioHarnessWindow(InterfaceBlackboard &_blackboard) noexcept
    :blackboard(_blackboard) {}

  ~VarioHarnessWindow() {
    delete gauge;
  }

  void SetFlightSimulator(FlightSimulator &_flight) noexcept
  {
    flight = &_flight;
  }

  void InvalidateGauge() noexcept {
    if (gauge != nullptr)
      gauge->Invalidate();
  }

protected:
  void OnCreate() override {
    ContainerWindow::OnCreate();
    const PixelRect client = GetClientRect();
    CreateButtons(client);
    Relayout();
  }

  void OnResize(PixelSize new_size) noexcept override {
    ContainerWindow::OnResize(new_size);
    Relayout();
  }

  void OnPaint(Canvas &canvas) noexcept override {
    const bool dark =
      blackboard.GetUISettings().info_boxes.theme ==
      InfoBoxSettings::Theme::DARK;
    canvas.Clear(dark ? COLOR_DARK_THEME_BACKGROUND : COLOR_WHITE);

    canvas.Select(Pen(Layout::ScaleFinePenWidth(1), COLOR_GRAY));
    canvas.SelectHollowBrush();
    for (unsigned i = 0; i < layout.count; ++i) {
      const PixelRect &rc = layout.positions[i];
      if (rc.GetWidth() > 0 && rc.GetHeight() > 0)
        canvas.DrawRectangle(rc);
    }

    ContainerWindow::OnPaint(canvas);
  }
};

static void
Main(TestMainWindow &main_window)
{
  InterfaceBlackboard blackboard;
  InitBlackboard(blackboard);

  VarioHarnessWindow window(blackboard);
  window.Create(main_window, main_window.GetClientRect());
  main_window.SetFullWindow(window);

  FlightSimulator flight(blackboard);
  window.SetFlightSimulator(flight);
  flight.Advance();
  window.InvalidateGauge();

  UI::PeriodicTimer timer([&](){
    flight.Advance();
    window.InvalidateGauge();
  });
  timer.Schedule(std::chrono::milliseconds(100));

  main_window.RunEventLoop();
}
