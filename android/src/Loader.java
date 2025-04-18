// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.util.Log;

public class Loader {
  private static final String TAG = "XCSoar";

  public static boolean loaded = false;
  public static String error;

  static {
    try {
      System.loadLibrary("xcsoar");
      loaded = true;
    } catch (UnsatisfiedLinkError e) {
      Log.e(TAG, e.getMessage());
      error = e.getMessage();
    }
  }
}
