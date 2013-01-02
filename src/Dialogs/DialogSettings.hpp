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

#ifndef XCSOAR_DIALOG_SETTINGS_HPP
#define XCSOAR_DIALOG_SETTINGS_HPP

#include <stdint.h>

/**
 * Dialog display styles
 */
enum DialogStyle
{
  /** cover screen, stretch controls horizontally */
  dsFullWidth = 0,
  /** stretch only frame to maintain aspect ratio */
  dsScaled,
  /** like eDialogScaled but center dialog in screen */
  dsScaledCentered,
  /** don't adjust at all (same as !Layout::ScaleSupported()) */
  dsFixed,
  /** stretch horizontal and place to bottom */
  dsScaledBottom
};

struct DialogSettings {
  enum class TextInputStyle : uint8_t {
    /**
     * Use the platform default - i.e. keyboard if the device has a
     * pointing device.
     */
    Default,
    Keyboard,
    HighScore,
  };

  enum class TabStyle : uint8_t {
    Text,
    Icon,
  };

  DialogStyle dialog_style;

  TextInputStyle text_input_style;
  TabStyle tab_style;

  /**
   * Show the "expert" settings?
   */
  bool expert;

  void SetDefaults();
};

#endif
