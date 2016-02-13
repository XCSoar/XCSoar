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

  static int configDistance(EGL10 egl, EGLDisplay display, EGLConfig config,
                            int want_r, int want_g, int want_b, int want_a,
                            int want_depth, int want_stencil) {
      int r = getConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
      int g = getConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
      int b = getConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
      int a = getConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);
      int d = getConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
      int s = getConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);
      return Math.abs(r - want_r) + Math.abs(g - want_g) +
        Math.abs(b - want_b) + Math.abs(a - want_a) +
        Math.abs(d - want_depth) + Math.abs(s - want_stencil);
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
    return "{rgba=" + getConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0) +
      "/" + getConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0) +
      "/" + getConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0) +
      "/" + getConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0) +
      "; depth=" + getConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0) +
      "; stencil=" + getConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0) +
      "}";
  }
}
