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
  /** Vertical em scale from Display::AndroidTextScaleY(). */
  private static float textScaleY = 1f;

  /** Horizontal advance compensation from Display::AndroidTextLetterSpacing(). */
  private static float textLetterSpacing = 0f;

  private final Paint paint;
  private final Paint.FontMetricsInt metrics;
  private final boolean perGlyphLayout;
  private final int[] extent = new int[2];
  private final int[] id = new int[5];

  public static void setCorrectedDpi(float scaleY, float letterSpacing) {
    textScaleY = scaleY;
    textLetterSpacing = letterSpacing;
  }

  /** Framebuffer pixels, not dp/sp (see bitmap canvas DENSITY_NONE). */
  private static void setRawPixelDensity(Paint paint) {
    try {
      Paint.class.getMethod("setDensity", Integer.TYPE)
        .invoke(paint, Bitmap.DENSITY_NONE);
    } catch (ReflectiveOperationException e) {
      /* Stub SDKs omit setDensity; COMPLEX_UNIT_PX below is enough. */
    }
  }

  private static void applySkiaPaintSettings(Paint paint,
                                             boolean aliasEdging,
                                             boolean linearMetrics) {
    paint.setAntiAlias(!aliasEdging);
    paint.setSubpixelText(false);
    paint.setLinearText(linearMetrics);
    paint.setHinting(linearMetrics
                     ? Paint.HINTING_OFF
                     : Paint.HINTING_ON);
    paint.setFakeBoldText(false);
    paint.setStrikeThruText(false);
    paint.setUnderlineText(false);
    paint.setFilterBitmap(false);
    paint.setDither(false);
    paint.setTextScaleX(1f);
    setRawPixelDensity(paint);
    if (Build.VERSION.SDK_INT >= 28)
      paint.setElegantTextHeight(false);
  }

  private static Typeface createTypeface(boolean monospace, boolean bold,
                                         boolean italic, boolean aliasEdging) {
    if (Build.VERSION.SDK_INT >= 28) {
      /* Alias e-paper: thin regular (100), normal bold (400).
         LCD: 400 / 700.  Layout em is unchanged. */
      final int weight;
      if (aliasEdging && !bold)
        weight = 100;
      else if (aliasEdging)
        weight = 400;
      else
        weight = bold ? 700 : 400;
      final Typeface family;
      if (monospace)
        family = Typeface.MONOSPACE;
      else if (aliasEdging && !bold)
        family = Typeface.create("sans-serif-thin", Typeface.NORMAL);
      else if (aliasEdging && bold)
        family = Typeface.SANS_SERIF;
      else
        family = Typeface.SANS_SERIF;
      return Typeface.create(family, weight, italic);
    }

    final Typeface family = monospace
      ? Typeface.MONOSPACE
      : Typeface.SANS_SERIF;

    int style = Typeface.NORMAL;
    if (bold)
      style |= Typeface.BOLD;
    if (italic)
      style |= Typeface.ITALIC;
    return Typeface.create(family, style);
  }

  /** Derive the text size that yields em == targetEmPx (framebuffer px). */
  private static void fitEmHeight(Paint paint, float targetEmPx) {
    paint.setTextSize(targetEmPx);
    final Paint.FontMetricsInt fm = paint.getFontMetricsInt();
    final float em = fm.descent - fm.ascent;
    if (em > 0f && Math.abs(em - targetEmPx) > 0.5f)
      paint.setTextSize(targetEmPx * targetEmPx / em);
  }

  private float measureTextRun(String text) {
    float width = 0;
    for (int i = 0; i < text.length();) {
      final int cp = text.codePointAt(i);
      final int len = Character.charCount(cp);
      width += paint.measureText(text, i, i + len);
      i += len;
    }
    return width;
  }

  private void drawTextRun(Canvas canvas, String text, float x, float y) {
    float cx = x;
    for (int i = 0; i < text.length();) {
      final int cp = text.codePointAt(i);
      final int len = Character.charCount(cp);
      canvas.drawText(text, i, i + len, cx, y, paint);
      cx += paint.measureText(text, i, i + len);
      i += len;
    }
  }

  public TextUtil(int textSize, boolean bold, boolean italic,
                  boolean monospace, boolean aliasEdging,
                  boolean linearMetrics, boolean perGlyphLayout,
                  float fontLetterSpacingEm) {
    paint = new Paint(0);
    setRawPixelDensity(paint);

    final Typeface tf = createTypeface(monospace, bold, italic, aliasEdging);
    paint.setTypeface(tf);

    applySkiaPaintSettings(paint, aliasEdging, linearMetrics);

    final float targetEmPx = textSize * textScaleY;
    paint.setTextSize(targetEmPx);
    fitEmHeight(paint, targetEmPx);

    if (Build.VERSION.SDK_INT >= 21)
      paint.setLetterSpacing(textLetterSpacing + fontLetterSpacingEm);

    if (italic && !tf.isItalic())
      paint.setTextSkewX((float) -0.2);
    else
      paint.setTextSkewX(0f);

    this.perGlyphLayout = perGlyphLayout;
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
    if (perGlyphLayout)
      extent[0] = Math.round(measureTextRun(text));
    else
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
    if (perGlyphLayout)
      drawTextRun(canvas, text, 0, -metrics.ascent);
    else
      canvas.drawText(text, 0, -metrics.ascent, paint);

    if (!BitmapUtil.bitmapToOpenGL(bmp, true, false, id))
      return null;

    return id;
  }
}
