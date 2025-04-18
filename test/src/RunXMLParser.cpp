// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XML/Node.hpp"
#include "XML/Parser.hpp"
#include "io/StdioOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  const auto node = XML::ParseFile(path);

  StdioOutputStream out(stdout);
  BufferedOutputStream bos(out);
  node.Serialise(bos, true);
  bos.Flush();

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
