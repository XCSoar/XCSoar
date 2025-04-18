// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef _UNICODE
#include <tchar.h>
#endif

#include <cstddef>

class RGB8Color;

/**
 * Formats a Color struct into a hex-based RGB string, i.e. "#123456"
 */
void
FormatHexColor(char *buffer, size_t size, const RGB8Color color);

bool
ParseHexColor(const char *buffer, RGB8Color &color);

#ifdef _UNICODE

bool
ParseHexColor(const TCHAR *buffer, RGB8Color &color);

#endif
