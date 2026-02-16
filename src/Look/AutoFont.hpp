// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class FontDescription;

/**
 * Change the font size so that the text fits into the given width.
 *
 * Throws on error.
 */
void
AutoSizeFont(FontDescription &d, unsigned width, const char *text);
