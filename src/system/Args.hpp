// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"
#include "util/tstring.hpp"
#include "util/NumberParser.hpp"
#include "system/Path.hpp"

#ifdef _UNICODE
#include "system/ConvertPathName.hpp"
#else
#include <tchar.h>
#endif

#include <list>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cassert>
#include <iterator>

#ifdef MORE_USAGE
extern void PrintMoreUsage();
#endif

class Args {
  std::list<char *> args;
  const char *name, *usage;

#ifdef _WIN32
  char *cmdline;
#endif

public:
  Args(int argc, char **argv, const char *_usage)
    :name(argv[0]), usage(_usage) {
    assert(name != nullptr);
    assert(usage != nullptr);

    std::copy(argv + 1, argv + argc, std::back_inserter(args));

#ifdef _WIN32
    cmdline = nullptr;
#endif
  }

  Args(const Args &other) = delete;

  Args(Args &&other):name(other.name), usage(other.usage) {
    std::swap(args, other.args);
#ifdef _WIN32
    std::swap(cmdline, other.cmdline);
#endif
  }

#ifdef _WIN32
  Args(const TCHAR *_cmdline, const char *_usage)
    :usage(_usage) {
    ParseCommandLine(_cmdline);
  }

  ~Args() {
    delete[] cmdline;
  }

  void ParseCommandLine(const char *_cmdline) {
    const char *s = _cmdline;
    cmdline = new char[strlen(s) + 1];
    char *d = cmdline;                 // current position in destination buffer
    char *option = cmdline;

    name = nullptr;
    bool in_qoute = false;
    do {
      if (*s == '"')
        in_qoute = !in_qoute;
      else if (*s == '\0' || (!in_qoute && *s == ' ')) {
        // collapse runs of unqouted ' 's to a single '\0'
        if (d > cmdline && *(d-1) != '\0') {
          *d++ = '\0';
          // remember potential start position of next option
          option = d;
        }
      } else {
        *d = *s;
        if (option == d) {
          // first quoted blank or non blank character of new option
          // program name is not included in command line on CE
          if (name == nullptr)
            name = option;
          else
            args.push_back(option);
        }
        d++;
      }
    } while (*s++);

    if (name == nullptr)
      name = "";
  }

#ifdef _UNICODE
  void ParseCommandLine(const TCHAR *_cmdline) {
    WideToACPConverter convert(_cmdline);
    ParseCommandLine(convert);
  }
#endif
#endif

  Args &operator=(const Args &other) = delete;

  [[noreturn]]
  void UsageError() {
    fprintf(stderr, "Usage: %s %s\n", name, usage);
#ifdef MORE_USAGE
    PrintMoreUsage();
#endif
    exit(EXIT_FAILURE);
  }

  bool IsEmpty() const {
    return args.empty();
  }

  const char *GetNext() {
    assert(!IsEmpty());

    const char *p = args.front();
    Skip();
    return p;
  }

  void Skip() {
    args.pop_front();
  }

  const char *PeekNext() const {
    return IsEmpty() ? nullptr : args.front();
  }

  const char *ExpectNext() {
    if (IsEmpty())
      UsageError();

    return GetNext();
  }

  int ExpectNextInt() {
    const char *p = ExpectNext();
    assert(p != nullptr);

    char *endptr;
    int result = ParseInt(p, &endptr);
    if (p == endptr)
      UsageError();

    return result;
  }

  double ExpectNextDouble() {
    const char *p = ExpectNext();
    assert(p != nullptr);

    char *endptr;
    double result = ParseDouble(p, &endptr);
    if (p == endptr)
      UsageError();

    return result;
  }

  tstring ExpectNextT() {
    const char *p = ExpectNext();
    assert(p != nullptr);

#ifdef _UNICODE
    PathName convert(p);
    return tstring(((Path)convert).c_str());
#else
    return tstring(p);
#endif
  }

#ifdef _UNICODE
  AllocatedPath ExpectNextPath() {
    const char *p = ExpectNext();
    assert(p != nullptr);

    return AllocatedPath(PathName(p));
  }
#else
  Path ExpectNextPath() {
    const char *p = ExpectNext();
    assert(p != nullptr);

    return Path(p);
  }
#endif

  void ExpectEnd() {
    if (!IsEmpty())
      UsageError();
  }
};
