// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Setup.hxx"
#include "Easy.hxx"
#include "Version.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/CertificateUtil.hpp"
#include "java/Global.hxx"
#include "system/Path.hpp"
#include <mutex>
#endif

#include <stdio.h>

namespace Curl {

#ifdef ANDROID
/* Cache the CA certificate path - extract once per app session */
static std::mutex ca_cert_mutex;
static AllocatedPath ca_cert_path;
static bool ca_cert_initialized = false;

static Path
GetCaCertificatesPath() noexcept
{
  std::lock_guard<std::mutex> lock{ca_cert_mutex};

  if (ca_cert_initialized)
    return ca_cert_path;

  ca_cert_initialized = true;

  if (context == nullptr)
    return nullptr;

  const auto env = Java::GetEnv();
  if (env == nullptr)
    return nullptr;

  ca_cert_path = CertificateUtil::GetSystemCaCertificatesPath(env, *context);
  return ca_cert_path;
}
#endif

void
Setup(CurlEasy &easy)
{
	char user_agent[32];
	snprintf(user_agent, 32, "XCSoar/%s", XCSoar_Version);
	easy.SetUserAgent(user_agent);

#if !defined(ANDROID) && !defined(_WIN32)
	easy.SetOption(CURLOPT_NETRC, 1L);
#endif
	easy.SetNoSignal();
	easy.SetConnectTimeout(10);
	easy.SetOption(CURLOPT_HTTPAUTH, (long) CURLAUTH_ANY);

#ifdef ANDROID
	/* Use Android's system CA certificates for SSL validation */
	const auto ca_path = GetCaCertificatesPath();
	if (ca_path != nullptr) {
		easy.SetOption(CURLOPT_CAINFO, ca_path.c_str());
		easy.SetVerifyHost(true);
		easy.SetVerifyPeer(true);
	}
#elif defined(__APPLE__)
	/* SSL validation disabled on iOS/macOS until we implement
	   certificate extraction similar to Android */
	easy.SetVerifyHost(false);
	easy.SetVerifyPeer(false);
#endif

#ifdef KOBO
	/* no TLS certificate validation because Kobos usually don't
	   have the correct date/time in the real-time clock, which
	   causes the certificate validation to fail */
	easy.SetVerifyPeer(false);
#endif
}

} // namespace Curl
