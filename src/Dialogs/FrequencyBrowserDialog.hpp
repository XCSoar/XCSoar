// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <memory>

class Widget;

/**
 * Which radio action appears first in the button row (tab order / default focus).
 */
enum class FrequencyBrowserPrimary : uint8_t {
  StandbyFirst,
  ActiveFirst,
};

/**
 * Tabbed frequency browser: nearest sources, favorites file, manual entry.
 * @param initial_tab optional: "nearest", "favorites", or "manual" (case-sensitive); nullptr defaults to nearest.
 * @param primary from active vs standby InfoBox: puts that action's button first.
 */
void
ShowFrequencyBrowserDialog(const char *initial_tab,
                           FrequencyBrowserPrimary primary =
                             FrequencyBrowserPrimary::StandbyFirst) noexcept;

std::unique_ptr<Widget>
LoadFrequencyBrowserPanel(unsigned id) noexcept;
