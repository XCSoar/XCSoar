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

#ifndef XCSOAR_ACCELERATION_HPP
#define XCSOAR_ACCELERATION_HPP

/**
 * State of acceleration of aircraft
 */
struct AccelerationState
{
  /**
   * Is G-load information available?
   * @see Gload
   */
  bool available;

  /**
   * Is the G-load information coming from a connected device (true) or
   * was it calculated by XCSoar (false)
   */
  bool real;

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  double g_load;

  void Reset() {
    available = false;
  }

  void ProvideGLoad(double _g_load, bool _real) {
    g_load = _g_load;
    real = _real;
    available = true;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AccelerationState &add);
};

#endif
