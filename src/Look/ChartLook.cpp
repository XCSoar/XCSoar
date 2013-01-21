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

#include "ChartLook.hpp"
#include "Asset.hpp"
#include "StandardFonts.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

void
ChartLook::Initialise()
{
  pens[STYLE_BLUETHIN].Set(Pen::DASH, IsAltair() ? 1 : 2, Color(0, 50, 255));
  pens[STYLE_REDTHICK].Set(Pen::DASH, 3, Color(200, 50, 50));
  pens[STYLE_DASHGREEN].Set(Pen::DASH, 2, COLOR_GREEN);
  pens[STYLE_MEDIUMBLACK].Set(IsAltair() ? 1 : 2, Color(50, 243, 45));
  pens[STYLE_THINDASHPAPER].Set(Pen::DASH, 1, Color(0xB0, 0xB0, 0xB0));

  bar_brush.Set(COLOR_GREEN);

  PixelScalar axis_label_size = std::max(PixelScalar(8), Layout::FastScale(6));
  PixelScalar axis_value_size = std::max(PixelScalar(8), Layout::FastScale(7));

  label_font.Load(GetStandardFontFace(), Layout::FastScale(12));
  axis_label_font.Load(GetStandardFontFace(), axis_label_size, true);
  axis_value_font.Load(GetStandardFontFace(), axis_value_size);
}
