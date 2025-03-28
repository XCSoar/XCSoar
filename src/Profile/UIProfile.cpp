// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIProfile.hpp"
#include "Keys.hpp"
#include "Map.hpp"
#include "MapProfile.hpp"
#include "InfoBoxConfig.hpp"
#include "PageProfile.hpp"
#include "UnitsConfig.hpp"
#include "UISettings.hpp"

namespace Profile {
  static void Load(const ProfileMap &map, DisplaySettings &settings);
  static void Load(const ProfileMap &map, FormatSettings &settings);
  static void Load(const ProfileMap &map, VarioSettings &settings);
  static void Load(const ProfileMap &map, NavigatorSettings &settings);
  static void Load(const ProfileMap &map, TrafficSettings &settings);
  static void Load(const ProfileMap &map, DialogSettings &settings);
  static void Load(const ProfileMap &map, SoundSettings &settings);
  static void Load(const ProfileMap &map, VarioSoundSettings &settings);
};

void
Profile::Load(const ProfileMap &map, DisplaySettings &settings)
{
  map.GetEnum(ProfileKeys::MapOrientation, settings.orientation);
  map.Get(ProfileKeys::CursorSize, settings.cursor_size);
  map.Get(ProfileKeys::CursorColorsInverted, settings.invert_cursor_colors);
  map.Get(ProfileKeys::FullScreen, settings.full_screen);
}

void
Profile::Load(const ProfileMap &map, FormatSettings &settings)
{
  map.GetEnum(ProfileKeys::LatLonUnits, settings.coordinate_format);
  LoadUnits(map, settings.units);
}

void
Profile::Load(const ProfileMap &map, VarioSettings &settings)
{
  map.Get(ProfileKeys::AppGaugeVarioSpeedToFly, settings.show_speed_to_fly);
  map.Get(ProfileKeys::AppGaugeVarioAvgText, settings.show_average);
  map.Get(ProfileKeys::AppGaugeVarioMc, settings.show_mc);
  map.Get(ProfileKeys::AppGaugeVarioBugs, settings.show_bugs);
  map.Get(ProfileKeys::AppGaugeVarioBallast, settings.show_ballast);
  map.Get(ProfileKeys::AppGaugeVarioGross, settings.show_gross);
  map.Get(ProfileKeys::AppAveNeedle, settings.show_average_needle);
  map.Get(ProfileKeys::AppAveThermalNeedle, settings.show_thermal_average_needle);
}

void
Profile::Load(const ProfileMap &map, NavigatorSettings &settings) 
{
  map.Get(ProfileKeys::NavigatorHeight, settings.navigator_height);
}

void
Profile::Load(const ProfileMap &map, TrafficSettings &settings)
{
  map.Get(ProfileKeys::EnableFLARMGauge, settings.enable_gauge);
  map.Get(ProfileKeys::AutoCloseFlarmDialog, settings.auto_close_dialog);
  map.Get(ProfileKeys::FlarmAutoZoom, settings.auto_zoom);
  map.Get(ProfileKeys::FlarmNorthUp, settings.north_up);
  map.GetEnum(ProfileKeys::FlarmLocation, settings.gauge_location);
  map.Get(ProfileKeys::FlarmRadarZoom, settings.radar_zoom);
}

void
Profile::Load(const ProfileMap &map, DialogSettings &settings)
{
  map.GetEnum(ProfileKeys::AppTextInputStyle, settings.text_input_style);
  map.GetEnum(ProfileKeys::AppDialogTabStyle, settings.tab_style);
  map.Get(ProfileKeys::UserLevel, settings.expert);
}

void
Profile::Load(const ProfileMap &map, VarioSoundSettings &settings)
{
  map.Get(ProfileKeys::SoundAudioVario, settings.enabled);
  map.Get(ProfileKeys::SoundVolume, settings.volume);
  map.Get(ProfileKeys::VarioDeadBandEnabled, settings.dead_band_enabled);

  map.Get(ProfileKeys::VarioMinFrequency, settings.min_frequency);
  map.Get(ProfileKeys::VarioZeroFrequency, settings.zero_frequency);
  map.Get(ProfileKeys::VarioMaxFrequency, settings.max_frequency);

  map.Get(ProfileKeys::VarioMinPeriod, settings.min_period_ms);
  map.Get(ProfileKeys::VarioMaxPeriod, settings.max_period_ms);

  map.Get(ProfileKeys::VarioDeadBandMin, settings.min_dead);
  map.Get(ProfileKeys::VarioDeadBandMax, settings.max_dead);
}

void
Profile::Load(const ProfileMap &map, SoundSettings &settings)
{
  map.Get(ProfileKeys::SoundTask, settings.sound_task_enabled);
  map.Get(ProfileKeys::SoundModes, settings.sound_modes_enabled);
  map.Get(ProfileKeys::SoundDeadband, settings.sound_deadband);

  map.Get(ProfileKeys::MasterAudioVolume, settings.master_volume);

  Load(map, settings.vario);
}

void
Profile::Load(const ProfileMap &map, UISettings &settings)
{
  Load(map, settings.display);

  map.Get(ProfileKeys::MenuTimeout, settings.menu_timeout);

  map.Get(ProfileKeys::UIScale, settings.scale);
  if (settings.scale < 50 || settings.scale > 200)
    settings.scale = 100;

  map.Get(ProfileKeys::CustomDPI, settings.custom_dpi);
  if (settings.custom_dpi < 120 || settings.custom_dpi > 520)
    settings.custom_dpi = 0;

  /* Migrate old data if TA enabled */
  if (!map.GetEnum(ProfileKeys::TAPosition, settings.thermal_assistant_position)) {
    bool enable_thermal_assistant_gauge_obsolete;
    if (map.Get(ProfileKeys::EnableTAGauge,
                enable_thermal_assistant_gauge_obsolete)) {
      settings.thermal_assistant_position =
        enable_thermal_assistant_gauge_obsolete
        ? UISettings::ThermalAssistantPosition::BOTTOM_LEFT
        : UISettings::ThermalAssistantPosition::OFF;
    }
  }
  map.Get(ProfileKeys::AirspaceWarningDialog, settings.enable_airspace_warning_dialog);

  map.GetEnum(ProfileKeys::AppStatusMessageAlignment, settings.popup_message_position);

  map.GetEnum(ProfileKeys::HapticFeedback, settings.haptic_feedback);

  map.Get(ProfileKeys::ShowMenuButton, settings.show_menu_button);
  map.Get(ProfileKeys::ShowZoomButton, settings.show_zoom_button);

  if (!map.GetEnum(ProfileKeys::DarkMode, settings.dark_mode)) {
    /* migrate the old AppInverseInfoBox setting */
    bool inverse;
    if (map.Get(ProfileKeys::AppInverseInfoBox, inverse))
      settings.dark_mode = inverse
        ? UISettings::DarkMode::ON
        : UISettings::DarkMode::OFF;
  }

  Load(map, settings.format);
  Load(map, settings.map);
  Load(map, settings.info_boxes);
  Load(map, settings.navigator);
  Load(map, settings.vario);
  Load(map, settings.traffic);
  Load(map, settings.pages);
  Load(map, settings.dialog);
  Load(map, settings.sound);
}
