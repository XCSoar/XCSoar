// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Repository/Parser.hpp"
#include "Repository/FileRepository.hpp"
#include "io/FileLineReader.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PATH");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  FileLineReaderA reader(path);

  FileRepository repository;

  if (!ParseFileRepository(repository, reader)) {
    fprintf(stderr, "Failed to parse file\n");
    return EXIT_FAILURE;
  }

  for (auto i = repository.begin(), end = repository.end(); i != end; ++i) {
    const auto &file = *i;
    printf("file '%s' '%s' area='%s' type=%u\n",
           file.GetName(), file.GetURI(), file.GetArea(),
           (unsigned)file.type);

    if (file.update_date.IsPlausible())
      printf("  updated %04u-%02u-%02u\n", file.update_date.year,
             file.update_date.month, file.update_date.day);
  }

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
