// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <string_view>

namespace NOTAMFilter {

[[nodiscard]]
bool IsQCodeHidden(std::string_view qcode,
                   std::string_view hidden_list) noexcept;

[[nodiscard]]
bool ShouldDisplay(const NOTAM &notam,
                   const NOTAMSettings &settings,
                   bool log) noexcept;

} // namespace NOTAMFilter
