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

#ifndef XCSOAR_INFO_BOX_SETTINGS_HPP
#define XCSOAR_INFO_BOX_SETTINGS_HPP

#include "Util/StaticString.hpp"
#include "Compiler.h"
#include "InfoBoxes/Content/Factory.hpp"

enum InfoBoxBorderAppearance_t {
  apIbBox = 0,
  apIbTab
};

struct InfoBoxSettings {
  struct Panel {
    static const unsigned MAX_CONTENTS = 24;

    StaticString<32u> name;
    InfoBoxFactory::t_InfoBox contents[MAX_CONTENTS];

    void Clear();

    gcc_pure
    bool IsEmpty() const;
  };

  static const unsigned int MAX_PANELS = 8;
  static const unsigned int PREASSIGNED_PANELS = 3;

  bool inverse, use_colors;

  InfoBoxBorderAppearance_t border_style;

  Panel panels[MAX_PANELS];

  void SetDefaults();
};

#endif
