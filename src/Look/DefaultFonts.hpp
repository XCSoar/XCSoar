// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct FontSettings;

namespace Fonts {

[[gnu::pure]]
FontSettings
GetDefaults() noexcept;

/**
 * Load all fonts.
 *
 * Throws on error.
 */
void
Initialize();

} // namespace Fonts
