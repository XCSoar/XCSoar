// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

/**
 * Translated English msgid for an ISO 3166-1 alpha-2 area code,
 * or nullptr if the code is empty or unknown.  Pass the result
 * through gettext() before showing it.
 */
[[gnu::pure]]
const char *
GetCountryName(std::string_view area) noexcept;
