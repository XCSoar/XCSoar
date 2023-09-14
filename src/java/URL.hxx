// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Class.hxx"

#include <jni.h>
#include <cassert>

namespace Java {

/**
 * Wrapper for a java.net.URL object.
 */
class URL {
	static TrivialClass cls;
	static jmethodID ctor, openConnection_method;

public:
	static void Initialise(JNIEnv *env);
	static void Deinitialise(JNIEnv *env);

	static jobject Create(JNIEnv *env, jstring url) {
		assert(env != nullptr);
		assert(url != nullptr);
		assert(ctor != nullptr);

		return env->NewObject(cls, ctor, url);
	}

	static jobject Create(JNIEnv *env, const char *url);

	static jobject openConnection(JNIEnv *env, jobject url) {
		assert(env != nullptr);
		assert(url != nullptr);
		assert(openConnection_method != nullptr);

		return env->CallObjectMethod(url, openConnection_method);
	}

	static jobject openConnection(JNIEnv *env, const char *url);
};

/**
 * Wrapper for a java.net.URLConnection object.
 */
class URLConnection {
	static jmethodID setConnectTimeout_method;
	static jmethodID setReadTimeout_method;
	static jmethodID addRequestProperty_method;
	static jmethodID getContentLength_method;
	static jmethodID getInputStream_method;

public:
	static void Initialise(JNIEnv *env);

	static void setConnectTimeout(JNIEnv *env, jobject connection,
				      jint timeout) {
		assert(env != nullptr);
		assert(connection != nullptr);
		assert(setConnectTimeout_method != nullptr);

		env->CallVoidMethod(connection,
				    setConnectTimeout_method, timeout);
	}

	static void setReadTimeout(JNIEnv *env, jobject connection,
				   jint timeout) {
		assert(env != nullptr);
		assert(connection != nullptr);
		assert(setReadTimeout_method != nullptr);

		env->CallVoidMethod(connection, setReadTimeout_method,
				    timeout);
	}

	static void addRequestProperty(JNIEnv *env, jobject connection,
				       jstring field, jstring value) {
		assert(env != nullptr);
		assert(connection != nullptr);
		assert(setReadTimeout_method != nullptr);

		env->CallVoidMethod(connection,
				    addRequestProperty_method,
				    field, value);
	}

	static int getContentLength(JNIEnv *env, jobject connection) {
		assert(env != nullptr);
		assert(connection != nullptr);
		assert(getContentLength_method != nullptr);

		return env->CallIntMethod(connection,
					  getContentLength_method);
	}

	static jobject getInputStream(JNIEnv *env, jobject connection) {
		assert(env != nullptr);
		assert(connection != nullptr);
		assert(getInputStream_method != nullptr);

		return env->CallObjectMethod(connection,
					     getInputStream_method);
	}
};

} // namespace Java
