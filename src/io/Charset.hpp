// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A source character set for #ConvertLineReader.
 */
enum class Charset {
  /**
   * Attempt to determine automatically.  Read UTF-8, but switch to
   * ISO-Latin-1 as soon as the first invalid UTF-8 sequence is
   * seen.
   */
  AUTO,
  UTF8,
  ISO_LATIN_1,
};
