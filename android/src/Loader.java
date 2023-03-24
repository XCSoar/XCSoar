// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package de.opensoar;

import android.util.Log;

public class Loader {
  private static final String TAG = "OpenSoar";

  public static boolean loaded = false;
  public static String error;

  static {
    try {
      System.loadLibrary("OpenSoar");
      loaded = true;
    } catch (UnsatisfiedLinkError e) {
      Log.e(TAG, e.getMessage());
      error = e.getMessage();
    }
  }
}
