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

#ifndef XCSOAR_TRAFFIC_DATABASES_HPP
#define XCSOAR_TRAFFIC_DATABASES_HPP

#include "ColorDatabase.hpp"
#include "NameDatabase.hpp"
#include "FlarmNetDatabase.hpp"

struct TrafficDatabases {
  FlarmColorDatabase flarm_colors;

  FlarmNameDatabase flarm_names;

  FlarmNetDatabase flarm_net;

  /**
   * FlarmId of the glider to track.  Check FlarmId::IsDefined()
   * before using this attribute.
   *
   * This is a copy of TeamCodeSettings::team_flarm_id.
   */
  FlarmId team_flarm_id;

  TrafficDatabases()
    :team_flarm_id(FlarmId::Undefined()) {}

  gcc_pure
  FlarmColor GetColor(FlarmId id) const
  {
    FlarmColor color = flarm_colors.Get(id);
    if (color == FlarmColor::NONE && id == team_flarm_id)
      /* if no color found but target is teammate, use green */
      color = FlarmColor::GREEN;

    return color;
  }
};

#endif
