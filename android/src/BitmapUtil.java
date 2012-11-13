/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
import android.graphics.Bitmap;
import static android.opengl.GLES11.*;
import android.opengl.GLUtils;

/**
 * Utilities for dealing with #Bitmap objects and OpenGL.
 */
final class BitmapUtil {
  private static final String TAG = "XCSoar";

  private static int nextPowerOfTwo(int i) {
    return NativeView.nextPowerOfTwo(i);
  }

  public static int validateTextureSize(int i) {
    return NativeView.validateTextureSize(i);
  }

  /**
   * Initialize the current texture and load the specified Bitmap into
   * it.
   */
  private static void loadTexture(Bitmap bmp) {
    /* try the easy way - however this fails on Android 2.2; Android
       1.6 and 2.3 are doing fine */

    GLUtils.texImage2D(GL_TEXTURE_2D, 0, bmp, 0);
    if (glGetError() != GL_INVALID_VALUE)
      /* success */
      return;

    /* size must be a power of two; create an empty texture, and load
       the Bitmap into it */

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 nextPowerOfTwo(bmp.getWidth()),
                 nextPowerOfTwo(bmp.getHeight()),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
    GLUtils.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bmp);
    if (glGetError() != GL_INVALID_OPERATION)
      /* success */
      return;

    /* the two attempts above fail on the Samsung Galaxy Tab; the
       following has been verified to work */

    Bitmap tmp = bmp.copy(Bitmap.Config.RGB_565, false);
    if (tmp == null)
      return;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 nextPowerOfTwo(bmp.getWidth()),
                 nextPowerOfTwo(bmp.getHeight()),
                 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, null);
    GLUtils.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tmp);
    tmp.recycle();
  }

  /**
   * Loads an Android Bitmap as OpenGL texture.
   *
   * @param result an array of 3 integers: texture id, width, height
   * (all output)
   * @return true on success
   */
  public static boolean bitmapToOpenGL(Bitmap bmp, int[] result) {
    if (bmp == null)
      return false;

    result[1] = bmp.getWidth();
    result[2] = bmp.getHeight();

    if (bmp.getConfig() == null) {
      /* convert to a format compatible with OpenGL */
      Bitmap tmp = bmp.copy(Bitmap.Config.RGB_565, false);
      bmp.recycle();

      if (tmp == null)
        return false;

      bmp = tmp;
    }

    /* create and configure an OpenGL texture */

    glGenTextures(1, result, 0);
    glBindTexture(GL_TEXTURE_2D, result[0]);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);

    try {
      loadTexture(bmp);
    } catch (Exception e) {
      glDeleteTextures(1, result, 0);
      Log.e(TAG, "GLUtils error: " + e);
      return false;
    } finally {
      bmp.recycle();
    }

    /* done */

    return true;
  }
}
