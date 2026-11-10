// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MainWindow.hpp"
#include "Monitor/TrafficMonitor.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

/** Not a GaugeLocation; used only in this panel's Off/position enum. */
static constexpr unsigned TRAFFIC_RADAR_OFF = ~0u;

static constexpr StaticEnumChoice traffic_radar_list[] = {
  { TRAFFIC_RADAR_OFF, N_("Off"),
    N_("Disable the traffic radar.") },
  { TrafficSettings::GaugeLocation::AUTO,
    N_("Auto (follow InfoBoxes)") },
  { TrafficSettings::GaugeLocation::TOP_LEFT,
    N_("Top left") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT,
    N_("Top right") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT,
    N_("Bottom left") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT,
    N_("Bottom right") },
  { TrafficSettings::GaugeLocation::CENTER_TOP,
    N_("Center top") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM,
    N_("Center bottom") },
  { TrafficSettings::GaugeLocation::TOP_LEFT_AVOID_IB,
    N_("Top left (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::TOP_RIGHT_AVOID_IB,
    N_("Top right (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::CENTER_TOP_AVOID_IB,
    N_("Center top (avoid InfoBoxes)") },
  { TrafficSettings::GaugeLocation::CENTER_BOTTOM_AVOID_IB,
    N_("Center bottom (avoid InfoBoxes)") },
  nullptr
};

/** The FLARM radar and the spoken collision alarms. */
class TrafficConfigPanel final : public ConfigListPanel {
  unsigned radar;
  bool auto_close, no_position, spoken;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
TrafficConfigPanel::LoadSettings() noexcept
{
  const auto &traffic = CommonInterface::GetUISettings().traffic;

  radar = traffic.enable_gauge
    ? unsigned(traffic.gauge_location)
    : TRAFFIC_RADAR_OFF;
  auto_close = traffic.auto_close_dialog;
  no_position = traffic.no_position_target_distance_ring;
  spoken = traffic.spoken_alerts;
}

void
TrafficConfigPanel::Fill() noexcept
{
  AddGroup();

  AddEnumItem(_("FLARM Radar"),
              _("Enable the traffic radar and choose its position on the "
                "main screen. The track bearing of the target relative to "
                "the track bearing of the aircraft is displayed as an arrow "
                "head, and a triangle pointing up or down shows the relative "
                "altitude of the target relative to you. In all modes, the "
                "color of the target indicates the threat level."),
              traffic_radar_list, radar);

  if (IsExpert())
    AddToggleItem(_("Auto close FLARM"),
                  _("Setting this to \"On\" will automatically close the "
                    "FLARM dialog if there is no traffic. \"Off\" will keep "
                    "the dialog open even without current traffic."),
                  auto_close);

  AddToggleItem(_("No position target"),
                _("This parameter enables or disables the No Position "
                  "Target Distance Ring in Flarm Radar"),
                no_position, nullptr, radar == TRAFFIC_RADAR_OFF);

  AddToggleItem(_("Spoken alerts"),
                _("Speak the collision alarms of FLARM, with the urgency, "
                  "the clock position and whether the traffic is above or "
                  "below: \"Traffic urgent! Three o'clock. Above.\" "
                  "Obstacle and alert zone alarms are not spoken."),
                spoken);

  AddButton(_("Test spoken alert"), [](){ PlaySpokenAlertExample(); });
}

bool
TrafficConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &traffic = CommonInterface::SetUISettings().traffic;
  const bool enable = radar != TRAFFIC_RADAR_OFF;

  changed |= Profile::Update(ProfileKeys::EnableFLARMGauge,
                             traffic.enable_gauge, enable);

  if (enable) {
    const auto location =
      static_cast<TrafficSettings::GaugeLocation>(radar);
    if (Profile::Update(ProfileKeys::FlarmLocation,
                        traffic.gauge_location, location)) {
      CommonInterface::main_window->ReinitialiseLayout();
      changed = true;
    }
  }

  changed |= Profile::Update(ProfileKeys::AutoCloseFlarmDialog,
                             traffic.auto_close_dialog, auto_close);
  changed |= Profile::Update(ProfileKeys::NoPositionTargetDistanceRing,
                             traffic.no_position_target_distance_ring,
                             no_position);
  changed |= Profile::Update(ProfileKeys::FlarmSpokenAlerts,
                             traffic.spoken_alerts, spoken);

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateTrafficConfigPanel()
{
  return std::make_unique<TrafficConfigPanel>();
}
