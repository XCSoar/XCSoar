/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

  M Roberts (original release)
  Robin Birch <robinb@ruffnready.co.uk>
  Samuel Gisiger <samuel.gisiger@triadis.ch>
  Jeff Goodenough <jeff@enborne.f2s.com>
  Alastair Harrison <aharrison@magic.force9.co.uk>
  Scott Penrose <scottp@dd.com.au>
  John Wharington <jwharington@gmail.com>
  Lars H <lars_hn@hotmail.com>
  Rob Dunning <rob@raspberryridgesheepfarm.com>
  Russell King <rmk@arm.linux.org.uk>
  Paolo Ventafridda <coolwind@email.it>
  Tobias Lohner <tobias@lohner-net.de>
  Mirek Jezek <mjezek@ipplc.cz>
  Max Kellermann <max@duempel.org>
  Tobias Bieniek <tobias.bieniek@gmx.de>

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

/* to build:
 * $ g++ -o VALI-XCS.exe VALI-XCS.cpp ../../Logger/LoggerGRecord.cpp ../../Logger/MD5.cpp -I../../Logger  -I../../
 */

/* This creates the VALI-XCS.exe program
 * This is a DOS program that is used by the OLC or a scorer in a contest
 * to validate the GRecord of an XCSoar-generated IGC file
 */

#include "Logger/LoggerGRecord.hpp"

#include <stdio.h>
#include <string.h>
#include <tchar.h>

typedef enum {
  eValidationFailed,
  eValidationPassed,
  eValidationFileNotFound,
  eValidationFileRead,
} STATUS_t;

char szPass[] = "Validation check passed, data indicated as correct\r\n";
char szFail[] = "Validation check failed.  G Record is invalid\r\n";
char szNoFile[] = "Validation check failed.  File not found\r\n";
char szInfo[] = "Vali XCS for the XCSoar Flight Computer Version 1.0.2\r\n";

STATUS_t
ValidateXCS(char *FileName, GRecord &oGRecord)
{
  STATUS_t eStatus = eStatus=eValidationFileNotFound;

  FILE *inFile = NULL;
  inFile = fopen(FileName, ("r"));
  if (inFile == NULL)
    return eStatus;

  fclose(inFile);

  eStatus = eValidationFailed;

  oGRecord.Init();
  oGRecord.SetFileName(FileName);
  if (oGRecord.VerifyGRecordInFile())
    eStatus = eValidationPassed;

  return eStatus;
}

int main(int argc, char* argv[])
{
  GRecord oGRecord;

  int iRetVal;
  STATUS_t eStatus;
  iRetVal = 0; //false
  eStatus = eValidationFailed;

  printf(szInfo);
  if (argc > 1 && strcmp(argv[1], "-?") != 0) {
    eStatus = ValidateXCS((char*)argv[1], oGRecord);
    switch (eStatus) {
    case eValidationFailed:
      printf(szFail);
      break;

    case eValidationPassed:
      iRetVal = 1; // success
      printf(szPass);
      break;

    case eValidationFileNotFound:
      printf(szNoFile);
      break;

    default:
      printf(szFail);
      break;
    }

  }

  return iRetVal;
}
