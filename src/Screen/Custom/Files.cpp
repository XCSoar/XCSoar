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

#include "Files.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Path.hpp"
#include "Compiler.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

static const char *const font_search_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "/System/Library/Fonts/Core",
  "/System/Library/Fonts/Cache",
#else
  "/Library/Fonts",
  "/Library/Fonts/Microsoft",
#endif
#elif defined(WIN32) && !defined(HAVE_POSIX)
  /* just for the experimental WINSDL target */
  "c:\\windows\\fonts",
#elif defined(KOBO)
  "/mnt/onboard/XCSoar/fonts",
  "/mnt/onboard/fonts",
  "/opt/xcsoar/share/fonts",
#else
  "/usr/share/fonts/truetype",
  "/usr/share/fonts/TTF",
  "/usr/share/fonts",
#endif
  nullptr
};

static const char *const all_font_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "Arial.ttf",
  "Georgia.ttf",
  "TimesNewRoman.ttf",
#else
  "Tahoma.ttf",
  "Georgia.ttf",
  "Arial Narrow.ttf",
  "Times New Roman.ttf",
  "Arial.ttf",
#endif
#elif defined(WIN32) && !defined(HAVE_POSIX)
  /* just for the experimental WINSDL target */
  "arial.ttf",
#else

#ifndef KOBO
  "ttf-dejavu/DejaVuSansCondensed.ttf",
  "dejavu/DejaVuSansCondensed.ttf",
#endif
  "DejaVuSansCondensed.ttf",

#ifndef KOBO
  "roboto/Roboto-Regular.ttf",
#endif
  "Roboto-Regular.ttf",

#ifndef KOBO
  "ttf-droid/DroidSans.ttf",
  "droid/DroidSans.ttf",
#endif
  "DroidSans.ttf",

#ifndef KOBO
  "ttf-bitstream-vera/Vera.ttf",
#endif
  "Vera.ttf",

#ifndef KOBO
  "msttcorefonts/Arial.ttf",
  "corefonts/arial.ttf",
  "freefont/FreeSans.ttf",
  "freefont-ttf/FreeSans.ttf",
  "unifont/unifont.ttf",
  "corefonts/tahoma.ttf",
#endif

#endif

  nullptr
};

static const char *const all_bold_font_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "ArialBold.ttf",
  "GeorgiaBold.ttf",
  "TimesNewRomanBold.ttf",
#else
  "Tahoma Bold.ttf",
  "Georgia Bold.ttf",
  "Arial Narrow Bold.ttf",
  "Arial Bold.ttf",
#endif
#else

#ifndef KOBO
  "ttf-dejavu/DejaVuSansCondensed-Bold.ttf",
  "dejavu/DejaVuSansCondensed-Bold.ttf",
#endif
  "DejaVuSansCondensed-Bold.ttf",

#ifndef KOBO
  "roboto/Roboto-Bold.ttf",
#endif
  "Roboto-Bold.ttf",

#ifndef KOBO
  "ttf-droid/DroidSans-Bold.ttf",
  "droid/DroidSans-Bold.ttf",
#endif
  "DroidSans-Bold.ttf",

#ifndef KOBO
  "ttf-bitstream-vera/VeraBd.ttf",
#endif
  "VeraBd.ttf",

#ifndef KOBO
  "msttcorefonts/Arial_Bold.ttf",
  "corefonts/arialbd.ttf",
  "freefont/FreeSansBold.ttf",
  "freefont-ttf/FreeSansBold.ttf",
#endif

#endif
  nullptr
};

static const char *const all_italic_font_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "ArialItalic.ttf",
  "GeorgiaItalic.ttf",
  "TimesNewRomanItalic.ttf",
#else
  "Arial Italic.ttf",
  "Georgia Italic.ttf",
  "Arial Narrow Italic.ttf",
#endif
#else

