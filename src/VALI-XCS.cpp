/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/* This creates the VALI-XCS.exe program
 * This is a DOS program that is used by the OLC or a scorer in a contest
 * to validate the GRecord of an XCSoar-generated IGC file
 */

#include "OS/ConvertPathName.hpp"
#include "Logger/GRecord.hpp"
#include "Version.hpp"

#include <stdio.h>
#include <string.h>
#include <tchar.h>

#ifdef WIN32
#include <windows.h>
#endif

enum STATUS_t {
  eValidationFailed,
  eValidationPassed,
  eValidationFileNotFound,
  eValidationFileRead,
};

static const char szPass[] = "Validation check passed, data indicated as correct";
static const char szFail[] = "Validation check failed.  G Record is invalid";
static const char szNoFile[] = "Validation check failed.  File not found";

static STATUS_t
ValidateXCS(const TCHAR *FileName, GRecord &oGRecord)
{
  STATUS_t eStatus = eValidationFileNotFound;

  FILE *inFile = NULL;
  inFile = _tfopen(FileName, _T("r"));
  if (inFile == NULL)
    return eStatus;

  fclose(inFile);

  eStatus = eValidationFailed;

  oGRecord.Initialize();
  if (oGRecord.VerifyGRecordInFile(FileName))
    eStatus = eValidationPassed;

  return eStatus;
}

static int
RunValidate(const TCHAR *path)
{
  GRecord oGRecord;
  STATUS_t eStatus = ValidateXCS(path, oGRecord);
  switch (eStatus) {
  case eValidationFailed:
    puts(szFail);
    return 0;

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
{
  printf("Vali XCS for the XCSoar Flight Computer Version "
#ifdef _UNICODE
         "%S\n",
#else
         "%s\n",
#endif
         XCSoar_Version);

  if (argc > 1 && strcmp(argv[1], "-?") != 0) {
    PathName path(argv[1]);
    return RunValidate(path);
  } else
    return 0;
}
