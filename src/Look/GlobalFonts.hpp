// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct FontSettings;
class Font;

/**
 * Container for global font variables.  Avoid using it if you can,
 * use the "Look" objects instead.
 */
namespace Fonts {

extern Font map;
extern Font map_bold;

/**
 * Throws on error.
 */
void
Load(const FontSettings &settings);

void
Deinitialize() noexcept;

} // namespace Fonts
