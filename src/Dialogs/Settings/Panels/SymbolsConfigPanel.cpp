// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolsConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

static constexpr StaticEnumChoice ground_track_mode_list[] = {
  { DisplayGroundTrack::OFF, N_("Off"), N_("Disable display of ground track line.") },
  { DisplayGroundTrack::ON, N_("On"), N_("Always display ground track line.") },
  { DisplayGroundTrack::AUTO, NC_("Setting", "Auto"), N_("Display ground track line if there is a significant difference to plane heading.") },
  nullptr
};

static constexpr StaticEnumChoice trail_length_list[] = {
  { TrailSettings::Length::OFF, N_("Off") },
  { TrailSettings::Length::LONG, N_("Long") },
  { TrailSettings::Length::SHORT, N_("Short") },
  { TrailSettings::Length::FULL, N_("Full") },
  nullptr
};

static constexpr StaticEnumChoice trail_type_list[] = {
  { TrailSettings::Type::VARIO_1, N_("Vario #1"), N_("Within lift areas "
    "lines get displayed green and thicker, while sinking lines are shown brown and thin. "
    "Zero lift is presented as a grey line.") },
  { TrailSettings::Type::VARIO_1_DOTS, N_("Vario #1 (with dots)"), N_("The same "
    "colour scheme as the previous, but with dotted lines while sinking.") },
  { TrailSettings::Type::VARIO_2, N_("Vario #2"), N_("The climb colour "
    "for this scheme is orange to red, sinking is displayed as light blue to dark blue. "
    "Zero lift is presented as a yellow line.") },
  { TrailSettings::Type::VARIO_2_DOTS, N_("Vario #2 (with dots)"), N_("The same "
    "colour scheme as the previous, but with dotted lines while sinking.") },
  { TrailSettings::Type::VARIO_DOTS_AND_LINES,
    N_("Vario-scaled dots and lines"),
    N_("Vario-scaled dots with lines. "
       "Orange to red = climb. Light blue to dark blue = sink. "
       "Zero lift is presented as a yellow line.") },
  { TrailSettings::Type::VARIO_EINK, N_("Vario E-ink"), N_("E-ink friendly color scheme, lighter and thicker dots means lift while darker and thinner means sink.") },
  { TrailSettings::Type::ALTITUDE, N_("Altitude"), N_("The colour scheme corresponds to the height.") },
  nullptr
};

static constexpr StaticEnumChoice  aircraft_symbol_list[] = {
  { AircraftSymbol::SIMPLE, N_("Simple"),
    N_("Simplified line graphics, black with white contours.") },
  { AircraftSymbol::SIMPLE_LARGE, N_("Simple (large)"),
    N_("Enlarged simple graphics.") },
  { AircraftSymbol::DETAILED, N_("Detailed"),
    N_("Detailed rendered aircraft graphics.") },
  { AircraftSymbol::HANGGLIDER, N_("HangGlider"),
    N_("Simplified hang glider as line graphics, white with black contours.") },
  { AircraftSymbol::PARAGLIDER, N_("Paraglider"),
    N_("Simplified para glider as line graphics, white with black contours.") },
  nullptr
};

static constexpr StaticEnumChoice wind_arrow_list[] = {
  { WindArrowStyle::NO_ARROW, N_("Off"), N_("No wind arrow is drawn.") },
  { WindArrowStyle::ARROW_HEAD, N_("Arrow head"), N_("Draws an arrow head only.") },
  { WindArrowStyle::FULL_ARROW, N_("Full arrow"), N_("Draws an arrow head with a dashed arrow line.") },
  nullptr
};

static constexpr StaticEnumChoice online_traffic_map_mode_list[] = {
  { DisplayOnlineTrafficMapMode::OFF, N_("Off"), N_("No online traffic is drawn.") },
  { DisplayOnlineTrafficMapMode::SYMBOL, N_("Symbol"), N_("Draws the traffic symbol only.") },
  { DisplayOnlineTrafficMapMode::SYMBOL_NAME, N_("Symbol and Name"), N_("Draws the traffic symbol with name.") },
  nullptr
};

/**
 * The symbols the map draws: the aircraft, the traffic and the
 * trail.
 */
