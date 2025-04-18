// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/FileReader.hxx"
#include "system/Args.hpp"
#include "util/MD5.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>

static void
Feed(Reader &r, MD5 &state)
{
  while (true) {
    std::byte buffer[65536];
    size_t nbytes = r.Read(buffer);
    if (nbytes == 0)
      break;

    state.Append(std::span{buffer}.first(nbytes));
  }
}

static void
Feed(Path path, MD5 &state)
{
  FileReader r(path);
  Feed(r, state);
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  MD5 md5;
  md5.Initialise();

  Feed(path, md5);

  md5.Finalize();

  char digest[MD5::DIGEST_LENGTH + 1];
  md5.GetDigest(digest);

  puts(digest);
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
