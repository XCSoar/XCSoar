// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* This creates the VALI-XCS.exe program
 * This is a DOS program that is used by the OLC or a scorer in a contest
 * to validate the GRecord of an XCSoar-generated IGC file
 */

#include "system/ConvertPathName.hpp"
#include "Logger/GRecord.hpp"
#include "Version.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

enum STATUS_t {
  eValidationPassed,
  eValidationFileNotFound,
  eValidationFileRead,
};

static const char szPass[] = "Validation check passed, data indicated as correct";
static const char szFail[] = "Validation check failed.  G Record is invalid";
static const char szNoFile[] = "Validation check failed.  File not found";

static STATUS_t
ValidateXCS(Path path, GRecord &oGRecord)
{
  STATUS_t eStatus = eValidationFileNotFound;

  FILE *inFile = nullptr;
  inFile = _tfopen(path.c_str(), _T("r"));
  if (inFile == nullptr)
    return eStatus;

  fclose(inFile);

  oGRecord.Initialize();
  oGRecord.VerifyGRecordInFile(path);
  return eValidationPassed;
}

static int
RunValidate(Path path)
{
  GRecord oGRecord;
  STATUS_t eStatus = ValidateXCS(path, oGRecord);
  switch (eStatus) {
  case eValidationPassed:
    puts(szPass);
    return 0; // success

  case eValidationFileNotFound:
    puts(szNoFile);
    return 1;

  default:
    puts(szFail);
    return 1;
  }
}

int main(int argc, char* argv[])
try {
  printf("Vali XCS for the XCSoar Flight Computer Version %s\n",
         XCSoar_Version);

  if (argc > 1 && strcmp(argv[1], "-?") != 0) {
    PathName path(argv[1]);
    return RunValidate(path);
  } else
    return 0;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
