// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "lib/sodium/SHA256.hxx"
#include "system/Args.hpp"
#include "io/FileReader.hxx"
#include "util/PrintException.hxx"

#include <stdio.h>

static void
Feed(Reader &r, SHA256State &state)
{
  while (true) {
    std::byte buffer[65536];
    size_t nbytes = r.Read(buffer);
    if (nbytes == 0)
      break;

    state.Update(std::span{buffer}.first(nbytes));
  }
}

static void
Feed(Path path, SHA256State &state)
{
  FileReader r(path);
  Feed(r, state);
}

static void
HexPrint(std::span<const std::byte> b) noexcept
{
  for (const std::byte i : b)
    printf("%02x", (unsigned)i);
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  SHA256State state;
  Feed(path, state);

  HexPrint(state.Final());
  printf("\n");

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
