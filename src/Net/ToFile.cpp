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

#include "Net/ToFile.hpp"
#include "Net/Features.hpp"
#include "Net/Request.hpp"
#include "Operation/Operation.hpp"
#include "OS/FileUtil.hpp"
#include "Logger/MD5.hpp"

#include <assert.h>
#include <stdio.h>

static bool
DownloadToFile(Net::Session &session, const char *url, FILE *file,
               char *md5_digest,
               OperationEnvironment &env)
{
  assert(url != NULL);
  assert(file != NULL);

  Net::Request request(session, url, 10000);
  if (!request.Send(10000))
    return false;

  int64_t total = request.GetLength();
  if (total >= 0)
    env.SetProgressRange(total);
  total = 0;

  MD5 md5;
  md5.InitKey();

  uint8_t buffer[4096];
  while (true) {
    if (env.IsCancelled())
      return false;

    ssize_t nbytes = request.Read(buffer, sizeof(buffer), 5000);
    if (nbytes < 0)
      return false;

    if (nbytes == 0)
      break;

    if (md5_digest != NULL)
      md5.Append(buffer, nbytes);

    size_t written = fwrite(buffer, 1, nbytes, file);
    if (written != (size_t)nbytes)
      return false;

    total += nbytes;
    env.SetProgressPosition(total);
  }

  if (md5_digest != NULL) {
    md5.Finalize();
    md5.GetDigest(md5_digest);
  }

  return true;
}

bool
Net::DownloadToFile(Session &session, const char *url, const TCHAR *path,
                    char *md5_digest,
                    OperationEnvironment &env)
{
  assert(url != NULL);
  assert(path != NULL);

  /* make sure we create a new file */
  if (!File::Delete(path) && File::ExistsAny(path))
    /* failed to delete the old file */
    return false;

  /* now create the new file */
  FILE *file = _tfopen(path, _T("wb"));
  if (file == NULL)
    return false;

  bool success = ::DownloadToFile(session, url, file, md5_digest, env);
  success &= fclose(file) == 0;

  if (!success)
    /* delete the partial file on error */
    File::Delete(path);

  return success;
}

void
Net::DownloadToFileJob::Run(OperationEnvironment &env)
{
  success = DownloadToFile(session, url, path, md5_digest,env);
}
