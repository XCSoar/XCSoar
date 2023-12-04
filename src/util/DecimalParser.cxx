// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "DecimalParser.hxx"

#include <charconv> // std::from_chars()
#include <cstdint>

#include <math.h> // for pow()

std::optional<double>
ParseDecimal(std::string_view src) noexcept
{
#ifdef __MSVC__
  // MSVC cannot (change) the iterator to const char* automatically?
  const char *first = src.data(), *const last = first + src.size();
#else
  const char *first = src.begin(), *const last = first + src.size();
#endif
	if (first == last)
		return std::nullopt;

	/* parse (and strip) the sign */

	bool negative = false;
	if (*first == '-') {
		negative = true;
		++first;
	} else if (*first == '+') {
		++first;
	}

	if (first == last)
		return std::nullopt;

	/* parse the integer part */

	uint_least64_t integer_part = 0;

	if (*first != '.') {
		auto [ptr, ec] = std::from_chars(first, last, integer_part, 10);
		if (ec != std::errc{})
			return std::nullopt;

		first = ptr;
	}

	double value = static_cast<double>(integer_part);

	/* parse the fractional part */

	if (first != last) {
		if (*first != '.')
			return std::nullopt;

		++first;

		if (first != last) {
			/* we parse the fractional part as an integer
			   yand then divide by 10^n_digits */
			uint_least64_t fractional_part;

			auto [ptr, ec] = std::from_chars(first, last, fractional_part, 10);
			if (ptr != last || ec != std::errc{})
				return std::nullopt;

			int fractional_length = last - first;

			value += fractional_part * pow(10, -fractional_length);
		}
	}

	if (negative)
		value = -value;

	return value;
}
