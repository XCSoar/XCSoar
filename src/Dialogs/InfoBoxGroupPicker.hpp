// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Group.hpp"

#include <cstddef>
#include <span>

/**
 * Let the user tick the InfoBox groups the content lists shall show
 * - a checkbox list like the multi-file picker.
 *
 * @return true if the user confirmed (the mask may still be the same)
 */
bool
InfoBoxGroupPicker(InfoBoxFactory::GroupMask &mask) noexcept;

/**
 * Describe a mask for a caption: "All" when nothing is filtered,
 * otherwise the names of the ticked groups separated by commas
 * (translated).  Never empty - an empty mask reads "None".
 */
const char *
FormatInfoBoxGroups(InfoBoxFactory::GroupMask mask,
                    std::span<char> buffer) noexcept;
