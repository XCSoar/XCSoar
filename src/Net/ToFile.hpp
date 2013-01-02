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

#ifndef NET_TO_FILE_HPP
#define NET_TO_FILE_HPP

#include "Job/Job.hpp"

#include <tchar.h>

class OperationEnvironment;

namespace Net {
  class Session;

  /**
   * Download a URL into the specified file.
   *
   * @param md5_digest an optional buffer with at least 33 characters
   * which will contain the hex MD5 digest after returning
   * @return true on success, false on error
   */
  bool DownloadToFile(Session &session, const char *url, const TCHAR *path,
                      char *md5_digest,
                      OperationEnvironment &env);

  class DownloadToFileJob : public Job {
    Session &session;
    const char *url;
    const TCHAR *path;
    char md5_digest[33];
    bool success;

  public:
    DownloadToFileJob(Session &_session, const char *_url, const TCHAR *_path)
      :session(_session), url(_url), path(_path), success(false) {}

    bool WasSuccessful() const {
      return success;
    }

    const char *GetMD5Digest() const {
      return md5_digest;
    }

    virtual void Run(OperationEnvironment &env);
  };
}

#endif
