// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Path.hxx"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <cassert>

namespace Java {

AllocatedPath
ToPath(JNIEnv *env, jstring s) noexcept
{
	assert(env != nullptr);
	assert(s != nullptr);

	const auto c = Java::String::GetUTFChars(env, s);
	return Path(c.c_str());
}

} // namespace Java
