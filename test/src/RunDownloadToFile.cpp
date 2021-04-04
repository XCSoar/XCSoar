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

#include "net/http/ToFile.hpp"
#include "net/http/Init.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/AsioThread.hpp"
#include "util/ConstBuffer.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"

#include <stdio.h>

static void
HexPrint(ConstBuffer<void> _b) noexcept
{
  const auto b = ConstBuffer<uint8_t>::FromVoid(_b);
  for (uint8_t i : b)
    printf("%02x", i);
}

int
main(int argc, char **argv) noexcept
try {
  Args args(argc, argv, "URL PATH");
  const char *url = args.ExpectNext();
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  std::array<std::byte, 32> hash;

  AsioThread io_thread;
  io_thread.Start();
  AtScopeExit(&) { io_thread.Stop(); };
  const Net::ScopeInit net_init(io_thread.GetEventLoop());

  ConsoleOperationEnvironment env;
  Net::DownloadToFile(*Net::curl, url, path, &hash, env);

  HexPrint({&hash, sizeof(hash)});
  printf("\n");
  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
