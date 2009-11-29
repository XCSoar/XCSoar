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

#ifndef XCSOAR_AIRSPACE_DATABASE_HPP
#define XCSOAR_AIRSPACE_DATABASE_HPP

#include "Airspace.h"

#ifndef NDEBUG
#include <stdio.h> /* for Dump(FILE) */
#endif

struct GEOPOINT;
struct AIRSPACE_AREA;
struct AIRSPACE_CIRCLE;
struct SETTINGS_COMPUTER;
class MapWindowProjection;
class RasterTerrain;

class AirspaceDatabase {
public:
  AIRSPACE_POINT *AirspacePoint;
  unsigned int NumberOfAirspacePoints;

  AIRSPACE_AREA *AirspaceArea;
  unsigned int NumberOfAirspaceAreas;

  AIRSPACE_CIRCLE *AirspaceCircle;
  unsigned int NumberOfAirspaceCircles;

protected:
  /**
   * The QNH all flight level calculations are based on.
   */
  double qnh;

public:
  AirspaceDatabase();
  ~AirspaceDatabase() { Clear(); }

  void Clear();

  bool GrowPoints(unsigned n);
  bool GrowAreas(unsigned n);
  bool GrowCircles(unsigned n);

  AIRSPACE_POINT *AppendPoint() {
    return &AirspacePoint[NumberOfAirspacePoints++];
  }

  AIRSPACE_AREA *AppendArea() {
    return &AirspaceArea[NumberOfAirspaceAreas++];
  }

  AIRSPACE_CIRCLE *AppendCircle() {
    return &AirspaceCircle[NumberOfAirspaceCircles++];
  }

  void Sort();

  void SetQNH(double _qnh);

  bool Valid() const {
    return NumberOfAirspacePoints > 0 || NumberOfAirspaceAreas > 0 ||
      NumberOfAirspaceCircles > 0;
  }

  /**
   * Returns distance between location and AirspaceCircle border
   * @param location Location used for calculation
   * @param i Array id of the AirspaceCircle
   * @return Distance between location and airspace border
   */
  double CircleDistance(const GEOPOINT &location, const unsigned i) const;

  /**
   * Checks whether the given location is inside
   * of a certain AirspaceCircle defined by i
   * @param location Location to be checked
   * @param i Array id of the AirspaceCircle
   * @return True if location is in AirspaceCircle, False otherwise
   */
  bool InsideCircle(const GEOPOINT &location, const unsigned i) const;

  /**
   * Finds the nearest AirspaceCircle
   * @param location Location where to check
   * @param settings Pointer to the settings object
   * @param nearestdistance Pointer in which the distance
   * to the nearest circle will be written in
   * @param nearestbearing Pointer in which the bearing
   * to the nearest circle will be written in
   * @param height If != NULL only airspaces are considered
   * where the height is between base and top of the airspace
   * @return Array id of the nearest AirspaceCircle
   */
  int NearestCircle(const GEOPOINT &location,
                    double altitude, double terrain_altitude,
                    const SETTINGS_COMPUTER &settings,
                    double *nearestdistance,
                    double *nearestbearing,
                    double *height=NULL) const;

  /**
   * Checks whether a given location is inside the
   * AirspaceArea defined by i
   * @param location Location to be checked
   * @param i Array id of the AirspaceArea
   * @return True if location is inside the AirspaceArea, False otherwise
   */
  bool InsideArea(const GEOPOINT &location, const unsigned i) const;

  /**
   * Calculates the distance to the border of the
   * AirspaceArea defined by i
   * @param location Location used for calculation
   * @param i Array id of the AirspaceArea
   * @param bearing Bearing to the closest point of the
   * airspace (Pointer)
   * @param map_projection MapWindowProjection object
   * @return Distance to the closest point of the airspace
   */
  double RangeArea(const GEOPOINT &location, const int i, double *bearing,
                   const MapWindowProjection &map_projection) const;

  /**
   * Finds the nearest AirspaceArea
   * @param location Location where to check
   * @param settings Pointer to the settings object
   * @param map_projection MapWindowProjection object
   * @param nearestdistance Pointer in which the distance
   * to the nearest area will be written in
   * @param nearestbearing Pointer in which the bearing
   * to the nearest area will be written in
   * @param height If != NULL only airspaces are considered
   * where the height is between base and top of the airspace
   * @return Array id of the nearest AirspaceArea
   */
  int NearestArea(const GEOPOINT &location,
                  double altitude, double terrain_altitude,
                  const SETTINGS_COMPUTER &settings,
                  const MapWindowProjection& map_projection,
                  double *nearestdistance, double *nearestbearing,
                  double *height=NULL) const;

  void ScanLine(const GEOPOINT *locs, const double *heights,
                int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X]) const;

  /**
   * Recalculates the MSL values of all AGL specification based on
   * terrain data.
   *
   * @param terrain the terrain data (must be locked)
   */
  void UpdateAGL(const RasterTerrain &terrain);

#ifndef NDEBUG
  void Dump(FILE *file) const;
#endif
};

#endif
