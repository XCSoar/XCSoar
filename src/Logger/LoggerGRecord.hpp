/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef GRECORD_HPP
#define GRECORD_HPP

#include "Compiler.h"

#include <tchar.h>
#include "Logger/MD5.hpp"

#define XCSOAR_IGC_CODE "XCS"

class GRecord
{
public:
  enum {
    DIGEST_LENGTH = 4 * MD5::DIGEST_LENGTH + 1,
  };

private:
  MD5 md5[4];

  enum {
    BUFF_LEN = 255,
  };

  TCHAR filename[BUFF_LEN];

public:

  void Initialize();
  const TCHAR *GetVersion() const;

  /**
   * @return returns true if record is appended, false if skipped
   */
  bool AppendRecordToBuffer(const char *szIn);
  void FinalizeBuffer();
  void GetDigest(char *buffer);

  gcc_const
  static bool IsValidIGCChar(char c) {
    return MD5::IsValidIGCChar(c);
  }

  // File specific functions
  void SetFileName(const TCHAR *szIn);
  /** loads a file into the data buffer */
  bool LoadFileToBuffer();
  /// writes error if invalid G Record
  bool AppendGRecordToFile(bool bValid);
  /**
   * returns in szOutput the G Record from the file referenced by
   * filename member
   */
  bool ReadGRecordFromFile(char *buffer, size_t max_length);
  /// returns 0 if false, 1 if true
  bool VerifyGRecordInFile(void);

private:
  void Initialize(int iKey);
  void AppendStringToBuffer(const unsigned char *szIn);
  /**
   * returns false if record is not to be included in
   * G record calc (see IGC specs)
   */
  bool IncludeRecordInGCalc(const unsigned char *szIn);
};
#endif

