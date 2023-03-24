// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "net/http/Init.hpp"
#include "lib/curl/Mime.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "system/Args.hpp"
#include "io/StdioOutputStream.hxx"
#include "util/PrintException.hxx"

#include <stdio.h>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static Co::InvokeTask
Run(CurlGlobal &curl, const char *url, const char *name, const char *path,
    OutputStream &os)
{
  CurlEasy easy(url);
  Curl::Setup(easy);
  easy.SetFailOnError();

  CurlMime mime(easy.Get());
  mime.Add(name).FileData(path);
  easy.SetMimePost(mime.get());

  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), os);
}

int
main(int argc, char **argv) noexcept
try {
  Args args(argc, argv, "URL NAME PATH");
  const char *url = args.ExpectNext();
  const char *name = args.ExpectNext();
  const char *path = args.ExpectNext();
  args.ExpectEnd();

  Instance instance;
  StdioOutputStream sos(stdout);
  instance.Run(Run(*Net::curl, url, name, path, sos));

  printf("\n");
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
