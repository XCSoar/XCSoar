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

#include "CoInstance.hpp"
#include "net/http/Mime.hxx"
#include "net/http/CoStreamRequest.hxx"
#include "net/http/Easy.hxx"
#include "net/http/Init.hpp"
#include "net/http/Setup.hxx"
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
