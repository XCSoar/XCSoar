// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Serialize.hxx"
#include "io/OutputStream.hxx"
#include "util/SpanCast.hxx"

#include <boost/json/serializer.hpp>

class OutputStream;

namespace Json {

void
Serialize(OutputStream &os, const boost::json::value &v)
{
	boost::json::serializer s;
	s.reset(&v);

	while (!s.done()) {
		char buffer[BOOST_JSON_STACK_BUFFER_SIZE];
		auto r = s.read(buffer);
		os.Write(AsBytes(r));
	}
}

} // namespace Json
