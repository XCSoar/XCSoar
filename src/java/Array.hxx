// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

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
