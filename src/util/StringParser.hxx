// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "CharUtil.hxx"
#include "NumberParser.hpp"
#include "StringStrip.hxx"

#include <optional>
#include <string_view>

/**
 * Parse a string incrementally.
 */
template<typename T=char>
class StringParser {
	typedef T value_type;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef size_t size_type;

	const_pointer p;

	static constexpr value_type SENTINEL = '\0';

public:
	constexpr explicit StringParser(const_pointer _p):p(_p) {}

	StringParser(const StringParser &) = delete;
	StringParser &operator=(const StringParser &) = delete;

	constexpr const_pointer c_str() const {
		return p;
	}

	value_type front() const {
		return *p;
	}

	value_type pop_front() {
		const auto value = front();
		Skip();
		return value;
	}

	bool IsEmpty() const {
		return front() == SENTINEL;
	}

	void Strip() {
		p = ::StripLeft(p);
	}

	std::optional<unsigned> ReadUnsigned(int base=10) noexcept {
		pointer endptr;
		const auto value = ::ParseUnsigned(p, &endptr, base);
		if (endptr == p)
			return std::nullopt;

		p = endptr;
		return value;
	}

	std::optional<double> ReadDouble() noexcept {
		pointer endptr;
		const auto value = ::ParseDouble(p, &endptr);
		if (endptr == p)
			return std::nullopt;

		p = endptr;
		return value;
	}

	[[gnu::pure]]
	bool MatchAll(const_pointer value) {
		return StringIsEqual(p, value);
	}

	[[gnu::pure]]
	bool MatchAllIgnoreCase(const_pointer value) {
		return StringIsEqualIgnoreCase(p, value);
	}

	[[gnu::pure]]
	bool Match(value_type value) {
		return front() == value;
	}

	[[gnu::pure]]
	bool Match(std::string_view other) {
		return StringIsEqual(p, other.data(), other.size());
	}

	[[gnu::pure]]
	bool MatchIgnoreCase(std::string_view other) {
		return StringIsEqualIgnoreCase(p, other.data(), other.size());
	}

	void Skip(size_t n=1) {
		p += n;
	}

	bool SkipWhitespace() {
		bool match = IsWhitespaceNotNull(front());
		if (match)
			Skip();
		return match;
	}

	bool SkipMatch(value_type value) {
		bool match = Match(value);
		if (match)
			Skip();
		return match;
	}

	bool SkipMatch(std::string_view other) {
		bool match = Match(other);
		if (match)
			Skip(other.size());
		return match;
	}

	bool SkipMatchIgnoreCase(std::string_view other) {
		bool match = MatchIgnoreCase(other);
		if (match)
			Skip(other.size());
		return match;
	}

	/**
	 * Skip until the next whitespace is found.  If no whitespace
	 * is found, return false.  If yes, then that whitespace is
	 * skipped, too.
	 */
	bool SkipWord() {
		while (!IsEmpty()) {
			if (IsWhitespaceFast(pop_front())) {
				Strip();
				return true;
			}
		}

		return false;
	}
};
