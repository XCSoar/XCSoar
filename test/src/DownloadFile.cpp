/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "net/http/Init.hpp"
#include "net/http/Request.hxx"
#include "net/http/Handler.hxx"
#include "net/http/Init.hpp"
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
  void OnHeaders(unsigned status,
                 std::multimap<std::string, std::string> &&headers) override {
    printf("status: %u\n", status);

    for (const auto &[name, value] : headers)
      printf("%s: %s\n", name.c_str(), value.c_str());

    printf("\n");
  }

  void OnData(ConstBuffer<void> data) override {
    if (file != nullptr)
      fwrite(data.data, 1, data.size, file);
    else
      fwrite(data.data, 1, data.size, stdout);
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
