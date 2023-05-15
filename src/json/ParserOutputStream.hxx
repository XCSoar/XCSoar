// SPDX-License-Identifier: BSD-2-Clause
// Copyright Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "io/OutputStream.hxx"

#include <boost/json.hpp>

namespace Json {

/**
 * An #OutputStream implementation which parses all incoming data as
 * JSON.  Call Finish() to obtain the JSON value.
 */
class ParserOutputStream final : public OutputStream {
	boost::json::stream_parser parser;

public:
	decltype(auto) Finish() {
		return parser.release();
	}

	/* virtual methods from class OutputStream */
	void Write(std::span<const std::byte> src) override;
};

} // namespace Json
