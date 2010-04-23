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

#ifndef __GRecord__
#define __GRecord__

#include <tchar.h>
#include "Logger/MD5.hpp"

#define XCSOAR_IGC_CODE "XCS"

class  GRecord {
public:
  enum {
    DIGEST_LENGTH = 4 * MD5::DIGEST_LENGTH + 1,
  };

private:
  MD5 oMD5[4];

  enum {
    BUFF_LEN = 255,
  };

  TCHAR Digest[BUFF_LEN];
  TCHAR DigestRead[BUFF_LEN];
  TCHAR FileName[BUFF_LEN];
  unsigned int ptrDigest;
  unsigned int ptrFileName;

public:

  int Init();
  const TCHAR *GetVersion() const;
  int AppendRecordToBuffer(const TCHAR *szIn);
  int FinalizeBuffer();
  int GetDigest(TCHAR * szOut);
  int IsValidIGCChar(char c);

  // File specific functions
  int SetFileName(const TCHAR *szIn);
  int LoadFileToBuffer();
  int AppendGRecordToFile(bool bValid); // writes error if invalid G Record
  int ReadGRecordFromFile(TCHAR szOutput []);
  bool VerifyGRecordInFile(void);  // returns 0 if false, 1 if true

private:
  int Init(int iKey);
  int AppendStringToBuffer(const unsigned char *szIn);
  int AppendRecordToBuffer(const unsigned char *szIn);
  bool IncludeRecordInGCalc(const unsigned char *szIn);

};
#endif

