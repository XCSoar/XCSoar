// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* TextUtil.java - Android text handling to be used by C++ Code via jni.
 */

package org.xcsoar;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.os.Build;

public final class TextUtil {
  private final Paint paint;
  private final Paint.FontMetricsInt metrics;
  private final int[] extent = new int[2];
  private final int[] id = new int[5];

  /** Framebuffer pixels, not dp/sp (see bitmap canvas DENSITY_NONE). */
  private static void setRawPixelDensity(Paint paint) {
    try {
      Paint.class.getMethod("setDensity", Integer.TYPE)
        .invoke(paint, Bitmap.DENSITY_NONE);
    } catch (ReflectiveOperationException e) {
      /* Stub SDKs omit setDensity; setTextSize() is already pixels. */
    }
  }

  private static void applySkiaPaintSettings(Paint paint,
                                             boolean aliasEdging,
                                             boolean linearMetrics) {
    paint.setAntiAlias(!aliasEdging);
    /* E-paper draws unhinted 1-bit glyphs. */
    if (linearMetrics) {
      paint.setLinearText(true);
      paint.setHinting(Paint.HINTING_OFF);
    }
    setRawPixelDensity(paint);
  }

  private static Typeface createTypeface(boolean monospace, boolean bold,
                                         boolean italic) {
    final Typeface family = monospace
      ? Typeface.MONOSPACE
      : Typeface.SANS_SERIF;

    /* The style bit does not set the wght axis on Android 14 variable
       fonts, so bold is sometimes the regular face and sometimes a
       heavy fallback.  400 and 700 are the designed cuts. */
    if (Build.VERSION.SDK_INT >= 28)
      return Typeface.create(family, bold ? 700 : 400, italic);

    int style = Typeface.NORMAL;
    if (bold)
      style |= Typeface.BOLD;
    if (italic)
      style |= Typeface.ITALIC;
    return Typeface.create(family, style);
  }

  public TextUtil(int textSize, boolean bold, boolean italic,
                  boolean monospace, boolean aliasEdging,
                  boolean linearMetrics,
                  float fontLetterSpacingEm) {
    paint = new Paint(0);

    final Typeface tf = createTypeface(monospace, bold, italic);
    paint.setTypeface(tf);

    applySkiaPaintSettings(paint, aliasEdging, linearMetrics);

    /* Same pixel size the other font engines get from FontScale.
       Do not refit the em: bold and regular have different boxes, so
       the refit changes their weight. */
    paint.setTextSize(textSize);
    paint.setLetterSpacing(fontLetterSpacingEm);

    if (italic && !tf.isItalic())
      paint.setTextSkewX((float) -0.2);
    else
      paint.setTextSkewX(0f);

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
    extent[0] = Math.round(paint.measureText(text));
    extent[1] = metrics.descent - metrics.ascent;
    return extent;
  }

  public int[] getTextTextureGL(String text) {
    getTextBounds(text);

    Bitmap bmp = Bitmap.createBitmap(extent[0], extent[1],
                                     Bitmap.Config.ALPHA_8);
    bmp.setDensity(Bitmap.DENSITY_NONE);
    bmp.eraseColor(Color.TRANSPARENT);
    paint.setColor(Color.WHITE);
    Canvas canvas = new Canvas(bmp);
    canvas.setDensity(Bitmap.DENSITY_NONE);
    canvas.drawText(text, 0, -metrics.ascent, paint);

    if (!BitmapUtil.bitmapToOpenGL(bmp, true, false, id))
      return null;

    return id;
  }
}
