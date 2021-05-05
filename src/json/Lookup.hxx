/*
 * Copyright 2007-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
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

#pragma once

#include <boost/json.hpp>

#include <cstddef>
#include <string_view>

namespace Json {

[[gnu::pure]]
static inline const auto *
Lookup(const boost::json::object &o, std::string_view key) noexcept
{
	return o.if_contains(key);
}

[[gnu::pure]]
static inline const auto *
Lookup(const boost::json::value &v, std::string_view key) noexcept
{
	const auto *o = v.if_object();
	return o != nullptr
		? Lookup(*o, key)
		: nullptr;
}

[[gnu::pure]]
static inline const auto *
Lookup(const boost::json::array &a, std::size_t i) noexcept
{
	return a.if_contains(i);
}

[[gnu::pure]]
static inline const auto *
Lookup(const boost::json::value &v, std::size_t i) noexcept
{
	const auto *a = v.if_array();
	return a != nullptr
		? Lookup(*a, i)
		: nullptr;
}

template<typename J, typename K, typename... Args>
[[gnu::pure]]
static inline const auto *
Lookup(const J &j, K &&key, Args&&... args)
{
	const auto *l = Lookup(j, std::forward<K>(key));
	return l != nullptr
		? Lookup(*l, std::forward<Args>(args)...)
		: nullptr;
}

template<typename... Args>
[[gnu::pure]]
static inline const auto *
LookupObject(Args&&... args)
{
	const auto *o = Lookup(std::forward<Args>(args)...);
	return o != nullptr
		? o->if_object()
		: nullptr;
}

} // namespace Json
