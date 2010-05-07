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

#ifndef XCSOAR_SPEED_VECTOR_HPP
#define XCSOAR_SPEED_VECTOR_HPP

#include "Math/Angle.hpp"

/**
 * An object describing the speed vector in a two dimensional surface.
 */
struct SpeedVector {
  /**
   * The direction of the vector in degrees true (0..360).
   */
  Angle bearing;

  /**
   * The norm of the vector [m/s].
   */
  fixed norm;

  /** 
   * Constructor for null speed
   * 
   * @return Initialised object
   */
  SpeedVector():bearing(), norm(fixed_zero) {}

  /** 
   * Constructor given bearing and magnitude
   * 
   * @param _bearing Bearing of vector (degrees true)
   * @param _norm Magnitude of vector (m/s)
   * @return Initialised object
   */
  SpeedVector(Angle _bearing, fixed _norm):bearing(_bearing), norm(_norm) {}

  /**
   * Returns true if the norm of the vector is zero.
   */
  bool is_zero() const {
    return !is_non_zero();
  }

  /**
   * Returns true if the norm of the vector is non-zero.
   */
  bool is_non_zero() const {
    return positive(norm);
  }
};

#endif
