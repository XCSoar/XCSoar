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

#include "net/http/Mime.hxx"
#include "net/http/ToStream.hpp"
#include "net/http/Easy.hxx"
#include "net/http/Init.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/StdioOutputStream.hxx"
#include "io/async/AsioThread.hpp"
#include "util/ConstBuffer.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"

#include <stdio.h>

int
main(int argc, char **argv) noexcept
try {
  Args args(argc, argv, "URL NAME PATH");
  const char *url = args.ExpectNext();
  const char *name = args.ExpectNext();
  const char *path = args.ExpectNext();
  args.ExpectEnd();

  AsioThread io_thread;
  io_thread.Start();
  AtScopeExit(&) { io_thread.Stop(); };
  const Net::ScopeInit net_init(io_thread.GetEventLoop());

  CurlEasy easy(url);
  easy.SetFailOnError();

  CurlMime mime(easy.Get());
  mime.Add(name).FileData(path);

  easy.SetMimePost(mime.get());

  ConsoleOperationEnvironment env;
  StdioOutputStream sos(stdout);
  Net::DownloadToStream(*Net::curl, std::move(easy), sos, env);

  printf("\n");
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
