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
#ifndef GLIDESTATE_HPP
#define GLIDESTATE_HPP

#include "Navigation/Geometry/GeoVector.hpp"

struct AIRCRAFT_STATE;
struct GEOPOINT;

/**
 * Class used to define a glide/navigation task
 */
struct GlideState 
{

/** 
 * Dummy task constructor.  Typically used for synthetic glide
 * tasks.  Where there are real targets, the other constructors should
 * be used instead.
 * 
 * @param vector Specified vector for task
 * @param htarget Height of target (m above MSL)
 * @param aircraft Aircraft state
 * 
 * @return Initialised glide task
 */
  GlideState(const GeoVector &vector,
              const double htarget,
              const AIRCRAFT_STATE &aircraft);

  GeoVector Vector;             /**< Distance/bearing of task */
  double MinHeight;             /**< Height (m above MSL) of end */
  double WindDirection;         /**< Direction of wind (deg True) */
  double AltitudeDifference;    /**< Aircraft height less target height */

/** 
 * Calculate internal quantities to reduce computation time
 * by clients of this class
 * 
 * @param aircraft Aircraft state
 */
  void calc_speedups(const AIRCRAFT_STATE &aircraft);

/** 
 * Calculates average cross-country speed from effective 
 * cross-country speed (accounting for wind)
 * 
 * @param Veff Effective cruise speed (m/s)
 * 
 * @return Average cross-country speed (m/s)
 */
  double calc_ave_speed(const double Veff) const;

/** 
 * Calculate distance a circling aircraft will drift
 * in a given time
 * 
 * @param t_climb Time spent in climb (s)
 * 
 * @return Distance (m) of drift
 */
  double drifted_distance(const double t_climb) const;

  double EffectiveWindSpeed;    /**< (internal use) */
  double EffectiveWindAngle;    /**< (internal use) */
  double wsq_;                  /**< (internal use) */
  double dwcostheta_;           /**< (internal use) */
};

#endif