#ifndef KOBO
  "ttf-dejavu/DejaVuSansCondensed-Oblique.ttf",
  "dejavu/DejaVuSansCondensed-Oblique.ttf",
#endif
  "DejaVuSansCondensed-Oblique.ttf",

#ifndef KOBO
  "roboto/Roboto-Italic.ttf",
#endif
  "Roboto-Italic.ttf",

#ifndef KOBO
  "ttf-bitstream-vera/VeraIt.ttf",
#endif
  "VeraIt.ttf",

#ifndef KOBO
  "msttcorefonts/Arial_Italic.ttf",
  "corefonts/ariali.ttf",
  "freefont/FreeSansOblique.ttf",
  "freefont-ttf/FreeSansOblique.ttf",
#endif

#endif
  nullptr
};

static const char *const all_bold_italic_font_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "ArialBoldItalic.ttf",
  "GeorgiaBoldItalic.ttf",
  "TimesNewRomanBoldItalic.ttf",
#else
  "Arial Bold Italic.ttf",
  "Georgia Bold Italic.ttf",
  "Arial Narrow Bold Italic.ttf",
#endif
#else

#ifndef KOBO
  "ttf-dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "dejavu/DejaVuSansCondensed-BoldOblique.ttf",
#endif
  "DejaVuSansCondensed-BoldOblique.ttf",

#ifndef KOBO
  "roboto/Roboto-BoldItalic.ttf",
#endif
  "Roboto-BoldItalic.ttf",

#ifndef KOBO
  "ttf-bitstream-vera/VeraBI.ttf",
#endif
  "VeraBI.ttf",

#ifndef KOBO
  "msttcorefonts/Arial_Bold_Italic.ttf",
  "corefonts/arialbi.ttf",
  "freefont/FreeSansBoldOblique.ttf",
  "freefont-ttf/FreeSansBoldOblique.ttf",
#endif

#endif
  nullptr
};

static const char *const all_monospace_font_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
  "CourierNew.ttf",
#else
  "Courier New.ttf",
#endif
#else

#ifndef KOBO
  "ttf-dejavu/DejaVuSansMono.ttf",
  "dejavu/DejaVuSansMono.ttf",
#endif
  "DejaVuSansMono.ttf",

#ifndef KOBO
  "ttf-droid/DroidSansMono.ttf",
  "droid/DroidSansMono.ttf",
#endif
  "DroidSansMono.ttf",

#ifndef KOBO
  "ttf-bitstream-vera/VeraMono.ttf",
#endif
  "VeraMono.ttf",

#ifndef KOBO
  "msttcorefonts/couri.ttf",
  "freefont/FreeMono.ttf",
#endif

#endif
  nullptr
};

gcc_nonnull_all
static AllocatedPath
FindInSearchPaths(Path suffix)
{
  for (const char *const* i = font_search_paths; *i != nullptr; ++i) {
    const Path path(*i);

    auto full_path = AllocatedPath::Build(path, suffix);
    if (File::Exists(full_path))
      return full_path;
  }

  return nullptr;
}

gcc_const
static AllocatedPath
FindFile(const char *const*list)
{
  for (const char *const* i = list; *i != nullptr; ++i) {
    const Path path(*i);

    if (path.IsAbsolute()) {
      if (File::Exists(path))
        return path;
    } else {
      auto result = FindInSearchPaths(path);
      if (result != nullptr)
        return result;
    }
  }

  return nullptr;
}

AllocatedPath
FindDefaultFont()
{
  return FindFile(all_font_paths);
}

AllocatedPath
FindDefaultBoldFont()
{
  return FindFile(all_bold_font_paths);
}

AllocatedPath
FindDefaultItalicFont()
{
  return FindFile(all_italic_font_paths);
}

AllocatedPath
FindDefaultBoldItalicFont()
{
  return FindFile(all_bold_italic_font_paths);
}

AllocatedPath
FindDefaultMonospaceFont()
{
  return FindFile(all_monospace_font_paths);
}
