// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "lib/curl/CoRequest.hxx"
#include "co/Task.hxx"

#include <array>
#include <cstddef> // for std::byte

class Path;
class ProgressListener;

namespace Net {

/**
 * Download a URL into the specified file.
 *
 * Throws on error.
 */
Co::EagerTask<Curl::CoResponse>
CoDownloadToFile(CurlGlobal &curl, const char *url,
                 const char *username, const char *password,
                 Path path, std::array<std::byte, 32> *sha256,
                 ProgressListener &progress);

} // namespace Net
