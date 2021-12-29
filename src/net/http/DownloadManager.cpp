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

#include "DownloadManager.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"

#ifdef ANDROID

#include "Android/DownloadManager.hpp"
#include "Android/Main.hpp"

static AndroidDownloadManager *download_manager;

bool
Net::DownloadManager::Initialise() noexcept
{
  assert(download_manager == nullptr);

  if (!AndroidDownloadManager::Initialise(Java::GetEnv()))
    return false;

  try {
    download_manager = new AndroidDownloadManager(Java::GetEnv(), *context);
    return true;
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to initialise the DownloadManager");
    return false;
  }
}

void
Net::DownloadManager::BeginDeinitialise() noexcept
{
}

void
Net::DownloadManager::Deinitialise() noexcept
{
  delete download_manager;
  download_manager = nullptr;
  AndroidDownloadManager::Deinitialise(Java::GetEnv());
}

bool
Net::DownloadManager::IsAvailable() noexcept
{
  return download_manager != nullptr;
}

void
Net::DownloadManager::AddListener(DownloadListener &listener) noexcept
{
  assert(download_manager != nullptr);

  download_manager->AddListener(listener);
}

void
Net::DownloadManager::RemoveListener(DownloadListener &listener) noexcept
{
  assert(download_manager != nullptr);

  download_manager->RemoveListener(listener);
}

void
Net::DownloadManager::Enumerate(DownloadListener &listener) noexcept
{
  assert(download_manager != nullptr);

  download_manager->Enumerate(Java::GetEnv(), listener);
}

void
Net::DownloadManager::Enqueue(const char *uri, Path relative_path) noexcept
{
  assert(download_manager != nullptr);

  download_manager->Enqueue(Java::GetEnv(), uri, relative_path);
}

void
Net::DownloadManager::Cancel(Path relative_path) noexcept
{
  assert(download_manager != nullptr);

  download_manager->Cancel(Java::GetEnv(), relative_path);
}

#else /* !ANDROID */

#include "Global.hxx"
#include "Init.hpp"
#include "CoDownloadToFile.hpp"
#include "Operation/ProgressListener.hpp"
#include "LocalPath.hpp"
#include "thread/Mutex.hxx"
#include "co/InjectTask.hxx"
#include "io/FileTransaction.hpp"

#include <string>
#include <list>
#include <algorithm>

#include <string.h>

class DownloadManagerThread final
  : ProgressListener {
  struct Item {
    std::string uri;
    AllocatedPath path_relative;

    Item(const Item &other) = delete;

    Item(Item &&other) noexcept = default;

    Item(const char *_uri, Path _path_relative) noexcept
      :uri(_uri), path_relative(_path_relative) {}

    Item &operator=(const Item &other) = delete;

    [[gnu::pure]]
    bool operator==(Path other) const noexcept {
      return path_relative == other;
    }
  };

  /**
   * The coroutine performing the current download.
   */
  Co::InjectTask task{Net::curl->GetEventLoop()};

  Mutex mutex;

  /**
   * Information about the current download, i.e. queue.front().
   * Protected by #mutex.
   */
  int64_t current_size = -1, current_position = -1;

  std::list<Item> queue;

  std::list<Net::DownloadListener *> listeners;

public:
  void AddListener(Net::DownloadListener &listener) noexcept {
    assert(std::find(listeners.begin(), listeners.end(),
                     &listener) == listeners.end());

    listeners.push_back(&listener);
  }

  void RemoveListener(Net::DownloadListener &listener) noexcept {
    auto i = std::find(listeners.begin(), listeners.end(), &listener);
    assert(i != listeners.end());
    listeners.erase(i);
  }

  void Enumerate(Net::DownloadListener &listener) noexcept {
    for (auto i = queue.begin(), end = queue.end(); i != end; ++i) {
      const Item &item = *i;

      int64_t size = -1, position = -1;
      if (i == queue.begin()) {
        const std::lock_guard lock{mutex};
        size = current_size;
        position = current_position;
      }

      listener.OnDownloadAdded(item.path_relative, size, position);
    }
  }

  void Enqueue(const char *uri, Path path_relative) noexcept {
    queue.emplace_back(uri, path_relative);

    for (auto *listener : listeners)
      listener->OnDownloadAdded(path_relative, -1, -1);

    if (!task)
      Start();
  }

  void Cancel(Path relative_path) noexcept {
    auto i = std::find(queue.begin(), queue.end(), relative_path);
    if (i == queue.end())
      return;

    if (i == queue.begin()) {
      /* current download; stop the thread to cancel the current file
         and restart the thread to continue downloading the following
         files */

      task.Cancel();

      if (!queue.empty())
        Start();
    } else {
      /* queued download; simply remove it from the list */
      queue.erase(i);
    }

    for (auto *listener : listeners)
      listener->OnDownloadError(relative_path, {});
  }

private:
  void Start() noexcept;
  void OnCompletion(std::exception_ptr error) noexcept;

  /* methods from class ProgressListener */
  void SetProgressRange(unsigned range) noexcept override {
    std::lock_guard<Mutex> lock(mutex);
    current_size = range;
  }

  void SetProgressPosition(unsigned position) noexcept override {
    std::lock_guard<Mutex> lock(mutex);
    current_position = position;
  }
};

