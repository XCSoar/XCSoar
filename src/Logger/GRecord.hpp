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

#ifndef GRECORD_HPP
#define GRECORD_HPP

#include "Compiler.h"

#include <tchar.h>
#include "Logger/MD5.hpp"

#define XCSOAR_IGC_CODE "XCS"

class TextWriter;

class GRecord
{
public:
  static constexpr size_t DIGEST_LENGTH = 4 * MD5::DIGEST_LENGTH;

private:
  MD5 md5[4];

  /**
   * If true, then the comma is ignored in the MD5 calculation, even
   * though it's a valid IGC character.
   *
   * Background information: in XCSoar 6.5, the IGC standard was
   * implemented correctly for the first time, and the comma became a
   * legal character.  Up to XCSoar 6.4.6, the comma was not only
   * ignored for the GRecord, but also eliminated from IGC files.
   * This made XCSoar 6.5 incompatible with the old vali-xcs program
   * (TRAC #2657).
   *
   * This attribute is the workaround: ignore_comma=false means that
   * we're currently reading a XCSoar 6.5 IGC file.
   *
   * When writing a new IGC file, the comma is written, but ignored
   * for the G record calculation.
   */
  bool ignore_comma;

public:

  void Initialize();

  /**
   * @return returns true if record is appended, false if skipped
   */
  bool AppendRecordToBuffer(const char *szIn);
  void FinalizeBuffer();

  /**
   * @param buffer a buffer of at least #DIGEST_LENGTH+1 bytes
   */
  void GetDigest(char *buffer) const;

  /** loads a file into the data buffer */
  bool LoadFileToBuffer(const TCHAR *path);

  void WriteTo(TextWriter &writer) const;

  bool AppendGRecordToFile(const TCHAR *path);

  /**
   * returns in szOutput the G Record from the file
   */
  static bool ReadGRecordFromFile(const TCHAR *path,
                                  char *buffer, size_t max_length);

  /// returns 0 if false, 1 if true
  bool VerifyGRecordInFile(const TCHAR *path);

private:
  void AppendStringToBuffer(const char *szIn);
  /**
   * returns false if record is not to be included in
   * G record calc (see IGC specs)
   */
  bool IncludeRecordInGCalc(const char *szIn);
};
#endif

