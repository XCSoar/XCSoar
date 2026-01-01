// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/Files.hpp"
#include "ui/canvas/FontSearch.hpp"
#include "Asset.hpp"

static constexpr const char *const ios_font_search_paths[] = {
  "/System/Library/Fonts/Core",
  "/System/Library/Fonts/Cache",
  nullptr
};

static constexpr const char *const mac_font_search_paths[] = {
  "/Library/Fonts",
  "/Library/Fonts/Microsoft",

  /* found on macOS Catalina */
  "/System/Library/Fonts/Supplemental",
  nullptr
};

static constexpr const char *const ios_all_font_paths[] = {
  "Arial.ttf",
  "Georgia.ttf",
  "TimesNewRoman.ttf",
  nullptr
};

static constexpr const char *const mac_all_font_paths[] = {
  "Tahoma.ttf",
  "Georgia.ttf",
  "Arial Narrow.ttf",
  "Times New Roman.ttf",
  "Arial.ttf",
  nullptr
};

static constexpr const char *const ios_all_bold_font_paths[] = {
  "ArialBold.ttf",
  "GeorgiaBold.ttf",
  "TimesNewRomanBold.ttf",
  nullptr
};

static constexpr const char *const mac_all_bold_font_paths[] = {
  "Tahoma Bold.ttf",
  "Georgia Bold.ttf",
  "Arial Narrow Bold.ttf",
  "Arial Bold.ttf",
  nullptr
};

static constexpr const char *const ios_all_italic_font_paths[] = {
  "ArialItalic.ttf",
  "GeorgiaItalic.ttf",
  "TimesNewRomanItalic.ttf",
  nullptr
};

static constexpr const char *const mac_all_italic_font_paths[] = {
  "Arial Italic.ttf",
  "Georgia Italic.ttf",
  "Arial Narrow Italic.ttf",
  nullptr
};

static constexpr const char *const ios_all_bold_italic_font_paths[] = {
  "ArialBoldItalic.ttf",
  "GeorgiaBoldItalic.ttf",
  "TimesNewRomanBoldItalic.ttf",
  nullptr
};

static constexpr const char *const mac_all_bold_italic_font_paths[] = {
  "Arial Bold Italic.ttf",
  "Georgia Bold Italic.ttf",
  "Arial Narrow Bold Italic.ttf",
  nullptr
};

static constexpr const char *const ios_all_monospace_font_paths[] = {
  "CourierNew.ttf",
  nullptr
};

static constexpr const char *const mac_all_monospace_font_paths[] = {
  "Courier New.ttf",
  nullptr
};

[[gnu::pure]]
static const char *const*
GetFontSearchPaths() noexcept
{
  return IsIOS()
    ? ios_font_search_paths
    : mac_font_search_paths;
}

[[gnu::pure]]
static const char *const*
GetAllFontPaths() noexcept
{
  return IsIOS()
    ? ios_all_font_paths
    : mac_all_font_paths;
}

[[gnu::pure]]
static const char *const*
GetAllBoldFontPaths() noexcept
{
  return IsIOS()
    ? ios_all_bold_font_paths
    : mac_all_bold_font_paths;
}

[[gnu::pure]]
static const char *const*
GetAllItalicFontPaths() noexcept
{
  return IsIOS()
    ? ios_all_italic_font_paths
    : mac_all_italic_font_paths;
}

[[gnu::pure]]
static const char *const*
GetAllBoldItalicFontPaths() noexcept
{
  return IsIOS()
    ? ios_all_bold_italic_font_paths
    : mac_all_bold_italic_font_paths;
}

[[gnu::pure]]
static const char *const*
GetAllMonospaceFontPaths() noexcept
{
  return IsIOS()
    ? ios_all_monospace_font_paths
    : mac_all_monospace_font_paths;
}

[[nodiscard]]
AllocatedPath
FindDefaultFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), GetAllFontPaths());
}

[[nodiscard]]
AllocatedPath
FindDefaultBoldFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), GetAllBoldFontPaths());
}

[[nodiscard]]
AllocatedPath
FindDefaultItalicFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), GetAllItalicFontPaths());
}

[[nodiscard]]
AllocatedPath
FindDefaultBoldItalicFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), GetAllBoldItalicFontPaths());
}

[[nodiscard]]
AllocatedPath
FindDefaultMonospaceFont() noexcept
{
  return FontSearch::FindFile(GetFontSearchPaths(), GetAllMonospaceFontPaths());
}
