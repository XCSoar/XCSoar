/* Copyright_License {

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

import android.view.View;

class ImmersiveFullScreenMode {
  /**
   * Set / Reset the System UI visibility flags for Immersive Full
   * Screen Mode.  Requires API level 19 (Android 4.4 "KitKat").
   */
  public static void enable(View decorView) {
    decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|
                                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
                                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|
                                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
                                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|
                                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
  }
}
