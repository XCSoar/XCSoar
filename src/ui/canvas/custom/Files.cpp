// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Files.hpp"
#include "ui/canvas/FontSearch.hpp"

static const char *const font_search_paths[] = {
  "/usr/share/fonts/truetype",
  "/usr/share/fonts/TTF",
  "/usr/share/fonts",
  nullptr
};

static const char *const all_font_paths[] = {
  "ttf-dejavu/DejaVuSansCondensed.ttf",
  "dejavu/DejaVuSansCondensed.ttf",
  "DejaVuSansCondensed.ttf",
  "roboto/Roboto-Regular.ttf",
  "Roboto-Regular.ttf",
  "ttf-droid/DroidSans.ttf",
  "droid/DroidSans.ttf",
  "DroidSans.ttf",
  "ttf-bitstream-vera/Vera.ttf",
  "Vera.ttf",
  "msttcorefonts/Arial.ttf",
  "corefonts/arial.ttf",
  "freefont/FreeSans.ttf",
  "freefont-ttf/FreeSans.ttf",
  "unifont/unifont.ttf",
  "corefonts/tahoma.ttf",
  nullptr
};

static const char *const all_bold_font_paths[] = {
  "ttf-dejavu/DejaVuSansCondensed-Bold.ttf",
  "dejavu/DejaVuSansCondensed-Bold.ttf",
  "DejaVuSansCondensed-Bold.ttf",
  "roboto/Roboto-Bold.ttf",
  "Roboto-Bold.ttf",
  "ttf-droid/DroidSans-Bold.ttf",
  "droid/DroidSans-Bold.ttf",
  "DroidSans-Bold.ttf",
  "ttf-bitstream-vera/VeraBd.ttf",
  "VeraBd.ttf",
  "msttcorefonts/Arial_Bold.ttf",
  "corefonts/arialbd.ttf",
  "freefont/FreeSansBold.ttf",
  "freefont-ttf/FreeSansBold.ttf",
  nullptr
};

static const char *const all_italic_font_paths[] = {
  "ttf-dejavu/DejaVuSansCondensed-Oblique.ttf",
  "dejavu/DejaVuSansCondensed-Oblique.ttf",
  "DejaVuSansCondensed-Oblique.ttf",
  "roboto/Roboto-Italic.ttf",
  "Roboto-Italic.ttf",
  "ttf-bitstream-vera/VeraIt.ttf",
  "VeraIt.ttf",
  "msttcorefonts/Arial_Italic.ttf",
  "corefonts/ariali.ttf",
  "freefont/FreeSansOblique.ttf",
  "freefont-ttf/FreeSansOblique.ttf",
  nullptr
};

static const char *const all_bold_italic_font_paths[] = {
  "ttf-dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "dejavu/DejaVuSansCondensed-BoldOblique.ttf",
  "DejaVuSansCondensed-BoldOblique.ttf",
  "roboto/Roboto-BoldItalic.ttf",
  "Roboto-BoldItalic.ttf",
  "ttf-bitstream-vera/VeraBI.ttf",
  "VeraBI.ttf",
  "msttcorefonts/Arial_Bold_Italic.ttf",
  "corefonts/arialbi.ttf",
  "freefont/FreeSansBoldOblique.ttf",
  "freefont-ttf/FreeSansBoldOblique.ttf",
  nullptr
};

static const char *const all_monospace_font_paths[] = {
  "ttf-dejavu/DejaVuSansMono.ttf",
  "dejavu/DejaVuSansMono.ttf",
  "DejaVuSansMono.ttf",
  "ttf-droid/DroidSansMono.ttf",
  "droid/DroidSansMono.ttf",
  "DroidSansMono.ttf",
  "ttf-bitstream-vera/VeraMono.ttf",
  "VeraMono.ttf",
  "msttcorefonts/couri.ttf",
  "freefont/FreeMono.ttf",
  nullptr
};

AllocatedPath
FindDefaultFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_font_paths);
}

AllocatedPath
FindDefaultBoldFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_font_paths);
}

AllocatedPath
FindDefaultItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_italic_font_paths);
}

AllocatedPath
FindDefaultBoldItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_italic_font_paths);
}

AllocatedPath
FindDefaultMonospaceFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_monospace_font_paths);
}
