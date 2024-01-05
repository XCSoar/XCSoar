// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <charconv>
#include <optional>
#include <string_view>
#include <type_traits> // for std::is_integral_v

template<typename T>
requires std::is_integral_v<T>
bool
ParseIntegerTo(const char *first, const char *last, T &value, int base=10) noexcept
{
	auto [ptr, ec] = std::from_chars(first, last, value, base);
	return ptr == last && ec == std::errc{};
}

template<typename T>
requires std::is_integral_v<T>
bool
ParseIntegerTo(std::string_view src, T &value, int base=10) noexcept
{
	return ParseIntegerTo(src.data(), src.data() + src.size(), value, base);
}

template<typename T>
requires std::is_integral_v<T>
[[gnu::pure]]
std::optional<T>
ParseInteger(const char *first, const char *last, int base=10) noexcept
{
	T value;
	if (ParseIntegerTo(first, last, value, base))
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
