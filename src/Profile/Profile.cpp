/*
Copyright_License {

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

#include "Profile/Profile.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "LocalPath.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "IO/KeyValueFileReader.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"
#include "IO/FileTransaction.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

#include <string.h>
#include <windef.h> /* for MAX_PATH */

#define XCSPROFILE "xcsoar-registry.prf"

namespace Profile {
  static bool SaveFile(const FileTransaction &transaction);
}

static TCHAR startProfileFile[MAX_PATH];

const TCHAR *
Profile::GetPath()
{
  return startProfileFile;
}

void
Profile::Load()
{
  LogFormat("Loading profiles");
  LoadFile(startProfileFile);
  SetModified(false);
}

void
Profile::LoadFile(const TCHAR *szFile)
{
  if (StringIsEmpty(szFile))
    return;

  FileLineReaderA reader(szFile);
  if (reader.error())
    return;

  LogStartUp(_T("Loading profile from %s"), szFile);

  KeyValueFileReader kvreader(reader);
  KeyValuePair pair;
  while (kvreader.Read(pair))
    Set(pair.key, pair.value);
}

void
Profile::Save()
{
  if (!IsModified())
    return;

  LogFormat("Saving profiles");
  if (StringIsEmpty(startProfileFile))
    SetFiles(_T(""));
  SaveFile(startProfileFile);
}

bool
Profile::SaveFile(const FileTransaction &transaction)
{
  TextWriter writer(transaction.GetTemporaryPath());
  // ... on error -> return
  if (!writer.IsOpen())
    return false;

  KeyValueFileWriter kvwriter(writer);
  Export(kvwriter);

  return writer.Flush();
}

void
Profile::SaveFile(const TCHAR *szFile)
{
  if (StringIsEmpty(szFile))
    return;

  LogStartUp(_T("Saving profile to %s"), szFile);

  // Try to open the file for writing
  FileTransaction transaction(szFile);
  if (SaveFile(transaction))
    transaction.Commit();
}

void
Profile::SetFiles(const TCHAR *override_path)
{
  /* set the "modified" flag, because we are potentially saving to a
     new file now */
  SetModified(true);

  if (!StringIsEmpty(override_path)) {
    if (IsBaseName(override_path)) {
      LocalPath(startProfileFile, override_path);

      if (_tcschr(override_path, '.') == NULL)
        _tcscat(startProfileFile, _T(".prf"));
    } else
      CopyString(startProfileFile, override_path, MAX_PATH);
    return;
  }

  // Set the default profile file
  LocalPath(startProfileFile, _T(XCSPROFILE));

  if (IsAltair() && !File::Exists(startProfileFile)) {
    /* backwards compatibility with old Altair firmware */
    LocalPath(startProfileFile, _T("config/")_T(XCSPROFILE));
    if (!File::Exists(startProfileFile))
      LocalPath(startProfileFile, _T(XCSPROFILE));
  }
}

bool
Profile::GetPath(const char *key, TCHAR *value)
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return false;

  ExpandLocalPath(value, buffer);
  return true;
}

bool
Profile::GetPathIsEqual(const char *key, const TCHAR *value)
{
  TCHAR saved[MAX_PATH];
  if (!GetPath(key, saved))
    return false;

  return StringIsEqual(saved, value);
}

const TCHAR *
Profile::GetPathBase(const char *key)
{
  TCHAR buffer[MAX_PATH];
  if (!Get(key, buffer, ARRAY_SIZE(buffer)))
      return nullptr;

  const TCHAR *p = buffer;
  if (DIR_SEPARATOR != '\\') {
    const TCHAR *backslash = _tcsrchr(p, _T('\\'));
    if (backslash != NULL)
      p = backslash + 1;
  }

  return BaseName(p);
}

void
Profile::SetPath(const char *key, const TCHAR *value)
{
  TCHAR path[MAX_PATH];

  if (StringIsEmpty(value))
    path[0] = '\0';
  else {
    CopyString(path, value, MAX_PATH);
    ContractLocalPath(path);
  }

  Set(key, path);
}
