/*
Copyright_License {

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

/* TextUtil.java - Android text handling to be used by C++ Code via jni.
 */

package org.xcsoar;

import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Color;

public class TextUtil {
  private Paint paint;
  private Paint.FontMetricsInt metrics;
  private int[] extent = new int[2];
  private int[] id = new int[5];

  public TextUtil(int style, int textSize,
                  int paint_flags, boolean monospace) {
    Typeface tf = monospace
      ? Typeface.MONOSPACE
      : Typeface.create((Typeface)null, style);

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
    metrics[1] = paint.getTypeface().getStyle();
    metrics[2] = Math.round(-paint.ascent());
    metrics[3] = bounds.height();
    metrics[4] = Math.round(paint.getFontSpacing());
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
    canvas.drawText(text, 0, -paint.getFontMetricsInt().ascent, paint);

    // create OpenGL texture
    if (!BitmapUtil.bitmapToOpenGL(bmp, true, false, id))
      return null;

    return id;
  }
}
