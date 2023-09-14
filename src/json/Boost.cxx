// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

/* suppress -Wundef */
#define BOOST_VERSION 0

#include <boost/json/src.hpp>