static Co::InvokeTask
DownloadToFileTransaction(CurlGlobal &curl,
                          const char *url, AllocatedPath path,
                          std::array<std::byte, 32> *sha256,
                          ProgressListener &progress)
{
  FileTransaction transaction(path);
  const auto ignored_response = co_await
    Net::CoDownloadToFile(curl, url, nullptr, nullptr,
                          transaction.GetTemporaryPath(),
                          sha256, progress);
  transaction.Commit();
}

void
DownloadManagerThread::Start() noexcept
{
  assert(!queue.empty());
  assert(!task);
  assert(current_size == -1);
  assert(current_position == -1);

  const Item &item = queue.front();
  current_position = 0;

  task.Start(DownloadToFileTransaction(*Net::curl, item.uri.c_str(),
                                       LocalPath(item.path_relative.c_str()),
                                       nullptr, *this),
             BIND_THIS_METHOD(OnCompletion));
}

void
DownloadManagerThread::OnCompletion(std::exception_ptr error) noexcept
{
  assert(!queue.empty());

  const AllocatedPath path_relative = std::move(queue.front().path_relative);
  queue.pop_front();

  current_size = current_position = -1;

  if (error) {
    LogError(error);
    for (auto *listener : listeners)
      listener->OnDownloadError(path_relative, error);
  } else {
    for (auto *listener : listeners)
      listener->OnDownloadComplete(path_relative);
  }

  // start the next download
  if (!queue.empty())
    Start();
}

static DownloadManagerThread *thread;

bool
Net::DownloadManager::Initialise() noexcept
{
  assert(thread == nullptr);

  thread = new DownloadManagerThread();
  return true;
}

void
Net::DownloadManager::BeginDeinitialise() noexcept
{
}

void
Net::DownloadManager::Deinitialise() noexcept
{
  assert(thread != nullptr);

  delete thread;
}

bool
Net::DownloadManager::IsAvailable() noexcept
{
  assert(thread != nullptr);

  return true;
}

void
Net::DownloadManager::AddListener(DownloadListener &listener) noexcept
{
  assert(thread != nullptr);

  thread->AddListener(listener);
}

void
Net::DownloadManager::RemoveListener(DownloadListener &listener) noexcept
{
  assert(thread != nullptr);

  thread->RemoveListener(listener);
}

void
Net::DownloadManager::Enumerate(DownloadListener &listener) noexcept
{
  assert(thread != nullptr);

  thread->Enumerate(listener);
}

void
Net::DownloadManager::Enqueue(const char *uri, Path relative_path) noexcept
{
  assert(thread != nullptr);

  thread->Enqueue(uri, relative_path);
}

void
Net::DownloadManager::Cancel(Path relative_path) noexcept
{
  assert(thread != nullptr);

  thread->Cancel(relative_path);
}

#endif
