/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "SettingsMap.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Dialogs/DialogSettings.hpp"

enum StateMessageAlign_t {
  smAlignCenter = 0,
  smAlignTopLeft,
};

/**
 * User interface settings.
 */
struct UISettings {
  bool custom_fonts;

  bool enable_auto_blank;

  /** Show FLARM radar if traffic present */
  bool enable_flarm_gauge;

  /** Automatically close the FLARM dialog when no traffic present */
  bool auto_close_flarm_dialog;

  /** Show ThermalAssistant if circling */
  bool enable_thermal_assistant_gauge;

  StateMessageAlign_t popup_message_position;

  SETTINGS_MAP map;
  InfoBoxSettings info_boxes;
  DialogSettings dialog;

  void SetDefaults();
};

#endif
