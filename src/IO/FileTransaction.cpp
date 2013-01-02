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

#include "FileTransaction.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

#include <assert.h>

static void
MakeTemporaryPath(TCHAR *path)
{
  assert(path != NULL);

#ifdef HAVE_POSIX
  _tcscat(path, _T(".tmp"));
#else
  TCHAR *base = const_cast<TCHAR *>(BaseName(path));
  if (base == NULL) {
    /* dirty fallback */
    _tcscat(path, _T(DIR_SEPARATOR_S "tmp.tmp"));
    return;
  }

  TCHAR *dot = _tcsrchr(base, '.');
  if (dot != NULL)
    /* replace existing file name extension */
    _tcscpy(dot + 1, _T("tmp"));
  else
    /* append file name extension */
    _tcscat(base, _T(".tmp"));
#endif
}

FileTransaction::FileTransaction(const TCHAR *_path)
  :final_path(_path), temporary_path(_path)
{
  assert(_path != NULL);

  MakeTemporaryPath(temporary_path.buffer());

  /* ensure the temporary file doesn't exist already */
  File::Delete(temporary_path.c_str());
}

FileTransaction::~FileTransaction()
{
  if (!temporary_path.empty())
    /* cancel the transaction */
    File::Delete(temporary_path.c_str());
}

bool
FileTransaction::Commit()
{
  assert(!temporary_path.empty());

  bool success = File::Replace(temporary_path.c_str(), final_path.c_str());
  if (success)
    /* mark the transaction as "finished" to avoid deletion in the
       destructor */
    temporary_path.clear();

  return success;
}

void
FileTransaction::Abandon()
{
  assert(!temporary_path.empty());
  temporary_path.clear();
}
