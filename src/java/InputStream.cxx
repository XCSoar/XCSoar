// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "InputStream.hxx"
#include "Class.hxx"

jmethodID Java::InputStream::close_method;
jmethodID Java::InputStream::read_method;

void
Java::InputStream::Initialise(JNIEnv *env)
{
	Class cls(env, "java/io/InputStream");

	close_method = env->GetMethodID(cls, "close", "()V");
	assert(close_method != nullptr);

	read_method = env->GetMethodID(cls, "read", "([B)I");
	assert(read_method != nullptr);
}
