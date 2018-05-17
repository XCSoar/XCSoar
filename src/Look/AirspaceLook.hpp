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

#ifndef XCSOAR_AIRSPACE_LOOK_HPP
#define XCSOAR_AIRSPACE_LOOK_HPP

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Features.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"

#define NUMAIRSPACECOLORS 16
#define NUMAIRSPACEBRUSHES 8

struct AirspaceRendererSettings;
struct AirspaceClassRendererSettings;
class Font;

struct AirspaceClassLook {
  Color fill_color;

#if defined(HAVE_ALPHA_BLEND) || !defined(HAVE_HATCHED_BRUSH)
  /**
   * Non-pattern brushes used for transparent
   */
  Brush solid_brush;
#endif

  Pen border_pen;

  void Initialise(const AirspaceClassRendererSettings &settings);
};

struct AirspaceLook {
  static const RGB8Color preset_colors[NUMAIRSPACECOLORS];

#ifdef HAVE_HATCHED_BRUSH
  Bitmap bitmaps[NUMAIRSPACEBRUSHES];
  Brush brushes[NUMAIRSPACEBRUSHES];
#endif

  AirspaceClassLook classes[AIRSPACECLASSCOUNT];

  Pen thick_pen;

  MaskedIcon intercept_icon;

  /**
   * look for labels
   */
  Pen label_pen;
  Brush label_brush;
  Color label_text_color;

  /**
   * The font used to render the airspace name.
   */
  const Font *name_font;

  void Initialise(const AirspaceRendererSettings &settings,
                  const Font &_name_font);

  void Reinitialise(const AirspaceRendererSettings &settings);
};

#endif
