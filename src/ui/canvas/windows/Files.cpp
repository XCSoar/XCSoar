// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/Files.hpp"
#include "ui/canvas/FontSearch.hpp"

#include <shlobj.h>
#include <windef.h> // for MAX_PATH
#include <string>

/**
 * Get the Windows fonts directory using the Shell API.
 * This handles custom font locations and different Windows installations.
 */
static const char *
GetWindowsFontsPath() noexcept
{
  static std::string fonts_path;

  if (fonts_path.empty()) {
    char buffer[MAX_PATH];
    if (SHGetSpecialFolderPathA(nullptr, buffer, CSIDL_FONTS, FALSE)) {
      fonts_path = buffer;
    } else {
      // Fallback to typical location if API fails
      fonts_path = "C:\\Windows\\Fonts";
    }
  }

  return fonts_path.c_str();
}

static const char *font_search_paths_storage[5];

static const char *const *
GetFontSearchPaths() noexcept
{
  static bool initialized = false;

  if (!initialized) {
    font_search_paths_storage[0] = GetWindowsFontsPath();
    // Include Linux paths as fallback (for Wine compatibility)
    font_search_paths_storage[1] = "/usr/share/fonts/truetype";
    font_search_paths_storage[2] = "/usr/share/fonts/TTF";
    font_search_paths_storage[3] = "/usr/share/fonts";
    font_search_paths_storage[4] = nullptr;
    initialized = true;
  }

  return font_search_paths_storage;
}

static constexpr const char *const all_font_paths[] = {
  // Windows native fonts (directly in C:\Windows\Fonts)
  "arial.ttf",
  "Arial.ttf",
  "tahoma.ttf",
  "Tahoma.ttf",
  "segoeui.ttf",
  "verdana.ttf",
  "Verdana.ttf",
  // Linux fonts as fallback (for Wine)
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

static constexpr const char *const all_bold_font_paths[] = {
  // Windows native bold fonts
  "arialbd.ttf",
  "Arialbd.ttf",
  "tahomabd.ttf",
  "Tahomabd.ttf",
  "segoeuib.ttf",
  "verdanab.ttf",
  "Verdanab.ttf",
  // Linux fonts as fallback (for Wine)
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

static constexpr const char *const all_italic_font_paths[] = {
  // Windows native italic fonts
  "ariali.ttf",
  "Ariali.ttf",
  "segoeuii.ttf",
  "verdanai.ttf",
  "Verdanai.ttf",
  // Linux fonts as fallback (for Wine)
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

static constexpr const char *const all_bold_italic_font_paths[] = {
  // Windows native bold-italic fonts
  "arialbi.ttf",
  "Arialbi.ttf",
  "segoeuiz.ttf",
  "verdanaz.ttf",
  "Verdanaz.ttf",
  // Linux fonts as fallback (for Wine)
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

static constexpr const char *const all_monospace_font_paths[] = {
  // Windows native monospace fonts
  "cour.ttf",
  "Cour.ttf",
  "consola.ttf",
  "Consola.ttf",
  "lucon.ttf",
  "Lucon.ttf",
  // Linux fonts as fallback (for Wine)
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

[[nodiscard]]
AllocatedPath
FindDefaultFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), all_font_paths);
}

[[nodiscard]]
AllocatedPath
FindDefaultBoldFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), all_bold_font_paths);
}

[[nodiscard]]
AllocatedPath
FindDefaultItalicFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), all_italic_font_paths);
}

[[nodiscard]]
AllocatedPath
FindDefaultBoldItalicFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), all_bold_italic_font_paths);
}

[[nodiscard]]
AllocatedPath
FindDefaultMonospaceFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), all_monospace_font_paths);
}
