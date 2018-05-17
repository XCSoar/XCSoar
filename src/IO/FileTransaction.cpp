/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include <assert.h>

static AllocatedPath
MakeTemporaryPath(Path path)
{
  assert(path != nullptr);

#ifdef HAVE_POSIX
  return path + _T(".tmp");
#else
  return path.WithExtension(_T(".tmp"));
#endif
}

FileTransaction::FileTransaction(Path _path)
  :final_path(_path), temporary_path(MakeTemporaryPath(_path))
{
  /* ensure the temporary file doesn't exist already */
  File::Delete(temporary_path);
}

FileTransaction::~FileTransaction()
{
  if (!temporary_path.IsNull())
    /* cancel the transaction */
    File::Delete(temporary_path);
}

bool
FileTransaction::Commit()
{
  assert(!temporary_path.IsNull());

  bool success = File::Replace(temporary_path, final_path);
  if (success)
    /* mark the transaction as "finished" to avoid deletion in the
       destructor */
    temporary_path = nullptr;

  return success;
}

void
FileTransaction::Abandon()
{
  assert(!temporary_path.IsNull());

  temporary_path = nullptr;
}
