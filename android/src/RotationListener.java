// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.hardware.SensorManager;
import android.view.OrientationEventListener;

/**
 * Listens for physical device orientation changes and notifies native
 * code to show a rotation suggestion button.  The actual target
 * orientation is determined at button-press time via
 * {@link #getPhysicalOrientation()}.
 *
 * OrientationEventListener reports degrees of device rotation:
 * 0 = portrait (upright), 90 = reverse landscape (top RIGHT),
 * 180 = reverse portrait (upside-down),
 * 270 = landscape (top LEFT).
 */
class RotationListener extends OrientationEventListener {
  /** last orientation bucket, to fire only on changes */
  private int lastBucket = -1;

  /**
   * The latest sensor angle (0-359), or -1 if unknown.
   * Read by {@link #getPhysicalOrientation()}.
   */
  private volatile int currentSensorAngle = -1;

  RotationListener(Context context) {
    super(context, SensorManager.SENSOR_DELAY_NORMAL);
  }

  @Override
  public void onOrientationChanged(int orientation) {
    if (orientation == ORIENTATION_UNKNOWN)
      return;

    currentSensorAngle = orientation;

    /* map to one of 4 orientation buckets with 30-degree
       hysteresis zones; the actual target orientation is
       determined later when the button is pressed */
    int bucket;
    if (orientation <= 30 || orientation >= 330)
      bucket = 0;
    else if (orientation >= 60 && orientation <= 120)
      bucket = 90;
    else if (orientation >= 150 && orientation <= 210)
      bucket = 180;
    else if (orientation >= 240 && orientation <= 300)
      bucket = 270;
    else
      return; /* transition zone */

    if (bucket != lastBucket) {
      lastBucket = bucket;
      NativeView.onRotationSuggestion();
    }
  }

  /**
   * Return the DisplayOrientation enum value matching the device's
   * current physical orientation (from the latest sensor reading).
   *
   * @return 0=DEFAULT (unknown), 1=PORTRAIT, 2=LANDSCAPE,
   *         3=REVERSE_PORTRAIT, 4=REVERSE_LANDSCAPE
   */
  int getPhysicalOrientation() {
    int angle = currentSensorAngle;
    if (angle < 0)
      return 0; /* unknown */

    if (angle <= 45 || angle >= 315)
      return 1; /* PORTRAIT */
    else if (angle < 135)
      return 4; /* REVERSE_LANDSCAPE (top RIGHT) */
    else if (angle < 225)
      return 3; /* REVERSE_PORTRAIT (upside-down) */
    else
      return 2; /* LANDSCAPE (top LEFT) */
  }
}
