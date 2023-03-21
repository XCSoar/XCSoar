// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Setup.hxx"
#include "Easy.hxx"
#include "util/ConvertString.hpp"
#include "Version.hpp"

#include <stdio.h>

namespace Curl {

void
Setup(CurlEasy &easy)
{
	char user_agent[32];
	snprintf(user_agent, 32, "XCSoar/%s",
		 (const char *)WideToUTF8Converter(XCSoar_Version));
	easy.SetUserAgent(user_agent);

#if !defined(ANDROID) && !defined(_WIN32)
	easy.SetOption(CURLOPT_NETRC, 1L);
#endif
	easy.SetNoSignal();
	easy.SetConnectTimeout(10);
	easy.SetOption(CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);

#ifdef ANDROID
	/* this is disabled until we figure out how to use Android's
	   CA certificates with libcurl */
	easy.SetVerifyHost(false);
	easy.SetVerifyPeer(false);
#endif
}

} // namespace Curl
