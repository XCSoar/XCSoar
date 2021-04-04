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

#ifndef XCSOAR_NET_DOWNLOAD_MANAGER_HPP
#define XCSOAR_NET_DOWNLOAD_MANAGER_HPP

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

#endif
