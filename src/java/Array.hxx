/*
 * Copyright 2021 Max Kellermann <max.kellermann@gmail.com>
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

#include <jni.h>

#include <utility>

namespace Java {

class ByteArrayElements {
	JNIEnv *env;
	jbyteArray array;
	jbyte *elems = nullptr;

public:
	ByteArrayElements() noexcept = default;

	ByteArrayElements(JNIEnv *_env, jbyteArray _array) noexcept
		:env(_env), array(_array),
		 elems(env->GetByteArrayElements(array, nullptr)) {}

	ByteArrayElements(ByteArrayElements &&src) noexcept
		:env(src.env), array(src.array),
		 elems(std::exchange(src.elems, nullptr)) {}

	~ByteArrayElements() noexcept {
		if (elems != nullptr)
			env->ReleaseByteArrayElements(array, elems, 0);
	}

	ByteArrayElements &operator=(ByteArrayElements &&src) noexcept {
		using std::swap;
		swap(env, src.env);
		swap(array, src.array);
		swap(elems, src.elems);
		return *this;
	}

	jbyte *get() const noexcept {
		return elems;
	}

	operator jbyte *() const noexcept {
		return elems;
	}
};

class IntArrayElements {
	JNIEnv *env;
	jintArray array;
	jint *elems = nullptr;

public:
	IntArrayElements() noexcept = default;

	IntArrayElements(JNIEnv *_env, jintArray _array) noexcept
		:env(_env), array(_array),
		 elems(env->GetIntArrayElements(array, nullptr)) {}

	IntArrayElements(IntArrayElements &&src) noexcept
		:env(src.env), array(src.array),
		 elems(std::exchange(src.elems, nullptr)) {}

	~IntArrayElements() noexcept {
		if (elems != nullptr)
			env->ReleaseIntArrayElements(array, elems, 0);
	}

	IntArrayElements &operator=(IntArrayElements &&src) noexcept {
		using std::swap;
		swap(env, src.env);
		swap(array, src.array);
		swap(elems, src.elems);
		return *this;
	}

	jint *get() const noexcept {
		return elems;
	}

	operator jint *() const noexcept {
		return elems;
	}
};

} // namespace Java
