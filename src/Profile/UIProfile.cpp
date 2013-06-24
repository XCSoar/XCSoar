/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Profile/UIProfile.hpp"
#include "Profile/MapProfile.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Profile/PageProfile.hpp"
#include "Profile/Profile.hpp"
#include "Profile/UnitsConfig.hpp"
#include "UISettings.hpp"

namespace Profile {
  static void Load(DisplaySettings &settings);
  static void Load(VarioSettings &settings);
  static void Load(TrafficSettings &settings);
  static void Load(DialogSettings &settings);
  static void Load(SoundSettings &settings);
  static void Load(VarioSoundSettings &settings);
};

void
Profile::Load(DisplaySettings &settings)
{
  Get(ProfileKeys::AutoBlank, settings.enable_auto_blank);
  GetEnum(ProfileKeys::DisplayOrientation, settings.orientation);
}

void
Profile::Load(VarioSettings &settings)
{
  Get(ProfileKeys::AppGaugeVarioSpeedToFly, settings.show_speed_to_fly);
  Get(ProfileKeys::AppGaugeVarioAvgText, settings.show_average);
  Get(ProfileKeys::AppGaugeVarioMc, settings.show_mc);
  Get(ProfileKeys::AppGaugeVarioBugs, settings.show_bugs);
  Get(ProfileKeys::AppGaugeVarioBallast, settings.show_ballast);
  Get(ProfileKeys::AppGaugeVarioGross, settings.show_gross);
  Get(ProfileKeys::AppAveNeedle, settings.show_average_needle);
}

void
Profile::Load(TrafficSettings &settings)
{
  Get(ProfileKeys::EnableFLARMGauge, settings.enable_gauge);
  Get(ProfileKeys::AutoCloseFlarmDialog, settings.auto_close_dialog);
  Get(ProfileKeys::FlarmAutoZoom, settings.auto_zoom);
  Get(ProfileKeys::FlarmNorthUp, settings.north_up);
  GetEnum(ProfileKeys::FlarmLocation, settings.gauge_location);
}

void
Profile::Load(DialogSettings &settings)
{
  GetEnum(ProfileKeys::AppDialogTabStyle, settings.tab_style);
  Get(ProfileKeys::UserLevel, settings.expert);
}

void
Profile::Load(VarioSoundSettings &settings)
{
  Get(ProfileKeys::SoundAudioVario, settings.enabled);
  Get(ProfileKeys::SoundVolume, settings.volume);
  Get(ProfileKeys::VarioDeadBandEnabled, settings.dead_band_enabled);

  Get(ProfileKeys::VarioMinFrequency, settings.min_frequency);
  Get(ProfileKeys::VarioZeroFrequency, settings.zero_frequency);
  Get(ProfileKeys::VarioMaxFrequency, settings.max_frequency);

  Get(ProfileKeys::VarioMinPeriod, settings.min_period_ms);
  Get(ProfileKeys::VarioMaxPeriod, settings.max_period_ms);

  Get(ProfileKeys::VarioDeadBandMin, settings.min_dead);
  Get(ProfileKeys::VarioDeadBandMax, settings.max_dead);
}

void
Profile::Load(SoundSettings &settings)
{
  Get(ProfileKeys::SoundTask, settings.sound_task_enabled);
  Get(ProfileKeys::SoundModes, settings.sound_modes_enabled);
  Get(ProfileKeys::SoundDeadband, settings.sound_deadband);

  Load(settings.vario);
}

void
Profile::Load(UISettings &settings)
{
  Load(settings.display);

  Get(ProfileKeys::MenuTimeout, settings.menu_timeout);

  Get(ProfileKeys::UseCustomFonts, settings.custom_fonts);

  Get(ProfileKeys::EnableTAGauge, settings.enable_thermal_assistant_gauge);

  GetEnum(ProfileKeys::AppStatusMessageAlignment, settings.popup_message_position);

  GetEnum(ProfileKeys::HapticFeedback, settings.haptic_feedback);

  GetEnum(ProfileKeys::LatLonUnits, settings.coordinate_format);

  LoadUnits(settings.units);
  Load(settings.map);
  Load(settings.info_boxes);
  Load(settings.vario);
  Load(settings.traffic);
  Load(settings.pages);
  Load(settings.dialog);
  Load(settings.sound);
}
