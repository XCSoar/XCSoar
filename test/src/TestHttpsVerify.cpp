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

struct FetchResults {
  bool internet_available = false;
  bool badssl_ok = false;
  bool wrong_host_ok = false;
  bool xcsoar_ok = false;
};

static Co::Task<FetchResults>
FetchAll(CurlGlobal &curl)
{
  FetchResults results;

  /* Run all fetches in one coroutine; CoInstance's EventLoop cannot be
     restarted after Run() breaks it on completion. */
  results.internet_available =
    co_await FetchOk(curl, "http://http.badssl.com/");

  if (results.internet_available) {
    results.badssl_ok =
      co_await FetchOk(curl, "https://badssl.com/");
    results.wrong_host_ok =
      co_await FetchOk(curl, "https://wrong.host.badssl.com/");
    results.xcsoar_ok =
      co_await FetchOk(curl, "https://download.xcsoar.org/repository");
  }

  co_return results;
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
  plan_tests(3);

  Instance instance;
  const auto results = instance.Run(FetchAll(*Net::curl));

  /* Avoid failing the TLS tests when the test host is simply offline. */
  if (!results.internet_available) {
    skip(3, 1, "internet unavailable");
    return exit_status();
  }

  ok1(results.badssl_ok);
  ok1(!results.wrong_host_ok);
  ok1(results.xcsoar_ok);

  return exit_status();
#endif
}
