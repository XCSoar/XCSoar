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

#ifndef XCSOAR_GLOBAL_FONTS_HPP
#define XCSOAR_GLOBAL_FONTS_HPP

class Font;

/**
 * Container for global font variables.  Avoid using it if you can,
 * use the "Look" objects instead.
 */
namespace Fonts
{
  extern Font infobox;
  extern Font infobox_small;
#ifndef GNAV
  extern Font infobox_units;
#endif

  extern Font title;
  extern Font cdi;
  extern Font monospace;

  extern Font map;
  extern Font map_bold;
  extern Font map_label;
  extern Font map_label_important;

  void Deinitialize();
}

#endif
