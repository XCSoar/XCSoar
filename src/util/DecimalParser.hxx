// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <optional>
#include <string_view>

/**
 * A simple parser for decimal numbers.  It doesn't support
 * exponential notation, infinity, not a number.
 *
 * We have this only because most std::from_chars implementations are
 * incomplete and don't support floating point numbers.
 */
[[gnu::pure]]
std::optional<double>
ParseDecimal(std::string_view src) noexcept;
