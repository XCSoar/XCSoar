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

#include "InfoBoxSettings.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <stdio.h>

void
InfoBoxSettings::Panel::Clear()
{
  name.clear();
  std::fill(contents, contents + MAX_CONTENTS, 0);
}

bool
InfoBoxSettings::Panel::IsEmpty() const
{
  for (unsigned i = 0; i < MAX_CONTENTS; ++i)
    if (contents[i] != 0)
      return false;

  return true;
}

void
InfoBoxSettings::SetDefaults()
{
  inverse = false;
  use_colors = true;
  border_style = apIbBox;

  for (unsigned i = 0; i < MAX_PANELS; ++i)
    panels[i].Clear();

  static gcc_constexpr_data unsigned DFLT_CONFIG_BOXES = 9;
  static gcc_constexpr_data unsigned DFLT_CONFIG_PANELS = 4;
  static gcc_constexpr_data int contents[DFLT_CONFIG_PANELS][DFLT_CONFIG_BOXES] = {
    { 0x0E, 0x0B, 0x16, 0x31, 0x30, 0x21, 0x07, 0x0F, 0x2D },
    { 0x0E, 0x0B, 0x03, 0x2B, 0x30, 0x21, 0x11, 0x0F, 0x2D },
    { 0x0E, 0x12, 0x03, 0x2B, 0x26, 0x21, 0x29, 0x0F, 0x2D },
    { 0x34, 0x33, 0x31, 0x00, 0x06, 0x19, 0x27, 0x25, 0x1A }
  };

  assert(MAX_PANELS >= DFLT_CONFIG_PANELS);
  assert(Panel::MAX_CONTENTS >= DFLT_CONFIG_BOXES);

  panels[0].name = N_("Circling");
  panels[1].name = N_("Cruise");
  panels[2].name = N_("FinalGlide");

  for (unsigned i = PREASSIGNED_PANELS; i < MAX_PANELS; i++)
    _stprintf(panels[i].name.buffer(), _T("AUX-%u"), i-2);

  for (unsigned i = 0; i < DFLT_CONFIG_PANELS; i++)
    for (unsigned j = 0; j < DFLT_CONFIG_BOXES; j++)
      panels[i].contents[j] = contents[i][j];
}
