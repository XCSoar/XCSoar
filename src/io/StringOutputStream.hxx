// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#ifndef STRING_OUTPUT_STREAM_HXX
#define STRING_OUTPUT_STREAM_HXX

#include "OutputStream.hxx"

#include <string>

class StringOutputStream final : public OutputStream {
	std::string value;

public:
	const std::string &GetValue() const & noexcept {
		return value;
	}

	std::string &&GetValue() && noexcept {
		return std::move(value);
	}

	/* virtual methods from class OutputStream */
	void Write(const void *data, size_t size) override {
		value.append((const char *)data, size);
	}
};

#endif
