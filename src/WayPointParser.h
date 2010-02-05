/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef WAYPOINTPARSER_HPP
#define WAYPOINTPARSER_HPP

#include "Engine/Math/fixed.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

#include <tchar.h>

#define wpTerrainBoundsYes    100
#define wpTerrainBoundsYesAll 101
#define wpTerrainBoundsNo     102
#define wpTerrainBoundsNoAll  103

class Waypoints;
class MapWindowProjection;
class RasterTerrain;

struct Waypoint;
struct GEOPOINT;
struct SETTINGS_COMPUTER;

int dlgWaypointOutOfTerrain(const TCHAR *Message);

void SetHome(const Waypoints &way_points, const RasterTerrain *terrain,
    SETTINGS_COMPUTER &settings, const bool reset,
    const bool set_location = false);

enum WayPointFileType {
  ftWinPilot
};

/**
 * This class is used to parse different waypoint files
 */
class WayPointParser {
public:
  WayPointParser();

  int WaypointOutOfTerrainRangeDialogResult;
  static int WaypointsOutOfRangeSetting;

  /**
   * Reads the waypoints out of the two waypoint files and appends them to the
   * specified waypoint list
   * @param way_points The waypoint list to fill
   * @param terrain RasterTerrain (for automatic waypoint height)
   */
  static bool ReadWaypoints(Waypoints &way_points,
                            const RasterTerrain *terrain = NULL);
  static void SaveWaypoints(Waypoints &way_points);
  static void CloseWaypoints(Waypoints &way_points);

  /**
   * Sets the filename to parse or save in
   * @param filename The filename (will be converted to filepath later)
   * @param returnOnFileMissing If true the function returns false if the file
   * does not exist, If false it just continues
   * @return True if filename was set and file extension is known to XCSoar
   */
  bool SetFile(TCHAR* filename, bool returnOnFileMissing = true, int filenum = 0);
  /**
   * Clears the filename attribute
   */
  void ClearFile();

  /**
   * Parses the waypoint file provided by SetFile() into the given waypoint list
   * @param way_points The waypoint list to fill
   * @param terrain RasterTerrain (for automatic waypoint height)
   * @return True if the waypoint file parsing was okay, False otherwise
   */
  bool Parse(Waypoints &way_points, const RasterTerrain *terrain);
  /**
   * Calls SetFile() and Parse() afterwards
   */
  bool ParseFromFile(Waypoints &way_points, const RasterTerrain *terrain,
                     TCHAR* filename, int filenum = 0) {
    return SetFile(filename, true, filenum) && Parse(way_points, terrain);
  }

  bool Save(Waypoints &way_points);
  /**
   * Calls SetFile() and Save() afterwards
   */
  bool SaveToFile(Waypoints &way_points, TCHAR* filename, int filenum = 0) {
    return SetFile(filename, false, filenum) && Save(way_points);
  }

  bool IsWritable() {
    switch (filetype) {
    case ftWinPilot:
      return true;
    default:
      return false;
    }
  }

  static double AltitudeFromTerrain(GEOPOINT &location,
                                    const RasterTerrain &terrain);

private:
  TCHAR file[255];
  int filenum;
  WayPointFileType filetype;
  bool compressed;

  /**
   * Redirects the parsing of a file line to the
   * appropriate parser for the filetype
   * @param line The line to parse
   * @param linenum The line number in the file
   * @param way_points The waypoint list to fill
   * @param terrain RasterTerrain (for automatic waypoint height)
   * @return True if the line was parsed correctly or ignored, False if
   * parsing error occured
   */
  bool parseLine(const TCHAR* line, unsigned linenum,
                 Waypoints &way_points, const RasterTerrain *terrain);

  // WinPilot/Cambridge parsers
  /**
   * Parses a WinPilot waypoint file line
   * @see parseLine()
   */
  bool parseLineWinPilot(const TCHAR* line, const unsigned linenum,
                         Waypoints &way_points, const RasterTerrain *terrain);
  bool parseStringWinPilot(const TCHAR* src, tstring& dest);
  bool parseAngleWinPilot(const TCHAR* src, fixed& dest, const bool lat);
  bool parseAltitudeWinPilot(const TCHAR* src, fixed& dest);
  bool parseFlagsWinPilot(const TCHAR* src, WaypointFlags& dest);

  // WinPilot/Cambridge composers
  void saveFileWinPilot(FILE *fp, const Waypoints &way_points);
  tstring composeLineWinPilot(const Waypoint& wp);
  tstring composeAngleWinPilot(const fixed& src, const bool lat);
  tstring composeAltitudeWinPilot(const fixed& src);
  tstring composeFlagsWinPilot(const WaypointFlags& src);

  // Helper functions
  void setDefaultFlags(WaypointFlags& dest, bool turnpoint = true);

  bool checkWaypointInTerrainRange(const Waypoint &way_point,
                                          const RasterTerrain &terrain);

  static size_t extractParameters(const TCHAR *src, TCHAR *dst,
                                  const TCHAR **arr, size_t sz);

};

#endif
