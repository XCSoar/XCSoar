// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Object.hxx"
#include "String.hxx"

#include <jni.h>

namespace Java {

/**
 * Wrapper for a java.io.File object.
 */
class File : public LocalObject {
	static jmethodID getAbsolutePath_method;

public:
	using LocalObject::LocalObject;

	[[gnu::nonnull]]
	static void Initialise(JNIEnv *env) noexcept;

	[[gnu::nonnull]]
	static jstring GetAbsolutePath(JNIEnv *env, jobject file) noexcept {
		return (jstring)env->CallObjectMethod(file,
						      getAbsolutePath_method);
	}

	String GetAbsolutePath() const noexcept {
		return {GetEnv(), GetAbsolutePath(GetEnv(), Get())};
	}

	String GetAbsolutePathChecked() const noexcept {
		return *this ? GetAbsolutePath() : nullptr;
	}
};

} // namespace Java
