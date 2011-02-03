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

#include "Screen/Font.hpp"

#ifdef ANDROID
#include "Java/Global.hpp"
#include "Java/Class.hpp"

#include <assert.h>

JNIEnv *Font::env(NULL);
jmethodID Font::midTextUtil(NULL);
jmethodID Font::midGetFontMetrics(NULL);
jmethodID Font::midGetTextBounds(NULL);
jmethodID Font::midGetTextTextureGL(NULL);


bool
Font::set(const LOGFONT &log)
{
  return set(log.lfFaceName, (int) log.lfHeight,
             log.lfWeight > 600, log.lfItalic != 0);
}

/*
 * create a new instance of org.xcsoar.TextUtil and store it with a global
 * reference in textUtilObject member.
 */
bool
Font::set(const TCHAR *facename, int height, bool bold, bool italic)
{
  jobject localObject;
  jstring paramFamilyName;
  jint paramStyle, paramTextSize;
  jintArray metricsArray;
  jint metrics[5];

  reset();

  if (env == NULL) {
    // initialize static jvm
    env = Java::GetEnv();
  }

  Java::Class textUtilClass(env, "org/xcsoar/TextUtil");

  if (midTextUtil == NULL) {
    // initialize static method ID's once
    midTextUtil         = env->GetMethodID(textUtilClass, "<init>",
                                           "(Ljava/lang/String;II)V");
    midGetFontMetrics   = env->GetMethodID(textUtilClass, "getFontMetrics",
                                           "([I)V");
    midGetTextBounds    = env->GetMethodID(textUtilClass, "getTextBounds",
                                           "(Ljava/lang/String;[I)V");
    midGetTextTextureGL = env->GetMethodID(textUtilClass, "getTextTextureGL",
                                           "(Ljava/lang/String;IIIIII)I");
  }

#ifdef UNICODE
  paramFamilyName = env->NewString(facename, wcslen(facename));
#else
  paramFamilyName = env->NewStringUTF(facename);
#endif
  paramStyle = 0;
  if (bold)
    paramStyle |= 1;
  if (italic)
    paramStyle |= 2;
  paramTextSize = height;

  // construct org.xcsoar.TextUtil object
  localObject = env->NewObject(textUtilClass, midTextUtil, paramFamilyName,
                               paramStyle, paramTextSize);
  if (!localObject)
    return false;

  textUtilObject = env->NewGlobalRef(localObject);
  if (!textUtilObject)
    return false;

  // get height, ascent_height and capital_height
  assert(midGetFontMetrics);
  metricsArray = env->NewIntArray(5);
  env->CallVoidMethod(textUtilObject, midGetFontMetrics, metricsArray);
  env->GetIntArrayRegion(metricsArray, 0, 5, metrics);
  this->height = metrics[0];
  style = metrics[1];
  ascent_height = metrics[2];
  capital_height = metrics[3];
  line_spacing = metrics[4];

  // store face name. android API does not provide ways to query it.
  Font::facename = facename;

  // free local references
  env->DeleteLocalRef(metricsArray);
  env->DeleteLocalRef(localObject);
  env->DeleteLocalRef(paramFamilyName);

  return true;
}

void
Font::reset()
{
  if (textUtilObject) {
    env->DeleteGlobalRef(textUtilObject);
    textUtilObject = NULL;
  }
}

void
Font::text_width(const TCHAR *text, int &width, int &height) const
{
  jstring paramText;
  jintArray paramExtent;
  jint extent[2];

  if (!textUtilObject)
    return;

#ifdef UNICODE
  paramText = env->NewString(text, wcslen(text));
#else
  paramText = env->NewStringUTF(text);
#endif
  paramExtent = env->NewIntArray(2);

  env->CallVoidMethod(textUtilObject, midGetTextBounds,
                      paramText, paramExtent);
  env->GetIntArrayRegion(paramExtent, 0, 2, extent);

  width = extent[0];
  height = extent[1];

  // free local references
  env->DeleteLocalRef(paramText);
  env->DeleteLocalRef(paramExtent);
}

int
Font::text_texture_gl(const TCHAR *text, SIZE &size,
                      const Color &fg, const Color &bg) const
{
  jstring paramText;
  jint jfg[3] = { fg.red(), fg.green(), fg.blue() };
  jint jbg[3] = { bg.red(), bg.green(), bg.blue() };
  jint textureID;

  if (!textUtilObject)
    return NULL;

  size.cx = size.cy = 0;
  text_width(text, size);
  if (size.cx == 0 || size.cy == 0)
    return NULL;

#ifdef UNICODE
  paramText = env->NewString(text, wcslen(text));
#else
  paramText = env->NewStringUTF(text);
#endif

  textureID = env->CallIntMethod(textUtilObject, midGetTextTextureGL,
                                 paramText,
                                 jfg[0], jfg[1], jfg[2],
                                 jbg[0], jbg[1], jbg[2]);

  // free local references
  env->DeleteLocalRef(paramText);

  return textureID;
}


