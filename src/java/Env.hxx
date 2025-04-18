// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Exception.hxx"
#include "Ref.hxx"

namespace Java {

template<typename... Args>
LocalObject
NewObjectRethrow(JNIEnv *env, jclass cls, jmethodID method, Args... args)
{
	LocalObject result{env, env->NewObject(cls, method, args...)};
	RethrowException(env);
	return result;
}

template<typename... Args>
LocalObject
CallStaticObjectMethodRethrow(JNIEnv *env, jclass cls, jmethodID method,
			      Args... args)
{
	LocalObject result{env,
		env->CallStaticObjectMethod(cls, method, args...)};
	RethrowException(env);
	return result;
}

template<typename... Args>
LocalObject
CallObjectMethodRethrow(JNIEnv *env, jobject obj, jmethodID method,
			Args... args)
{
	LocalObject result{env, env->CallObjectMethod(obj, method, args...)};
	RethrowException(env);
	return result;
}

} // namespace Java
