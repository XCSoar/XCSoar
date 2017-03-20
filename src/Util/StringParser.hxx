/*
 * Copyright (C) 2015 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STRING_PARSER_HXX
#define STRING_PARSER_HXX

#include "CharUtil.hpp"
#include "StringUtil.hpp"
#include "NumberParser.hpp"

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

	bool ReadUnsigned(unsigned &value_r, int base=10) {
		pointer endptr;
		const auto value = ::ParseUnsigned(p, &endptr, base);
		if (endptr == p)
			return false;

		value_r = value;
		p = endptr;
		return true;
	}

	bool ReadDouble(double &value_r) {
		pointer endptr;
		const auto value = ::ParseDouble(p, &endptr);
		if (endptr == p)
			return false;

		value_r = value;
		p = endptr;
		return true;
	}

	gcc_pure
	bool MatchAll(const_pointer value) {
		return StringIsEqual(p, value);
	}

	gcc_pure
	bool MatchAllIgnoreCase(const_pointer value) {
		return StringIsEqualIgnoreCase(p, value);
	}

	gcc_pure
	bool Match(value_type value) {
		return front() == value;
	}

	gcc_pure
	bool Match(const_pointer value, size_t size) {
		return StringIsEqual(p, value, size);
	}

	gcc_pure
	bool MatchIgnoreCase(const_pointer value, size_t size) {
		return StringIsEqualIgnoreCase(p, value, size);
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

	bool SkipMatch(const_pointer value, size_t size) {
		bool match = Match(value, size);
		if (match)
			Skip(size);
		return match;
	}

	bool SkipMatchIgnoreCase(const_pointer value, size_t size) {
		bool match = MatchIgnoreCase(value, size);
		if (match)
			Skip(size);
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

#endif
