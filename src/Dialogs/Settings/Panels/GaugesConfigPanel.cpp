// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GaugesConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MainWindow.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

static constexpr StaticEnumChoice final_glide_bar_display_mode_list[] = {
  { FinalGlideBarDisplayMode::OFF, N_("Off"),
    N_("Disable final glide bar.") },
  { FinalGlideBarDisplayMode::ON, N_("On"),
    N_("Always show final glide bar.") },
  { FinalGlideBarDisplayMode::AUTO, NC_("Setting", "Auto"),
    N_("Show final glide bar if approaching final glide range.") },
  nullptr
};

static constexpr StaticEnumChoice flarm_display_location_list[] = {
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

static constexpr StaticEnumChoice thermal_assistant_position_list[] = {
  { UISettings::ThermalAssistantPosition::OFF,
    N_("Off"),
    N_("Disable thermal assistant.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT,
    N_("Bottom left"),
    N_("Show thermal assistant in bottom left.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB,
    N_("Bottom left (avoid InfoBoxes)"),
    N_("Show thermal assistant in bottom left, above or to the right of InfoBoxes (if present).") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT,
    N_("Bottom right"),
    N_("Show thermal assistant in bottom right.") },
  { UISettings::ThermalAssistantPosition::BOTTOM_RIGHT_AVOID_IB,
    N_("Bottom right (avoid InfoBoxes)"),
    N_("Show thermal assistant in bottom right, above or to the left of InfoBoxes (if present).") },
  { UISettings::ThermalAssistantPosition::TOP_LEFT,
    N_("Top left"),
    N_("Show thermal assistant in top left.") },
  { UISettings::ThermalAssistantPosition::TOP_RIGHT,
    N_("Top right"),
    N_("Show thermal assistant in top right.") },
  { UISettings::ThermalAssistantPosition::CENTER_TOP,
    N_("Center top"),
    N_("Show thermal assistant in center top.") },
  { UISettings::ThermalAssistantPosition::TOP_LEFT_AVOID_IB,
    N_("Top left (avoid InfoBoxes)"),
    N_("Show thermal assistant in top left (avoid InfoBoxes).") },
  { UISettings::ThermalAssistantPosition::TOP_RIGHT_AVOID_IB,
    N_("Top right (avoid InfoBoxes)"),
    N_("Show thermal assistant in top right (avoid InfoBoxes).") },
  { UISettings::ThermalAssistantPosition::CENTER_TOP_AVOID_IB,
    N_("Center top (avoid InfoBoxes)"),
    N_("Show thermal assistant in center top (avoid InfoBoxes).") },
  nullptr
};

/**
 * The gauges which lie over the map: the FLARM radar, the thermal
 * assistant and the bars at its edges.
 */
class GaugesConfigPanel final : public ConfigListPanel {
  bool flarm_gauge, auto_close_flarm, no_position_target;
  TrafficSettings::GaugeLocation flarm_location;

  UISettings::ThermalAssistantPosition thermal_assistant_position;
  bool thermal_profile;

  FinalGlideBarDisplayMode final_glide_bar_display_mode;
  bool final_glide_bar_mc0, vario_bar;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
GaugesConfigPanel::LoadSettings() noexcept
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  flarm_gauge = ui_settings.traffic.enable_gauge;
  auto_close_flarm = ui_settings.traffic.auto_close_dialog;
  flarm_location = ui_settings.traffic.gauge_location;
  no_position_target = ui_settings.traffic.no_position_target_distance_ring;

  thermal_assistant_position = ui_settings.thermal_assistant_position;
  thermal_profile = map_settings.show_thermal_profile;

  final_glide_bar_display_mode = map_settings.final_glide_bar_display_mode;
  final_glide_bar_mc0 = map_settings.final_glide_bar_mc0_enabled;
  vario_bar = map_settings.vario_bar_enabled;
}

void
GaugesConfigPanel::Fill() noexcept
{
  AddGroup(_("Traffic"));

  AddToggleItem(_("FLARM Radar"),
                _("This enables the display of the FLARM radar gauge. The track bearing of the target relative to the track bearing of the aircraft is displayed as an arrow head, and a triangle pointing up or down shows the relative altitude of the target relative to you. In all modes, the color of the target indicates the threat level."),
                flarm_gauge);

  if (IsExpert()) {
    AddToggleItem(_("Auto close FLARM"),
                  _("Setting this to \"On\" will automatically close the FLARM dialog if there is no traffic. \"Off\" will keep the dialog open even without current traffic."),
                  auto_close_flarm);

    AddEnumItem(_("FLARM display"),
                _("Choose a location for the FLARM display."),
                flarm_display_location_list, flarm_location);
  }

  AddToggleItem(_("No position target"),
                _("This parameter enables or disables the No Position Target Distance Ring in Flarm Radar"),
                no_position_target);

  AddGroup(_("Thermal"));

  AddEnumItem(_("Thermal Assistant"),
              _("Enable and select the position of the thermal assistant when overlayed on the main screen."),
              thermal_assistant_position_list, thermal_assistant_position);

  AddToggleItem(_("Thermal Band"),
                _("This enables the display of the thermal profile (climb band) display on the map."),
                thermal_profile);

  if (!IsExpert())
    return;

  AddGroup();

  AddEnumItem(_("Final glide bar"),
              _("If set to \"On\" the final glide will always be shown, if set to \"Auto\" it will be shown when approaching the final glide possibility."),
              final_glide_bar_display_mode_list,
              final_glide_bar_display_mode);

  if (final_glide_bar_display_mode != FinalGlideBarDisplayMode::OFF)
    AddToggleItem(_("Final glide bar MC0"),
                  _("If set to \"On\" the final glide bar will show a second arrow indicating the required height "
                    "to reach the final waypoint at MC zero."),
                  final_glide_bar_mc0);

  AddToggleItem(_("Vario bar"),
                _("If set to \"On\" the vario bar will be shown."),
                vario_bar);
}

bool
GaugesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();
  TrafficSettings &traffic = ui_settings.traffic;
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  changed |= Profile::Update(ProfileKeys::EnableFLARMGauge,
                             traffic.enable_gauge, flarm_gauge);

  changed |= Profile::Update(ProfileKeys::AutoCloseFlarmDialog,
                             traffic.auto_close_dialog, auto_close_flarm);

  /* the gauges move: the main window lays itself out again */
  const bool thermal_assistant_moved =
    Profile::Update(ProfileKeys::TAPosition,
                    ui_settings.thermal_assistant_position,
                    thermal_assistant_position);
  const bool flarm_moved =
    Profile::Update(ProfileKeys::FlarmLocation,
                    traffic.gauge_location, flarm_location);
  if (thermal_assistant_moved || flarm_moved)
    CommonInterface::main_window->ReinitialiseLayout();

  changed |= Profile::Update(ProfileKeys::EnableThermalProfile,
                             map_settings.show_thermal_profile,
                             thermal_profile);

  changed |= Profile::Update(ProfileKeys::FinalGlideBarDisplayMode,
                             map_settings.final_glide_bar_display_mode,
                             final_glide_bar_display_mode);

  changed |= Profile::Update(ProfileKeys::EnableFinalGlideBarMC0,
                             map_settings.final_glide_bar_mc0_enabled,
                             final_glide_bar_mc0);

  changed |= Profile::Update(ProfileKeys::EnableVarioBar,
                             map_settings.vario_bar_enabled, vario_bar);

  changed |= Profile::Update(ProfileKeys::NoPositionTargetDistanceRing,
                             traffic.no_position_target_distance_ring,
                             no_position_target);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateGaugesConfigPanel()
{
  return std::make_unique<GaugesConfigPanel>();
}
