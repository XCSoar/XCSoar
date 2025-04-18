// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "net/http/Init.hpp"
#include "lib/curl/Request.hxx"
#include "lib/curl/Handler.hxx"
#include "io/async/AsioThread.hpp"
#include "system/ConvertPathName.hpp"
#include "thread/AsyncWaiter.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"

#include <exception>
#include <iostream>
#include <stdio.h>

#include <tchar.h>

using namespace std;

class MyResponseHandler final : public CurlResponseHandler {
  FILE *const file;

  AsyncWaiter waiter;

public:
  explicit MyResponseHandler(FILE *_file):file(_file) {}

  void Wait() noexcept {
    waiter.Wait();
  }

  /* virtual methods from class CurlResponseHandler */
  void OnHeaders(unsigned status, Curl::Headers &&headers) override {
    printf("status: %u\n", status);

    for (const auto &[name, value] : headers)
      printf("%s: %s\n", name.c_str(), value.c_str());

    printf("\n");
  }

  void OnData(std::span<const std::byte> data) override {
    if (file != nullptr)
      fwrite(data.data(), 1, data.size(), file);
    else
      fwrite(data.data(), 1, data.size(), stdout);
  }

  void OnEnd() override {
    waiter.SetDone();
  }

  void OnError(std::exception_ptr e) noexcept override {
    waiter.SetError(std::move(e));
  }
};

static void
Download(CurlGlobal &curl, const char *url, Path path)
{
  FILE *file = path != nullptr ? _tfopen(path.c_str(), _T("wb")) : nullptr;
  MyResponseHandler handler(file);
  CurlRequest request(curl, url, handler);

  request.StartIndirect();
  handler.Wait();

  if (file != NULL)
    fclose(file);
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <url> [<filename>]" << endl;
    cout << "   <url> is the absolute url of the resource you are requesting" << endl << endl;
    cout << "   <filename> is the path where the requested file should be saved (optional)" << endl << endl;
    cout << "   Example: " << argv[0] << " http://www.domain.com/docs/readme.htm readme.htm" << endl;
    return 1;
  }

  try {
    AsioThread io_thread;
    io_thread.Start();
    AtScopeExit(&) { io_thread.Stop(); };
    const Net::ScopeInit net_init(io_thread.GetEventLoop());

    const char *url = argv[1];
    Download(*Net::curl, url, argc > 2 ? (Path)PathName(argv[2]) : nullptr);
  } catch (const std::exception &exception) {
    PrintException(exception);
    return EXIT_FAILURE;
  }

  return 0;
}
