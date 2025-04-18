// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <jni.h>
#include <cassert>
#include <cstddef>

namespace Java {

/**
 * Wrapper for a java.io.InputStream object.
 */
class InputStream {
	static jmethodID close_method, read_method;

public:
	static void Initialise(JNIEnv *env);

	static void close(JNIEnv *env, jobject is) {
		assert(env != nullptr);
		assert(is != nullptr);
		assert(close_method != nullptr);

		env->CallVoidMethod(is, close_method);
	}

	static int read(JNIEnv *env, jobject is, jbyteArray buffer) {
		assert(env != nullptr);
		assert(is != nullptr);
		assert(buffer != nullptr);
		assert(read_method != nullptr);

		return env->CallIntMethod(is, read_method, buffer);
	}
};

} // namespace Java
