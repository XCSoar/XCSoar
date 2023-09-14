// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "String.hxx"
#include "system/Path.hpp"

class AllocatedPath;

namespace Java {

AllocatedPath
ToPath(JNIEnv *env, jstring s) noexcept;

static inline AllocatedPath
ToPath(const Java::String &o) noexcept
{
	return ToPath(o.GetEnv(), o.Get());
}

static inline AllocatedPath
ToPathChecked(const Java::String &o) noexcept
{
	return o ? ToPath(o) : nullptr;
}

} // namespace Java