#else // !ANDROID

#ifdef ENABLE_SDL

bool
Font::_set(const char *file, int ptsize, bool bold, bool italic)
{
  reset();

  font = TTF_OpenFont(file, ptsize);
  if (font == NULL)
    return false;

  int style = TTF_STYLE_NORMAL;
  if (bold)
    style |= TTF_STYLE_BOLD;
  if (italic)
    style |= TTF_STYLE_ITALIC;
  TTF_SetFontStyle(font, style);

  calculate_heights();

  return true;
}

bool
Font::set(const TCHAR *facename, int height, bool bold, bool italic)
{
  LOGFONT lf;
  lf.lfWeight = bold ? 700 : 500;
  lf.lfHeight = height;
  lf.lfItalic = italic;
  return set(lf);
}

bool
Font::set(const LOGFONT &log_font)
{
  // XXX hard coded path
  const char *dejavu_ttf =
    "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf";

#ifdef ANDROID
  dejavu_ttf = "/system/fonts/DroidSans.ttf";
#endif

  return _set(dejavu_ttf, log_font.lfHeight > 0 ? log_font.lfHeight : 10,
              log_font.lfWeight >= 700,
              log_font.lfItalic);
}

void
Font::calculate_heights()
{
  height = TTF_FontHeight(font);
  ascent_height = TTF_FontAscent(font);

  int miny, maxy;
  TTF_GlyphMetrics(font, 'M', NULL, NULL, &miny, &maxy, NULL);

  capital_height = maxy - miny + 1;
}

void
Font::reset()
{
  if (font != NULL) {
    TTF_CloseFont(font);
    font = NULL;
  }
}

#else /* !ENABLE_SDL */

#include "Screen/BufferCanvas.hpp"
#include "Asset.hpp"

bool
Font::set(const TCHAR* facename, int height, bool bold, bool italic)
{
  LOGFONT font;
  memset((char *)&font, 0, sizeof(LOGFONT));

  _tcscpy(font.lfFaceName, facename);
  font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  font.lfHeight = (long)height;
  font.lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font.lfItalic = italic;
  if (is_altair()) // better would be: if (screen.dpi() < 100)
    font.lfQuality = NONANTIALIASED_QUALITY;
  else
    font.lfQuality = ANTIALIASED_QUALITY;
  return Font::set(font);
}

bool
Font::set(const LOGFONT &log_font)
{
  reset();

  font = ::CreateFontIndirect(&log_font);
  if (font == NULL)
    return false;

  if (GetObjectType(font) != OBJ_FONT) {
    reset();
    return false;
  }

  calculate_heights();

  return true;
}

void
Font::calculate_heights()
{
  VirtualCanvas canvas(1, 1);
  canvas.select(*this);

  TEXTMETRIC tm;
  ::GetTextMetrics(canvas, &tm);

  height = tm.tmHeight;
  ascent_height = tm.tmAscent;

  if (is_altair()) {
    // JMW: don't know why we need this in GNAV, but we do.

    BufferCanvas buffer(canvas, tm.tmAveCharWidth, tm.tmHeight);
    const HWColor white = buffer.map(Color::WHITE);

    buffer.background_opaque();
    buffer.set_background_color(Color::WHITE);
    buffer.set_text_color(Color::BLACK);
    buffer.select(*this);

    RECT rec;
    rec.left = 0;
    rec.top = 0;
    rec.right = tm.tmAveCharWidth;
    rec.bottom = tm.tmHeight;
    buffer.text_opaque(0, 0, rec, _T("M"));

    int top = tm.tmHeight, bottom = 0;

    for (int x = 0; x < tm.tmAveCharWidth; ++x) {
      for (int y = 0; y < tm.tmHeight; ++y) {
        if (buffer.get_pixel(x, y) != white) {
          if (top > y)
            top = y;
          if (bottom < y)
            bottom = y;
        }
      }
    }

    capital_height = bottom - top + 1;
  } else {
    // This works for PPC
    capital_height = tm.tmAscent - 1 - tm.tmHeight / 10;
  }
}

void
Font::reset()
{
  if (font != NULL) {
    ::DeleteObject(font);
    font = NULL;
  }
}

#endif /* !ENABLE_SDL */
#endif /* !ANDROID */
