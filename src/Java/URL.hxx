/*
 * Copyright (C) 2010-2015 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef JAVA_URL_HXX
#define JAVA_URL_HXX

#include "Class.hxx"

#include <jni.h>
#include <assert.h>

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
}

#endif
