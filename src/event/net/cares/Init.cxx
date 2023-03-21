// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

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
