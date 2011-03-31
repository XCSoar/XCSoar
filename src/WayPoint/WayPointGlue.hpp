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

#ifndef XCSOAR_WAY_POINT_GLUE_HPP
#define XCSOAR_WAY_POINT_GLUE_HPP

class Waypoints;
class RasterTerrain;
struct SETTINGS_COMPUTER;

class WayPointFile;

/**
 * This class is used to parse different waypoint files
 */
namespace WayPointGlue {
  void SetHome(Waypoints &way_points, const RasterTerrain *terrain,
               SETTINGS_COMPUTER &settings, const bool reset);

  /**
   * Reads the waypoints out of the two waypoint files and appends them to the
   * specified waypoint list
   * @param way_points The waypoint list to fill
   * @param terrain RasterTerrain (for automatic waypoint height)
   */
  bool ReadWaypoints(Waypoints &way_points,
                     const RasterTerrain *terrain);
  bool SaveWaypoints(const Waypoints &way_points);

  /**
   * Close all waypoint files.
   */
  void Close();
};

#endif
