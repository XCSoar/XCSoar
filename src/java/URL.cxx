// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

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
