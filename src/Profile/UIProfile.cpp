/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "UIProfile.hpp"
#include "ProfileKeys.hpp"
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
  static void Load(const ProfileMap &map, TrafficSettings &settings);
  static void Load(const ProfileMap &map, DialogSettings &settings);
  static void Load(const ProfileMap &map, SoundSettings &settings);
  static void Load(const ProfileMap &map, VarioSoundSettings &settings);
};

void
Profile::Load(const ProfileMap &map, DisplaySettings &settings)
{
  map.GetEnum(ProfileKeys::MapOrientation, settings.orientation);
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
}

void
Profile::Load(const ProfileMap &map, TrafficSettings &settings)
{
  map.Get(ProfileKeys::EnableFLARMGauge, settings.enable_gauge);
  map.Get(ProfileKeys::AutoCloseFlarmDialog, settings.auto_close_dialog);
  map.Get(ProfileKeys::FlarmAutoZoom, settings.auto_zoom);
  map.Get(ProfileKeys::FlarmNorthUp, settings.north_up);
  map.GetEnum(ProfileKeys::FlarmLocation, settings.gauge_location);
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

  map.Get(ProfileKeys::EnableTAGauge, settings.enable_thermal_assistant_gauge);

  map.Get(ProfileKeys::AirspaceWarningDialog, settings.enable_airspace_warning_dialog);

  map.GetEnum(ProfileKeys::AppStatusMessageAlignment, settings.popup_message_position);

  map.GetEnum(ProfileKeys::HapticFeedback, settings.haptic_feedback);

  map.Get(ProfileKeys::ShowMenuButton, settings.show_menu_button);

  Load(map, settings.format);
  Load(map, settings.map);
  Load(map, settings.info_boxes);
  Load(map, settings.vario);
  Load(map, settings.traffic);
  Load(map, settings.pages);
  Load(map, settings.dialog);
  Load(map, settings.sound);
}
