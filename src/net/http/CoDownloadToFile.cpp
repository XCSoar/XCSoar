/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "CoDownloadToFile.hpp"
#include "Setup.hxx"
#include "Progress.hpp"
#include "CoStreamRequest.hxx"
#include "io/FileOutputStream.hxx"
#include "Crypto/SHA256.hxx"
#include "Crypto/DigestOutputStream.hxx"

#include <cassert>
#include <optional>

namespace Net {

Co::EagerTask<Curl::CoResponse>
CoDownloadToFile(CurlGlobal &curl, const char *url,
                 const char *username, const char *password,
                 Path path, std::array<std::byte, 32> *sha256,
                 ProgressListener &progress)
{
  assert(url != nullptr);
  assert(path != nullptr);

  FileOutputStream file(path);
  OutputStream *os = &file;

  std::optional<DigestOutputStream<SHA256State>> digest;
  if (sha256 != nullptr)
    os = &digest.emplace(*os);

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();

  if (username != nullptr)
    easy.SetOption(CURLOPT_USERNAME, username);
  if (password != nullptr)
    easy.SetOption(CURLOPT_PASSWORD, password);

  auto response = co_await Curl::CoStreamRequest(curl, std::move(easy), *os);
  file.Commit();

  if (sha256 != nullptr)
    digest->Final(sha256);

  co_return response;
}

} // namespace Net
