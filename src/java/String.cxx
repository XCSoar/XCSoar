/*
 * Copyright 2010-2021 Max Kellermann <max.kellermann@gmail.com>
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

#include "String.hxx"
#include "util/TruncateString.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringView.hxx"

#include <string>

Java::String::String(JNIEnv *_env, StringView _value) noexcept
	// TODO: is there no way to do this without duplicating the string?
	:String(_env, std::string(_value.data, _value.size).c_str())
{
}

char *
Java::String::CopyTo(JNIEnv *env, jstring value,
		     char *buffer, size_t max_size) noexcept
{
	const char *p = env->GetStringUTFChars(value, nullptr);
	if (p == nullptr)
		return nullptr;

	char *result = CopyTruncateString(buffer, max_size, p);
	env->ReleaseStringUTFChars(value, p);
	return result;
}

std::string
Java::String::ToString(JNIEnv *env, jstring s) noexcept
{
	assert(env != nullptr);
	assert(s != nullptr);

	const char *p = env->GetStringUTFChars(s, nullptr);
	if (p == nullptr)
		return std::string();

	AtScopeExit(env, s, p) {
		env->ReleaseStringUTFChars(s, p);
	};

	return std::string(p);
}
