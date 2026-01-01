// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "TestUtil.hpp"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "net/http/Init.hpp"

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

/**
 * Test whether an HTTPS URL can be successfully fetched.
 * @return true if fetch succeeds with 2xx status, false on error or cert failure
 */
static Co::Task<bool>
FetchOk(CurlGlobal &curl, const char *url)
{
  try {
    CurlEasy easy{url};
    Curl::Setup(easy);
    easy.SetFailOnError();
    const auto response = co_await Curl::CoRequest(curl, std::move(easy));
    co_return response.status >= 200 && response.status < 300;
  } catch (...) {
    co_return false;
  }
}

int
main()
{
#if !defined(HAVE_HTTP)
  plan_skip_all("HTTP disabled");
  return exit_status();
#elif defined(ANDROID) || defined(KOBO)
  plan_skip_all("TLS verification disabled on this platform");
  return exit_status();
#else
  plan_tests(2);

  Instance instance;

  ok1(instance.Run(FetchOk(*Net::curl,
                           "https://download.xcsoar.org/repository")));
  ok1(!instance.Run(FetchOk(*Net::curl, "https://wrong.host.badssl.com/")));

  return exit_status();
#endif
}
