// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

/**
 * Extract the next column from the comma-separated line from a CUP
 * file and advance the #line parameter.
 */
std::string_view
CupNextColumn(std::string_view &line) noexcept;
