// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "File.hxx"
#include "Class.hxx"

jmethodID Java::File::getAbsolutePath_method;

void
Java::File::Initialise(JNIEnv *env) noexcept
{
	Class cls(env, "java/io/File");

	getAbsolutePath_method = env->GetMethodID(cls, "getAbsolutePath",
						  "()Ljava/lang/String;");
}
