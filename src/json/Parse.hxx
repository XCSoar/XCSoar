// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <boost/json/fwd.hpp>

class Reader;

namespace Json {

boost::json::value
Parse(Reader &r);

} // namespace Json
