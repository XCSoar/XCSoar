// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoDownloadToFile.hpp"
#include "Progress.hpp"
#include "lib/curl/Setup.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "io/DigestOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "system/Path.hpp"
#include "lib/sodium/SHA256.hxx"

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
    digest->Final(std::span{*sha256});

  co_return response;
}

} // namespace Net
