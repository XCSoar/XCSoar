/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

package org.xcsoar;

/**
 * A #I2Cbaro.Listener implementation that passes method calls to
 * native code.
 */
class NativeI2CbaroListener implements I2Cbaro.Listener {
  /**
   * A native pointer.
   */
  private final long ptr;

  NativeI2CbaroListener(long _ptr) {
    ptr = _ptr;
  }

  @Override public native void onI2CbaroValues(int sensor, int pressure);
  @Override public native void onI2CbaroError();
}
