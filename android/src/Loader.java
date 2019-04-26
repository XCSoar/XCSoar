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

package org.xcsoar;

import android.util.Log;

public class Loader {
  private static final String TAG = "XCSoar";

  public static boolean loaded = false;
  public static boolean isLegacy = false;
  public static String error;

  static {
    try {
      /* Depending on the Android version we load the fully functional library
       * or the functional slightly restricted version which is compatible down to
       * API level 12 (Honeycomb), and implements some work-arounds for missing functions
       * in the Bionic C library
       */

      if (android.os.Build.VERSION.SDK_INT >= 21) {
        System.loadLibrary("xcsoar");
        Log.i(TAG, "Loaded native library xcsoar");
        loaded = true;
      }
    } catch (UnsatisfiedLinkError e) {
      Log.w(TAG, "Native library xcsoar cannot be loaded. Load error = ");
      Log.w(TAG, e.getMessage());
      Log.w(TAG, "You may ignore the load error when this is a legacy-only build.");
    }

    /* Library xcsoar.so was not be loaded because
     * - It is running on an older Android version < API 21
     * - It is a legacy-only build of the APK
     * - Something with loading of the library xcsoar simply went wrong.
     * Try the same with the library xcsoar_legacy. If that does not work give up.
     */
    try {
      if (!loaded) {
        System.loadLibrary("xcsoar_legacy");
        Log.i(TAG, "Loaded native library xcsoar_legacy");
        isLegacy = true;
        loaded = true;
      }
    } catch (UnsatisfiedLinkError e) {
      Log.e(TAG, e.getMessage());
      error = e.getMessage();
    }
  }
}
