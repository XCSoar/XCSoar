// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* TextUtil.java - Android text handling to be used by C++ Code via jni.
 */

package org.xcsoar;

import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Color;

public final class TextUtil {
  private final Paint paint;
  private final Paint.FontMetricsInt metrics;
  private final int[] extent = new int[2];
  private final int[] id = new int[5];

  public TextUtil(int style, int textSize,
                  int paint_flags, boolean monospace) {
    Typeface tf = Typeface.create(monospace
                                  ? Typeface.MONOSPACE
                                  : Typeface.DEFAULT,
                                  style);

    paint = new Paint(paint_flags);
    paint.setTypeface(tf);
    paint.setTextSize(textSize);
    if ((style & Typeface.ITALIC) != 0 && !tf.isItalic())
      paint.setTextSkewX((float) -0.2);

    metrics = paint.getFontMetricsInt();
  }

  public void getFontMetrics(int[] metrics) {
    Rect bounds = new Rect();
    char[] m = new char[1];
    m[0] = 'M';
    paint.getTextBounds(m, 0, 1, bounds);

    metrics[0] = Math.round(paint.descent() - paint.ascent());
    metrics[1] = Math.round(-paint.ascent());
    metrics[2] = bounds.height();
    metrics[3] = Math.round(paint.getFontSpacing());
  }

  public int[] getTextBounds(String text) {
    /* we cannot simply use getTextBounds() here, because xcsoar will not
     * know where the baseline of the text is inside the texture
     */
    extent[0] = Math.round(paint.measureText(text, 0, text.length()));
    extent[1] = metrics.descent - metrics.ascent;
    return extent;
  }

  public int[] getTextTextureGL(String text) {
    getTextBounds(text);

    // draw text into a bitmap
    Bitmap bmp = Bitmap.createBitmap(extent[0], extent[1],
                                     Bitmap.Config.ALPHA_8);
    bmp.eraseColor(Color.TRANSPARENT);
    paint.setColor(Color.WHITE);
    Canvas canvas = new Canvas(bmp);
    canvas.drawText(text, 0, -metrics.ascent, paint);

    // create OpenGL texture
    if (!BitmapUtil.bitmapToOpenGL(bmp, true, false, id))
      return null;

    return id;
  }
}
