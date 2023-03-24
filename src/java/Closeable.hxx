// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Object.hxx"

namespace Java {

void
InitialiseCloseable(JNIEnv *env) noexcept;

void
CloseCloseable(JNIEnv *env, jobject obj) noexcept;

/**
 * Wrapper for a java.io.Closeable object.
 */
class GlobalCloseable : public GlobalObject {
public:
	using GlobalObject::GlobalObject;
	~GlobalCloseable() noexcept;
};

/**
 * Wrapper for a java.io.Closeable object.
 */
class LocalCloseable : public LocalObject {
public:
	using LocalObject::LocalObject;
	LocalCloseable(LocalObject &&src) noexcept
		:LocalObject(std::move(src)) {}
	~LocalCloseable() noexcept;
};

} // namespace Java
