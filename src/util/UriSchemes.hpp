// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringCompare.hxx"

#include <cstddef>

/**
 * URI scheme descriptor for URL detection.
 */
struct UriScheme {
  const char *prefix;
  std::size_t length;
  bool is_external;  ///< true for schemes opened via OpenLink(), false for internal
};

/**
 * Supported URI schemes for URL detection.
 * Listed longest-first for schemes with common prefixes (e.g., facetime-audio
 * before facetime).
 */
static constexpr UriScheme kUriSchemes[] = {
  {"whatsapp://", 11, true},
  {"facetime-audio:", 15, true},
  {"xcsoar://", 9, false},
  {"facetime:", 9, true},
  {"https://", 8, true},
  {"file://", 7, true},
  {"sgnl://", 7, true},
  {"mailto:", 7, true},
  {"http://", 7, true},
  {"skype:", 6, true},
  {"tg://", 5, true},
  {"sms:", 4, true},
  {"tel:", 4, true},
  {"geo:", 4, true},
};

/**
 * Check if string starts with a known URI scheme.
 * @return Pointer to matching scheme, or nullptr if no match
 */
[[gnu::pure]]
static inline const UriScheme *
FindUriScheme(const char *str) noexcept
{
  for (const auto &scheme : kUriSchemes) {
    if (StringStartsWith(str, scheme.prefix))
      return &scheme;
  }
  return nullptr;
}

/**
 * Check if URL is an external scheme that can be opened with OpenLink().
 * This includes web URLs (http/https), email (mailto), phone (tel),
 * messaging apps (sms, whatsapp, signal, telegram), geo locations,
 * video calls (skype, facetime), and local files (file://).
 *
 * Does NOT include xcsoar:// internal links.
 */
[[gnu::pure]]
static inline bool
IsExternalUriScheme(const char *url) noexcept
{
  const UriScheme *scheme = FindUriScheme(url);
  return scheme != nullptr && scheme->is_external;
}
