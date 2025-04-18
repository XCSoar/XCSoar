// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#include <cstdint>
#include <exception>

class Path;

namespace Net {

class DownloadListener {
public:
  /**
   * This is called by the #DownloadManager when a new download was
   * added, and when DownloadManager::Enumerate() is called.
   *
   * @param size the total size of the file when the download has
   * been finished; -1 if unknown
   * @param position the number of bytes already downloaded; -1 if
   * the download is queued, but has not been started yet
   */
  virtual void OnDownloadAdded(Path path_relative,
                               int64_t size, int64_t position) noexcept = 0;

  virtual void OnDownloadComplete(Path path_relative) noexcept = 0;

  /**
   * The download has failed or was canceled.
   *
   * @param error error details; may be empty (e.g. if this was due to
   * cancellation)
   */
  virtual void OnDownloadError(Path path_relative,
                               std::exception_ptr error) noexcept = 0;
};

} // namespace Net

namespace Net::DownloadManager {

#ifdef HAVE_DOWNLOAD_MANAGER
bool Initialise() noexcept;
void BeginDeinitialise() noexcept;
void Deinitialise() noexcept;

[[gnu::const]]
bool IsAvailable() noexcept;

void AddListener(DownloadListener &listener) noexcept;
void RemoveListener(DownloadListener &listener) noexcept;

/**
 * Enumerate the download queue, and invoke
 * DownloadListener::OnDownloadAdded() for each one.
 */
void Enumerate(DownloadListener &listener) noexcept;

void Enqueue(const char *uri, Path relative_path) noexcept;

/**
 * Cancel the download.  The download may however be already
 * finished before this function attempts the cancellation.
 */
void Cancel(Path relative_path) noexcept;
#else

static constexpr bool IsAvailable() noexcept {
  return false;
}
#endif

} // namespace Net::DownloadManager
