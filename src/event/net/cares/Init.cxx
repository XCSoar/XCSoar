// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Init.hxx"
#include "Error.hxx"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/Context.hpp"
#include "java/Exception.hxx"
#include "java/Global.hxx"
#include "java/Ref.hxx"
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

	assert(context != nullptr);

	ares_library_init_jvm(Java::jvm);

	const auto env = Java::GetEnv();
	const auto obj = context->GetSystemService(env, "connectivity");
	Java::RethrowException(env);

	if (!obj)
		throw std::runtime_error("No ConnectivityManager");

	ares_library_init_android(obj);
#endif
}

Init::~Init() noexcept
{
	ares_library_cleanup();
}

} // namespace Cares