class SymbolsConfigPanel final : public ConfigListPanel {
  DisplayGroundTrack display_ground_track;
  bool show_flarm_on_map, fade_traffic;
  TrailSettings trail;
  bool detour_cost_markers_enabled;
  AircraftSymbol aircraft_symbol;
  WindArrowStyle wind_arrow_style;
  DisplayOnlineTrafficMapMode online_traffic_map_mode;
  bool distance_rings_enabled;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
SymbolsConfigPanel::LoadSettings() noexcept
{
  const MapSettings &settings_map = CommonInterface::GetMapSettings();

  display_ground_track = settings_map.display_ground_track;
  show_flarm_on_map = settings_map.show_flarm_on_map;
  fade_traffic = settings_map.fade_traffic;
  trail = settings_map.trail;
  detour_cost_markers_enabled = settings_map.detour_cost_markers_enabled;
  aircraft_symbol = settings_map.aircraft_symbol;
  wind_arrow_style = settings_map.wind_arrow_style;
  online_traffic_map_mode = settings_map.online_traffic_map_mode;
  distance_rings_enabled = settings_map.distance_rings_enabled;
}

void
SymbolsConfigPanel::Fill() noexcept
{
  AddGroup();

  if (IsExpert())
    AddEnumItem(_("Aircraft symbol"), nullptr, aircraft_symbol_list,
                aircraft_symbol);

  AddEnumItem(_("Ground track"),
              _("Display the ground track as a grey line on the map."),
              ground_track_mode_list, display_ground_track);

  if (IsExpert())
    AddEnumItem(_("Wind arrow"),
                _("Determines the way the wind arrow is drawn on the map."),
                wind_arrow_list, wind_arrow_style);

  AddToggleItem(C_("Setting", "Distance rings"),
                _("Display distance rings around the aircraft on the map."),
                distance_rings_enabled);

  if (IsExpert())
    AddToggleItem(_("Detour cost markers"),
                  _("If the aircraft heading deviates from the current waypoint, markers are displayed "
                    "at points ahead of the aircraft. The value of each marker is the extra distance "
                    "required to reach that point as a percentage of straight-line distance to the waypoint."),
                  detour_cost_markers_enabled);

  AddGroup(_("Traffic"));

  AddToggleItem(_("FLARM Traffic"),
                _("This enables the display of FLARM traffic on the map window."),
                show_flarm_on_map);

  AddToggleItem(_("Fade traffic"),
                _("Keep showing traffic for a while after it has disappeared."),
                fade_traffic);

  AddEnumItem(C_("Setting", "Online traffic on map"),
              _("Show traffic from SkyLines and XCSoar Cloud on the map."),
              online_traffic_map_mode_list, online_traffic_map_mode);

  if (IsExpert()) {
    AddGroup(_("Trail"));

    AddEnumItem(_("Trail length"),
                _("Determines whether and how long a snail trail is drawn behind the glider."),
                trail_length_list, trail.length);

    /* the rest of the trail is only drawn while there is one */
    if (trail.length != TrailSettings::Length::OFF) {
      AddToggleItem(_("Trail drift"),
                    _("Determines whether the snail trail is drifted with the wind "
                      "when displayed in circling mode at near map scales. Switched "
                      "Off, the snail trail stays uncompensated for wind drift."),
                    trail.wind_drift_enabled);

      AddEnumItem(_("Trail type"),
                  _("Sets the type of the snail trail display."),
                  trail_type_list, trail.type);

      AddToggleItem(_("Trail scaled"),
                    _("If set to ON the snail trail width is scaled according to the vario signal."),
                    trail.scaling_enabled);
    }
  }
}

bool
SymbolsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapSettings &settings_map = CommonInterface::SetMapSettings();

  changed |= Profile::Update(ProfileKeys::DisplayTrackBearing,
                             settings_map.display_ground_track,
                             display_ground_track);

  changed |= Profile::Update(ProfileKeys::EnableFLARMMap,
                             settings_map.show_flarm_on_map,
                             show_flarm_on_map);

  changed |= Profile::Update(ProfileKeys::FadeTraffic,
                             settings_map.fade_traffic, fade_traffic);

  changed |= Profile::Update(ProfileKeys::SnailTrail,
                             settings_map.trail.length, trail.length);

  changed |= Profile::Update(ProfileKeys::TrailDrift,
                             settings_map.trail.wind_drift_enabled,
                             trail.wind_drift_enabled);

  changed |= Profile::Update(ProfileKeys::SnailType,
                             settings_map.trail.type, trail.type);

  changed |= Profile::Update(ProfileKeys::SnailWidthScale,
                             settings_map.trail.scaling_enabled,
                             trail.scaling_enabled);

  changed |= Profile::Update(ProfileKeys::DetourCostMarker,
                             settings_map.detour_cost_markers_enabled,
                             detour_cost_markers_enabled);

  changed |= Profile::Update(ProfileKeys::AircraftSymbol,
                             settings_map.aircraft_symbol, aircraft_symbol);

  changed |= Profile::Update(ProfileKeys::WindArrowStyle,
                             settings_map.wind_arrow_style,
                             wind_arrow_style);

  changed |= Profile::Update(ProfileKeys::OnlineTrafficMapMode,
                             settings_map.online_traffic_map_mode,
                             online_traffic_map_mode);

  changed |= Profile::Update(ProfileKeys::DistanceRingsEnabled,
                             settings_map.distance_rings_enabled,
                             distance_rings_enabled);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSymbolsConfigPanel()
{
  return std::make_unique<SymbolsConfigPanel>();
}
