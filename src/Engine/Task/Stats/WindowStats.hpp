/* Copyright_License {

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

#ifndef XCSOAR_WINDOW_STATS_HPP
#define XCSOAR_WINDOW_STATS_HPP

struct WindowStats {
  /**
   * The duration of this window [seconds].  A negative value means
   * this object is undefined.
   */
  double duration;

  /**
   * The distance travelled in this window.
   */
  double distance;

  /**
   * The quotient of distance and duration.
   */
  double speed;

  void Reset() {
    duration = -1;
  }
};

#endif
