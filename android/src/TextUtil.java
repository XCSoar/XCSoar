/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
import android.opengl.GLES11;
import static android.opengl.GLES11.*;
import java.nio.ShortBuffer;

public class TextUtil {
  private Paint paint;

  public TextUtil(String family_name, int style, int textSize) {
    Typeface tf = Typeface.create(family_name, style);
    paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    paint.setTypeface(tf);
    paint.setTextSize(textSize);
    if ((style & Typeface.ITALIC) != 0 && !tf.isItalic())
      paint.setTextSkewX((float) -0.2);
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

  public void getTextBounds(String text, int[] extent) {
    /* we cannot simply use getTextBounds() here, because xcsoar will not
     * know where the baseline of the text is inside the texture
     */
    Paint.FontMetricsInt metrics = paint.getFontMetricsInt();
    extent[0] = Math.round(paint.measureText(text, 0, text.length()));
    extent[1] = metrics.descent - metrics.ascent;
  }

  public int getTextTextureGL(String text,
                              int fg_red, int fg_green, int fg_blue,
                              int bg_red, int bg_green, int bg_blue) {
    int[] extent = new int[2];
    getTextBounds(text, extent);

    // draw text into a bitmap
    Bitmap bmp = Bitmap.createBitmap(extent[0], extent[1], Bitmap.Config.RGB_565);
    bmp.eraseColor(Color.rgb(bg_red, bg_green, bg_blue));
    paint.setColor(Color.rgb(fg_red, fg_green, fg_blue));
    Canvas canvas = new Canvas(bmp);
    canvas.drawText(text, 0, -paint.getFontMetricsInt().ascent, paint);

    // get bitmap pixels
    ShortBuffer pixels = ShortBuffer.allocate(extent[0] * extent[1]);
    bmp.copyPixelsToBuffer(pixels);

    // create OpenGL texture
    int[] id = new int[1];
    int width2 = next_power_of_two(extent[0]);
    int height2 = next_power_of_two(extent[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glGenTextures(1, id, 0);
    glBindTexture(GL_TEXTURE_2D, id[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (extent[0] == width2 && extent[1] == height2) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2,
                   0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2,
                   0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, null);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, extent[0], extent[1],
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
    }

    return id[0];
  }

  private int next_power_of_two(int i) {
    int p = 1;
    while (p < i) {
      p <<= 1;
    }
    return p;
  }
}
