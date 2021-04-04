/*
 * Copyright 2017-2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
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

#include "Init.hxx"
#include "Error.hxx"

#ifdef ANDROID
#include "java/Global.hxx"
#include "java/Class.hxx"
#include "java/Object.hxx"
#endif

#include <ares.h>

#include <cassert>

namespace Cares {

Init::Init()
{
	int code = ares_library_init(ARES_LIB_INIT_ALL);
	if (code != 0)
		throw Error(code, "ares_library_init() failed");

#ifdef ANDROID
	/* c-ares needs special initialization on Android; see
	   https://c-ares.haxx.se/ares_library_init_android.html for
	   details */

	ares_library_init_jvm(Java::jvm);
	const auto env = Java::GetEnv();
	Java::Class net_util(env, "org/xcsoar/NetUtil");

	const auto getConnectivityManager =
		env->GetStaticMethodID(net_util, "getConnectivityManager",
				       "()Landroid/net/ConnectivityManager;");
	assert(getConnectivityManager);

	Java::LocalObject obj(env, env->CallStaticObjectMethod(net_util,
							       getConnectivityManager));
	ares_library_init_android(obj);
#endif
}

Init::~Init() noexcept
{
	ares_library_cleanup();
}

} // namespace Cares
