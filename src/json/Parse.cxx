// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Parse.hxx"
#include "io/Reader.hxx"

#include <boost/json/stream_parser.hpp>

namespace Json {

boost::json::value
Parse(Reader &r)
{
	boost::json::stream_parser p;

	while (true) {
		/* reserve one more byte to work around assertion
		   failure because
		   boost::json::basic_parser_impl::sentinel() can
		   return a non-unique pointer to the end of the
		   buffer which causes a crash due to assertion
		   failure; see
		   https://github.com/boostorg/json/pull/814 for the
		   proposed upstream fix */
		char buffer[BOOST_JSON_STACK_BUFFER_SIZE + 1];

		const std::size_t nbytes = r.Read(std::as_writable_bytes(std::span{buffer}.first(BOOST_JSON_STACK_BUFFER_SIZE)));
		if (nbytes == 0)
			break;

		p.write(buffer, nbytes);
	}

	return p.release();
}

} // namespace Json
