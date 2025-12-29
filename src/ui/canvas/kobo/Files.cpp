// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/Files.hpp"
#include "ui/canvas/FontSearch.hpp"

static constexpr const char *const font_search_paths[] = {
  "/mnt/onboard/XCSoar/fonts",
  "/mnt/onboard/fonts",
  "/opt/xcsoar/share/fonts",
  nullptr
};

static constexpr const char *const all_font_paths[] = {
  "DejaVuSansCondensed.ttf",
  "Roboto-Regular.ttf",
  "DroidSans.ttf",
  "Vera.ttf",
  nullptr
};

static constexpr const char *const all_bold_font_paths[] = {
  "DejaVuSansCondensed-Bold.ttf",
  "Roboto-Bold.ttf",
  "DroidSans-Bold.ttf",
  "VeraBd.ttf",
  nullptr
};

static constexpr const char *const all_italic_font_paths[] = {
  "DejaVuSansCondensed-Oblique.ttf",
  "Roboto-Italic.ttf",
  "VeraIt.ttf",
  nullptr
};

static constexpr const char *const all_bold_italic_font_paths[] = {
  "DejaVuSansCondensed-BoldOblique.ttf",
  "Roboto-BoldItalic.ttf",
  "VeraBI.ttf",
  nullptr
};

static constexpr const char *const all_monospace_font_paths[] = {
  "DejaVuSansMono.ttf",
  "DroidSansMono.ttf",
  "VeraMono.ttf",
  nullptr
};

/**
 * Find the default regular font for Kobo.
 * Searches predefined paths for available font files.
 * @return Path to the font file, or nullptr if not found.
 */
[[nodiscard]]
AllocatedPath
FindDefaultFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_font_paths);
}

/**
 * Find the default bold font for Kobo.
 * Searches predefined paths for available font files.
 * @return Path to the font file, or nullptr if not found.
 */
[[nodiscard]]
AllocatedPath
FindDefaultBoldFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_font_paths);
}

/**
 * Find the default italic font for Kobo.
 * Searches predefined paths for available font files.
 * @return Path to the font file, or nullptr if not found.
 */
[[nodiscard]]
AllocatedPath
FindDefaultItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_italic_font_paths);
}

/**
 * Find the default bold-italic font for Kobo.
 * Searches predefined paths for available font files.
 * @return Path to the font file, or nullptr if not found.
 */
[[nodiscard]]
AllocatedPath
FindDefaultBoldItalicFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_bold_italic_font_paths);
}

/**
 * Find the default monospace font for Kobo.
 * Searches predefined paths for available font files.
 * @return Path to the font file, or nullptr if not found.
 */
[[nodiscard]]
AllocatedPath
FindDefaultMonospaceFont() noexcept
{
  return FontSearch::FindFile(font_search_paths, all_monospace_font_paths);
}
