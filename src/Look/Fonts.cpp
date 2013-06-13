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

#include "Fonts.hpp"
#include "Screen/Font.hpp"

/// values inside infoboxes  like numbers, etc.
Font Fonts::infobox;
Font Fonts::infobox_small;
#ifndef GNAV
Font Fonts::infobox_units;
#endif
/// Titles of infoboxes like Next, WP L/D etc.
Font Fonts::title;
/// vario display, runway informations
Font Fonts::cdi;
Font Fonts::monospace;
/// text names on the map
Font Fonts::map;
/// menu buttons, waypoint selection, messages, etc.
Font Fonts::map_bold;
/// Flarm Traffic draweing and stats, map labels in italic
Font Fonts::map_label;
/// font labels for important labels (e.g. big/medium cities)
Font Fonts::map_label_important;

void
Fonts::Deinitialize()
{
  infobox.Destroy();
  infobox_small.Destroy();
#ifndef GNAV
  infobox_units.Destroy();
#endif
  title.Destroy();
  map.Destroy();
  map_bold.Destroy();
  cdi.Destroy();
  map_label.Destroy();
  map_label_important.Destroy();
  monospace.Destroy();
}
