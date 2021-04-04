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

#include "URL.hxx"
#include "Object.hxx"
#include "String.hxx"

Java::TrivialClass Java::URL::cls;
jmethodID Java::URL::ctor;
jmethodID Java::URL::openConnection_method;

void
Java::URL::Initialise(JNIEnv *env)
{
	assert(!cls.IsDefined());
	assert(env != nullptr);

	cls.Find(env, "java/net/URL");

	ctor = env->GetMethodID(cls, "<init>", "(Ljava/lang/String;)V");
	assert(ctor != nullptr);

	openConnection_method = env->GetMethodID(cls, "openConnection",
						 "()Ljava/net/URLConnection;");
	assert(openConnection_method != nullptr);
}

void
Java::URL::Deinitialise(JNIEnv *env)
{
	cls.Clear(env);
}

jobject
Java::URL::Create(JNIEnv *env, const char *url)
{
	return Java::URL::Create(env, String(env, url));
}

jobject
Java::URL::openConnection(JNIEnv *env, const char *url)
{
	jobject _url_object = Java::URL::Create(env, url);
	if (env->ExceptionCheck())
		/* pass exception to caller */
		return nullptr;

	LocalObject url_object(env, _url_object);
	return Java::URL::openConnection(env, url_object);
}

jmethodID Java::URLConnection::setConnectTimeout_method;
jmethodID Java::URLConnection::setReadTimeout_method;
jmethodID Java::URLConnection::addRequestProperty_method;
jmethodID Java::URLConnection::getContentLength_method;
jmethodID Java::URLConnection::getInputStream_method;

void
Java::URLConnection::Initialise(JNIEnv *env)
{
	Java::Class cls(env, "java/net/URLConnection");

	setConnectTimeout_method = env->GetMethodID(cls, "setConnectTimeout",
						    "(I)V");
	assert(setConnectTimeout_method != nullptr);

	setReadTimeout_method = env->GetMethodID(cls, "setReadTimeout",
						 "(I)V");
	assert(setReadTimeout_method != nullptr);

	addRequestProperty_method = env->GetMethodID(cls, "addRequestProperty",
						     "(Ljava/lang/String;Ljava/lang/String;)V");
	assert(addRequestProperty_method != nullptr);

	getContentLength_method = env->GetMethodID(cls, "getContentLength",
						   "()I");
	assert(getContentLength_method != nullptr);

	getInputStream_method = env->GetMethodID(cls, "getInputStream",
						 "()Ljava/io/InputStream;");
	assert(getInputStream_method != nullptr);
}
