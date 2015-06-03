/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Util/StringAPI.hpp"
#include "Compiler.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include <algorithm>

static const char *const font_search_paths[] = {
#ifdef __APPLE__
#if TARGET_OS_IPHONE
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
#elif defined(KOBO)
  "DejaVuSansCondensed.ttf",
  "Vera.ttf",
#else
  "ttf-dejavu/DejaVuSansCondensed.ttf",
  "dejavu/DejaVuSansCondensed.ttf",
  "DejaVuSansCondensed.ttf",
  "ttf-droid/DroidSans.ttf",
  "droid/DroidSans.ttf",
  "DroidSans.ttf",
  "msttcorefonts/Arial.ttf",
  "corefonts/arial.ttf",
  "freefont/FreeSans.ttf",
  "freefont-ttf/FreeSans.ttf",
  "unifont/unifont.ttf",
  "corefonts/tahoma.ttf",
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
#elif defined(KOBO)
  "DejaVuSansCondensed-Bold.ttf",
  "VeraBd.ttf",
#elif defined(HAVE_POSIX)
  "ttf-dejavu/DejaVuSansCondensed-Bold.ttf",
  "dejavu/DejaVuSansCondensed-Bold.ttf",
  "DejaVuSansCondensed-Bold.ttf",
  "ttf-droid/DroidSans-Bold.ttf",
  "droid/DroidSans-Bold.ttf",
  "DroidSans-Bold.ttf",
  "msttcorefonts/Arial_Bold.ttf",
  "corefonts/arialbd.ttf",
  "freefont/FreeSansBold.ttf",
  "freefont-ttf/FreeSansBold.ttf",
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
#elif defined(KOBO)
  "DejaVuSansCondensed-Oblique.ttf",
  "VeraIt.ttf",
#elif defined(HAVE_POSIX)
  "ttf-dejavu/DejaVuSansCondensed-Oblique.ttf",
  "dejavu/DejaVuSansCondensed-Oblique.ttf",
  "DejaVuSansCondensed-Oblique.ttf",
  "ttf-bitstream-vera/VeraIt.ttf",
  "msttcorefonts/Arial_Italic.ttf",
  "corefonts/ariali.ttf",
  "freefont/FreeSansOblique.ttf",
  "freefont-ttf/FreeSansOblique.ttf",
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
#elif defined(KOBO)
  "DejaVuSansCondensed-BoldOblique.ttf",
  "VeraBI.ttf",
#elif defined(HAVE_POSIX)
  "ttf-dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "DejaVuSansCondensed-BoldOblique.ttf",
  "ttf-bitstream-vera/VeraBI.ttf",
  "msttcorefonts/Arial_Bold_Italic.ttf",
  "corefonts/arialbi.ttf",
  "freefont/FreeSansBoldOblique.ttf",
  "freefont-ttf/FreeSansBoldOblique.ttf",
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
#elif defined(KOBO)
  "DejaVuSansMono.ttf",
  "VeraMono.ttf",
#else
  "ttf-dejavu/DejaVuSansMono.ttf",
  "dejavu/DejaVuSansMono.ttf",
  "DejaVuSansMono.ttf",
  "ttf-droid/DroidSansMono.ttf",
  "droid/DroidSansMono.ttf",
  "DroidSansMono.ttf",
  "msttcorefonts/couri.ttf",
  "freefont/FreeMono.ttf",
#endif
  nullptr
};

gcc_malloc gcc_nonnull_all
static char *
DupString(const char *src)
{
  const auto size = StringLength(src) + 1;
  auto *dest = new char[size];
  std::copy_n(src, size, dest);
  return dest;
}

gcc_malloc gcc_nonnull_all
static char *
JoinPath(const char *prefix, const char *suffix, size_t suffix_length)
{
  const auto prefix_length = StringLength(prefix);
  auto *dest = new char[prefix_length + 1 + suffix_length + 1];
  auto *p = std::copy_n(prefix, prefix_length, dest);
  *p++ = DIR_SEPARATOR;
  std::copy_n(suffix, suffix_length +1, p);
  return dest;
}

gcc_malloc gcc_nonnull_all
static char *
FindInSearchPaths(const char *suffix)
{
  const auto suffix_length = StringLength(suffix);

  for (const char *const* i = font_search_paths; *i != nullptr; ++i) {
    const char *path = *i;

    auto *full_path = JoinPath(path, suffix, suffix_length);
    if (File::Exists(full_path))
      return full_path;

    delete[] full_path;
  }

  return nullptr;
}

gcc_const
static char *
FindFile(const char *const*list)
{
  for (const char *const* i = list; *i != nullptr; ++i) {
    const char *path = *i;

    if (IsAbsolutePath(path)) {
      if (File::Exists(path))
        return DupString(path);
    } else {
      auto *result = FindInSearchPaths(path);
      if (result != nullptr)
        return result;
    }
  }

  return nullptr;
}

char *
FindDefaultFont()
{
  return FindFile(all_font_paths);
}

char *
FindDefaultBoldFont()
{
  return FindFile(all_bold_font_paths);
}

char *
FindDefaultItalicFont()
{
  return FindFile(all_italic_font_paths);
}

char *
FindDefaultBoldItalicFont()
{
  return FindFile(all_bold_italic_font_paths);
}

char *
FindDefaultMonospaceFont()
{
  return FindFile(all_monospace_font_paths);
}
