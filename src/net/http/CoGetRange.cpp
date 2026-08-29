// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoGetRange.hpp"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Setup.hxx"

#include <fmt/format.h>

#include <cassert>
#include <stdexcept>

namespace Net {

Co::Task<std::string>
CoGetRange(CurlGlobal &curl, const char *url,
           uint_least64_t offset, std::size_t length)
{
  assert(url != nullptr);
  assert(length > 0);

  CurlEasy easy{url};
  Curl::Setup(easy);
  easy.SetOption(CURLOPT_FOLLOWLOCATION, 1L);
  easy.SetOption(CURLOPT_MAXREDIRS, 10L);
  easy.SetFailOnError();
  easy.SetTimeout(30);

  const auto range = fmt::format("{}-{}", offset, offset + length - 1);
  easy.SetOption(CURLOPT_RANGE, range.c_str());

  /* a server that does not understand the range would answer with the
     whole file; refuse it up front rather than let one tile pull
     megabytes over a flight connection */
  easy.SetOption(CURLOPT_MAXFILESIZE_LARGE, curl_off_t(length + 4096));

  auto response = co_await Curl::CoRequest(curl, std::move(easy));

  /* 206 is the only answer that means "here is the part you asked
     for"; a 200 is the whole file and must not be mistaken for it */
  if (response.status != 206)
    throw std::runtime_error("The server does not serve byte ranges");

  if (response.body.size() > length)
    throw std::runtime_error("The server sent more than the range");

  co_return std::move(response.body);
}

} // namespace Net
