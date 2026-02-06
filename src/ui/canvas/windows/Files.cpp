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

static const char *font_search_paths_storage[4];

static const char *const *
GetFontSearchPaths() noexcept
{
  static bool initialized = false;

  if (!initialized) {
    font_search_paths_storage[0] = GetWindowsFontsPath();
    // Include Linux paths as fallback for WSL compatibility
    font_search_paths_storage[1] = "/usr/share/fonts/truetype";
    font_search_paths_storage[2] = "/usr/share/fonts";
    font_search_paths_storage[3] = nullptr;
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
  nullptr
};

static constexpr const char *const all_italic_font_paths[] = {
  // Windows native italic fonts
  "ariali.ttf",
  "Ariali.ttf",
  "segoeuii.ttf",
  "verdanai.ttf",
  "Verdanai.ttf",
  nullptr
};

static constexpr const char *const all_bold_italic_font_paths[] = {
  // Windows native bold-italic fonts
  "arialbi.ttf",
  "Arialbi.ttf",
  "segoeuiz.ttf",
  "verdanaz.ttf",
  "Verdanaz.ttf",
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
