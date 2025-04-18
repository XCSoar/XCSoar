// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import static android.opengl.GLES20.*;
import android.opengl.GLUtils;

/**
 * Utilities for dealing with #Bitmap objects and OpenGL.
 */
final class BitmapUtil {
  private static final String TAG = "XCSoar";

  public static int validateTextureSize(int i) {
    return NativeView.validateTextureSize(i);
  }

  /**
   * Initialize the current texture and load the specified Bitmap into
   * it.
   */
  private static boolean loadTexture(Bitmap bmp, int allocated_width,
                                     int allocated_height) {
    int internalFormat, format, type;
    int unpackAlignment;

    switch (bmp.getConfig()) {
    case ARGB_4444:
    case ARGB_8888:
      internalFormat = format = GL_RGBA;
      type = GL_UNSIGNED_BYTE;
      unpackAlignment = 4;
      break;

    case RGB_565:
      internalFormat = format = GL_RGB;
      type = GL_UNSIGNED_SHORT_5_6_5;
      unpackAlignment = 2;
      break;

    case ALPHA_8:
      internalFormat = format = GL_ALPHA;
      type = GL_UNSIGNED_BYTE;
      unpackAlignment = 1;
      break;

    default:
      return false;
    }

    /* create an empty texture, and load the Bitmap into it */

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                 allocated_width, allocated_height,
                 0, format, type, null);
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
    GLUtils.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bmp, format, type);
    return true;
  }

  /**
   * Copy the red channel to a new Bitmap's alpha channel.  The old
   * one will be recycled.
   */
  static Bitmap redToAlpha(Bitmap src, boolean recycle) {
    Bitmap dest = Bitmap.createBitmap(src.getWidth(), src.getHeight(),
                                      Bitmap.Config.ARGB_8888);

    float[] matrix = new float[] {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 0, 0, 0, 0
    };

    Paint paint = new Paint();
    paint.setColorFilter(new ColorMatrixColorFilter(new ColorMatrix(matrix)));

    Canvas canvas = new Canvas(dest);
    canvas.setDensity(Bitmap.DENSITY_NONE);
    canvas.drawBitmap(src, 0, 0, paint);

    if (recycle)
      src.recycle();
    return dest;
  }

  /**
   * Convert the Bitmap to ALPHA_8.  The old one will be recycled.
   */
  static Bitmap toAlpha8(Bitmap src, boolean recycle) {
    if (!src.hasAlpha()) {
      src = redToAlpha(src, recycle);
      recycle = true;
    }

    Bitmap dest = src.copy(Bitmap.Config.ALPHA_8, false);
    if (recycle)
      src.recycle();
    return dest;
  }

  /**
   * Loads an Android Bitmap as OpenGL texture.
   *
   * @param recycle recycle the #Bitmap parameter?
   * @param alpha expect a GL_ALPHA texture?
   * @param result an array of 5 integers: texture id, width, height,
   * allocated width, allocated height (all output)
   * @return true on success
   */
  public static boolean bitmapToOpenGL(Bitmap bmp, boolean recycle,
                                       boolean alpha,
                                       int[] result) {
    result[1] = bmp.getWidth();
    result[2] = bmp.getHeight();
    result[3] = validateTextureSize(result[1]);
    result[4] = validateTextureSize(result[2]);

    if (alpha && bmp.getConfig() != Bitmap.Config.ALPHA_8) {
      bmp = toAlpha8(bmp, recycle);
      recycle = true;
    } else if (bmp.getConfig() == null) {
      /* convert to a format compatible with OpenGL */
      Bitmap.Config config = bmp.hasAlpha()
        ? Bitmap.Config.ARGB_8888
        : Bitmap.Config.RGB_565;

      Bitmap tmp = bmp.copy(config, false);
      if (recycle)
        bmp.recycle();

      if (tmp == null)
        return false;

      bmp = tmp;
      recycle = true;
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
      if (!loadTexture(bmp, result[3], result[4])) {
        glDeleteTextures(1, result, 0);
        return false;
      }
    } catch (Exception e) {
      glDeleteTextures(1, result, 0);
      Log.e(TAG, "GLUtils error", e);
      return false;
    } finally {
      if (recycle)
        bmp.recycle();
    }

    /* done */

    return true;
  }
}
