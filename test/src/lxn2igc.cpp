// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* Convert LXN files to IGC */

#include "Device/Driver/LX/Convert.hpp"
#include "system/Args.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/StdioOutputStream.hxx"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

static const long MAX_LXN_SIZE = 1024 * 1024;

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.lxn");
  const char *lxn_path = args.ExpectNext();
  args.ExpectEnd();

  FILE *file = fopen(lxn_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", lxn_path);
    return EXIT_FAILURE;
  }

  long size;
  if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) <= 0 ||
      fseek(file, 0, SEEK_SET) != 0 || size > MAX_LXN_SIZE)  {
    fprintf(stderr, "Failed to seek file %s\n", lxn_path);
    fclose(file);
    return EXIT_FAILURE;
  }

  void *data = malloc(size);
  size_t n = fread(data, 1, size, file);
  fclose(file);
  if (n != (size_t)size) {
    free(data);
    fprintf(stderr, "Failed to read from file %s\n", lxn_path);
    return EXIT_FAILURE;
  }

  StdioOutputStream sos(stdout);
  BufferedOutputStream bos(sos);

  bool success = LX::ConvertLXNToIGC(data, n, bos);
  free(data);

  bos.Flush();

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
