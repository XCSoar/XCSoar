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

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

/**
 * Utilities for the EGL API.
 */
class EGLUtil {
  /**
   * Wrapper for EGL10.eglGetConfigAttrib() with a fallback value just
   * in case the function fails.
   */
  static int getConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config,
                             int attribute, int defaultValue) {
    int[] mValue = new int[1];
    return egl.eglGetConfigAttrib(display, config, attribute, mValue)
      ? mValue[0]
      : defaultValue;
  }

  static int attribDistance(EGL10 egl, EGLDisplay display, EGLConfig config,
                            int attribute, int want) {
    int value = getConfigAttrib(egl, display, config, attribute, 0);
    int distance = Math.abs(value - want);
    if (want > 0 && value == 0)
      /* big penalty if this attribute if zero, but XCSoar prefers it
         to be non-zero */
      distance += 100;
    else if (want == 0 && value > 0)
      /* small penalty if this attribute is non-zero, but XCSoar
         prefers it to be zero */
      distance += 10;
    return distance;
  }

  static int configDistance(EGL10 egl, EGLDisplay display, EGLConfig config,
                            int want_r, int want_g, int want_b, int want_a,
                            int want_depth, int want_stencil) {
    int distance = 0;

    int caveat = getConfigAttrib(egl, display, config,
                                 EGL10.EGL_CONFIG_CAVEAT, EGL10.EGL_NONE);
    if (caveat != EGL10.EGL_NONE)
      /* large penalty for unaccelerated software renderer configs */
      distance += 1000;

    int r = attribDistance(egl, display, config,
                           EGL10.EGL_RED_SIZE, want_r);
    int g = attribDistance(egl, display, config,
                           EGL10.EGL_GREEN_SIZE, want_g);
    int b = attribDistance(egl, display, config,
                           EGL10.EGL_BLUE_SIZE, want_b);
    int a = attribDistance(egl, display, config,
                           EGL10.EGL_ALPHA_SIZE, want_a);
    int d = attribDistance(egl, display, config,
                           EGL10.EGL_DEPTH_SIZE, want_depth);
    int s = attribDistance(egl, display, config,
                           EGL10.EGL_STENCIL_SIZE, want_stencil);

    return distance + r + g + b + a + d + s;
  }

  static EGLConfig findClosestConfig(EGL10 egl, EGLDisplay display,
                                     EGLConfig[] configs,
                                     int want_r, int want_g, int want_b,
                                     int want_a,
                                     int want_depth, int want_stencil) {
    EGLConfig closestConfig = null;
    int closestDistance = 1000;
    for (EGLConfig config : configs) {
      int distance = configDistance(egl, display, config,
                                    want_r, want_g, want_b, want_a,
                                    want_depth, want_stencil);
      if (distance < closestDistance) {
        closestDistance = distance;
        closestConfig = config;
      }
    }

    return closestConfig;
  }

  /**
   * Produce a human-readable string describing the #EGLConfig (for
   * debug log messages).
   */
  static String toString(EGL10 egl, EGLDisplay display, EGLConfig config) {
    String s = "{";

    int luminance = getConfigAttrib(egl, display, config,
                                    EGL10.EGL_LUMINANCE_SIZE, 0);
    if (luminance > 0)
      s += "la=" + luminance;
    else
      s += "rgba=" + getConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0) +
        "/" + getConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0) +
        "/" + getConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);

    s += "/" + getConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);

    s += "; depth=" + getConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0) +
      "; stencil=" + getConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);

    s += "}";

    return s;
  }
}
