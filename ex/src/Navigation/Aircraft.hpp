/* Copyright_License {

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
#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "GeoPoint.hpp"

/**
 * Structure containing basic aircraft data used for navigation.
 * Typically this will be updated only by the GPS devices or by
 * a simulator.
 *
 * Copies of the state are made in some TaskPoint types so this should be
 * kept fairly small.  If extra data is required to be saved by the GPS
 * device, then this structure itself should be contained within a larger one.
 *
 */
struct AIRCRAFT_STATE {
  GEOPOINT Location; /**< location of aircraft */
  double Time; /**< global time (seconds UTC) */
  double Speed; /**< track speed (m/s) */
  double Altitude; /**< GPS altitude AMSL (m) */
  double WindSpeed; /**< Wind speed (m/s); must be positive */
  double WindDirection; /**< Wind direction (degrees) */

  /** Accessor for the aircraft location
   */
  const GEOPOINT& get_location() const {
    return Location;
  };
};

#endif
