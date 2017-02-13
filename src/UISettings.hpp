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

#ifndef XCSOAR_UI_SETTINGS_HPP
#define XCSOAR_UI_SETTINGS_HPP

#include "FormatSettings.hpp"
#include "MapSettings.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Gauge/VarioSettings.hpp"
#include "Gauge/TrafficSettings.hpp"
#include "PageSettings.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "DisplaySettings.hpp"
#include "Audio/Settings.hpp"

#include <type_traits>

#include <stdint.h>

/**
 * User interface settings.
 */
struct UISettings {
  DisplaySettings display;

  /** timeout in quarter seconds of menu button */
  unsigned menu_timeout;

  unsigned scale;

  /** Override OS dpi settings */
  unsigned custom_dpi;

  /** Show ThermalAssistant if circling */
  bool enable_thermal_assistant_gauge;

  /** Enable warning dialog */
  bool enable_airspace_warning_dialog;

  /** Show Menubutton */
  bool show_menu_button;

  enum class PopupMessagePosition : uint8_t {
    CENTER,
    TOP_LEFT,
  } popup_message_position;

  /** Haptic feedback settings */
  enum class HapticFeedback : uint8_t {
    DEFAULT,
    OFF,
    ON,
  } haptic_feedback;

  FormatSettings format;
  MapSettings map;
  InfoBoxSettings info_boxes;
  VarioSettings vario;
  TrafficSettings traffic;
  PageSettings pages;
  DialogSettings dialog;
  SoundSettings sound;

  void SetDefaults();

  unsigned GetPercentScale() const {
    return scale;
  }
};

static_assert(std::is_trivial<UISettings>::value, "type is not trivial");

#endif
