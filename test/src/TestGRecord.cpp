// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/GRecord.hpp"
#include "TestUtil.hpp"
#include "system/Path.hpp"
#include "util/PrintException.hxx"

#include <tchar.h>
#include <stdlib.h>

static void
CheckGRecord(const TCHAR *path)
{
  GRecord grecord;
  grecord.Initialize();
  grecord.VerifyGRecordInFile(Path(path));
  ok1(true);
}

int main()
try {
  plan_tests(4);

  CheckGRecord(_T("test/data/grecord64a.igc"));
  CheckGRecord(_T("test/data/grecord64b.igc"));
  CheckGRecord(_T("test/data/grecord65a.igc"));
  CheckGRecord(_T("test/data/grecord65b.igc"));

  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
