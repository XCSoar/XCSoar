// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <charconv>
#include <optional>
#include <string_view>
#include <type_traits> // for std::is_integral_v

template<typename T>
requires std::is_integral_v<T>
[[gnu::pure]]
std::optional<T>
ParseInteger(const char *first, const char *last, int base=10) noexcept
{
	T value;
	auto [ptr, ec] = std::from_chars(first, last, value, base);
	if (ptr == last && ec == std::errc{})
		return value;
	else
		return std::nullopt;
}

template<typename T>
requires std::is_integral_v<T>
[[gnu::pure]]
std::optional<T>
ParseInteger(std::string_view src, int base=10) noexcept
{
	return ParseInteger<T>(src.data(), src.data() + src.size(), base);
}
