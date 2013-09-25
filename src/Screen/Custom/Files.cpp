/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Files.hpp"
#include "OS/FileUtil.hpp"
#include "Compiler.h"

static const char *const all_font_paths[] = {
#ifdef __APPLE__
  "/Library/Fonts/Tahoma.ttf",
  "/Library/Fonts/Georgia.ttf",
  "/Library/Fonts/Arial Narrow.ttf",
  "/Library/Fonts/Times New Roman.ttf",
  "/Library/Fonts/Microsoft/Arial.ttf",
#elif defined(WIN32) && !defined(HAVE_POSIX)
  /* just for the experimental WINSDL target */
  "c:\\windows\\fonts\\arial.ttf",
#elif defined(KOBO)
  "/mnt/onboard/XCSoar/fonts/DejaVuSansCondensed.ttf",
  "/mnt/onboard/fonts/Vera.ttf",
  "/opt/xcsoar/share/fonts/Vera.ttf",
#else
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf",
  "/usr/share/fonts/TTF/dejavu/DejaVuSansCondensed.ttf",
  "/usr/share/fonts/truetype/DejaVuSansCondensed.ttf",
  "/usr/share/fonts/dejavu/DejaVuSansCondensed.ttf",
  "/usr/share/fonts/truetype/droid/DroidSans.ttf",
  "/usr/share/fonts/truetype/ttf-droid/DroidSans.ttf",
  "/usr/share/fonts/TTF/droid/DroidSans.ttf",
  "/usr/share/fonts/droid/DroidSans.ttf",
  "/usr/share/fonts/truetype/droid/DroidSans.ttf",
  "/usr/share/fonts/truetype/DroidSans.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
  "/usr/share/fonts/corefonts/arial.ttf",
  "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
  "/usr/share/fonts/freefont-ttf/FreeSans.ttf",
  "/usr/share/fonts/truetype/unifont/unifont.ttf",
  "/usr/share/fonts/unifont/unifont.ttf",
  "/usr/share/fonts/corefonts/tahoma.ttf",
#endif
  NULL
};

static const char *const all_bold_font_paths[] = {
#ifdef __APPLE__
  "/Library/Fonts/Tahoma Bold.ttf",
  "/Library/Fonts/Georgia Bold.ttf",
  "/Library/Fonts/Arial Narrow Bold.ttf",
  "/Library/Fonts/Microsoft/Arial Bold.ttf",
#elif defined(KOBO)
  "/mnt/onboard/XCSoar/fonts/DejaVuSansCondensed-Bold.ttf",
  "/mnt/onboard/fonts/VeraBd.ttf",
  "/opt/xcsoar/share/fonts/VeraBd.ttf",
#elif defined(HAVE_POSIX)
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed-Bold.ttf",
  "/usr/share/fonts/TTF/dejavu/DejaVuSansCondensed-Bold.ttf",
  "/usr/share/fonts/truetype/DejaVuSansCondensed-Bold.ttf",
  "/usr/share/fonts/dejavu/DejaVuSansCondensed-Bold.ttf",
  "/usr/share/fonts/truetype/droid/DroidSans-Bold.ttf",
  "/usr/share/fonts/truetype/ttf-droid/DroidSans-Bold.ttf",
  "/usr/share/fonts/droid/DroidSans-Bold.ttf",
  "/usr/share/fonts/TTF/droid/DroidSans-Bold.ttf",
  "/usr/share/fonts/truetype/droid/DroidSans-Bold.ttf",
  "/usr/share/fonts/truetype/DroidSans-Bold.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf",
  "/usr/share/fonts/corefonts/arialbd.ttf",
  "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
  "/usr/share/fonts/freefont-ttf/FreeSansBold.ttf",
#endif
  nullptr
};

static const char *const all_italic_font_paths[] = {
#if defined(KOBO)
  "/mnt/onboard/XCSoar/fonts/DejaVuSansCondensed-Oblique.ttf",
  "/mnt/onboard/fonts/VeraIt.ttf",
  "/mnt/onboard/XCSoar/fonts/VeraIt.ttf",
  "/opt/xcsoar/share/fonts/VeraIt.ttf",
#elif defined(HAVE_POSIX)
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed-Oblique.ttf",
  "/usr/share/fonts/TTF/dejavu/DejaVuSansCondensed-Oblique.ttf",
  "/usr/share/fonts/truetype/DejaVuSansCondensed-Oblique.ttf",
  "/usr/share/fonts/dejavu/DejaVuSansCondensed-Oblique.ttf",
  "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraIt.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/Arial_Italic.ttf",
  "/usr/share/fonts/corefonts/ariali.ttf",
  "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf",
  "/usr/share/fonts/freefont-ttf/FreeSansOblique.ttf",
#endif
  nullptr
};

static const char *const all_bold_italic_font_paths[] = {
#if defined(KOBO)
  "/mnt/onboard/XCSoar/fonts/DejaVuSansCondensed-BoldOblique.ttf",
  "/mnt/onboard/fonts/VeraBI.ttf",
  "/mnt/onboard/XCSoar/fonts/VeraBI.ttf",
  "/opt/xcsoar/share/fonts/VeraBI.ttf",
#elif defined(HAVE_POSIX)
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "/usr/share/fonts/TTF/dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "/usr/share/fonts/truetype/DejaVuSansCondensed-BoldOblique.ttf",
  "/usr/share/fonts/dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraBI.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold_Italic.ttf",
  "/usr/share/fonts/corefonts/arialbi.ttf",
  "/usr/share/fonts/truetype/freefont/FreeSansBoldOblique.ttf",
  "/usr/share/fonts/freefont-ttf/FreeSansBoldOblique.ttf",
#endif
  nullptr
};

static const char *const all_monospace_font_paths[] = {
#ifdef __APPLE__
  "/Library/Fonts/Courier New.ttf",
#elif defined(KOBO)
  "/mnt/onboard/XCSoar/fonts/DejaVuSansMono.ttf",
  "/mnt/onboard/fonts/VeraMono.ttf",
  "/opt/xcsoar/share/fonts/VeraMono.ttf",
#else
  "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf",
  "/usr/share/fonts/truetype/DejaVuSansMono.ttf",
  "/usr/share/fonts/truetype/droid/DroidSansMono.ttf",
  "/usr/share/fonts/truetype/ttf-droid/DroidSansMono.ttf",
  "/usr/share/fonts/TTF/droid/DroidSansMono.ttf",
  "/usr/share/fonts/truetype/DroidSansMono.ttf",
  "/usr/share/fonts/truetype/droid/DroidSansMono.ttf",
  "/usr/share/fonts/truetype/msttcorefonts/couri.ttf",
  "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
  "/usr/share/fonts/TTF/freefont/FreeMono.ttf",
#endif
  NULL
};

gcc_const
static const char *
FindFile(const char *const*list)
{
  for (const char *const* i = list; *i != nullptr; ++i)
    if (File::Exists(*i))
      return *i;

  return nullptr;
}

const char *
FindDefaultFont()
{
  return FindFile(all_font_paths);
}

const char *
FindDefaultBoldFont()
{
  return FindFile(all_bold_font_paths);
}

const char *
FindDefaultItalicFont()
{
  return FindFile(all_italic_font_paths);
}

const char *
FindDefaultBoldItalicFont()
{
  return FindFile(all_bold_italic_font_paths);
}

const char *
FindDefaultMonospaceFont()
{
  return FindFile(all_monospace_font_paths);
}
