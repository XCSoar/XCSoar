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

#include "DownloadManager.hpp"
#include "OS/Path.hpp"

#ifdef ANDROID

#include "Android/DownloadManager.hpp"
#include "Android/Main.hpp"

static AndroidDownloadManager *download_manager;

bool
Net::DownloadManager::Initialise()
{
  assert(download_manager == nullptr);

  if (!AndroidDownloadManager::Initialise(Java::GetEnv()))
    return false;

  download_manager = AndroidDownloadManager::Create(Java::GetEnv(), *context);
  return download_manager != nullptr;
}

void
Net::DownloadManager::BeginDeinitialise()
{
}

void
Net::DownloadManager::Deinitialise()
{
  delete download_manager;
  download_manager = nullptr;
  AndroidDownloadManager::Deinitialise(Java::GetEnv());
}

bool
Net::DownloadManager::IsAvailable()
{
  return download_manager != nullptr;
}

void
Net::DownloadManager::AddListener(DownloadListener &listener)
{
  assert(download_manager != nullptr);

  download_manager->AddListener(listener);
}

void
Net::DownloadManager::RemoveListener(DownloadListener &listener)
{
  assert(download_manager != nullptr);

  download_manager->RemoveListener(listener);
}

void
Net::DownloadManager::Enumerate(DownloadListener &listener)
{
  assert(download_manager != nullptr);

  download_manager->Enumerate(Java::GetEnv(), listener);
}

void
Net::DownloadManager::Enqueue(const char *uri, Path relative_path)
{
  assert(download_manager != nullptr);

  download_manager->Enqueue(Java::GetEnv(), uri, relative_path);
}

void
Net::DownloadManager::Cancel(Path relative_path)
{
  assert(download_manager != nullptr);

  download_manager->Cancel(Java::GetEnv(), relative_path);
}

#else /* !ANDROID */

#include "ToFile.hpp"
#include "Session.hpp"
#include "Thread/StandbyThread.hpp"
#include "Operation/Operation.hpp"
#include "LocalPath.hpp"
#include "IO/FileTransaction.hpp"
#include "LogFile.hpp"

#include <string>
#include <list>
#include <algorithm>

#include <string.h>

class DownloadManagerThread final
  : protected StandbyThread, private QuietOperationEnvironment {
  struct Item {
    std::string uri;
    AllocatedPath path_relative;

    Item(const Item &other) = delete;

    Item(Item &&other)
      :uri(std::move(other.uri)),
       path_relative(std::move(other.path_relative)) {}

    Item(const char *_uri, Path _path_relative)
      :uri(_uri), path_relative(_path_relative) {}

    Item &operator=(const Item &other) = delete;

    gcc_pure
    bool operator==(Path other) const {
      return path_relative == other;
    }
  };

  /**
   * Information about the current download, i.e. queue.front().
   * Protected by StandbyThread::mutex.
   */
  int64_t current_size, current_position;

  std::list<Item> queue;

  std::list<Net::DownloadListener *> listeners;

public:
  DownloadManagerThread()
    :StandbyThread("DownloadMgr"),
     current_size(-1), current_position(-1) {}

  void StopAsync() {
    ScopeLock protect(mutex);
    StandbyThread::StopAsync();
  }

  void WaitStopped() {
    ScopeLock protect(mutex);
    StandbyThread::WaitStopped();
  }

  void AddListener(Net::DownloadListener &listener) {
    ScopeLock protect(mutex);

    assert(std::find(listeners.begin(), listeners.end(),
                     &listener) == listeners.end());

    listeners.push_back(&listener);
  }

  void RemoveListener(Net::DownloadListener &listener) {
    ScopeLock protect(mutex);

    auto i = std::find(listeners.begin(), listeners.end(), &listener);
    assert(i != listeners.end());
    listeners.erase(i);
  }

  void Enumerate(Net::DownloadListener &listener) {
    ScopeLock protect(mutex);

    for (auto i = queue.begin(), end = queue.end(); i != end; ++i) {
      const Item &item = *i;

      int64_t size = -1, position = -1;
      if (i == queue.begin()) {
        size = current_size;
        position = current_position;
      }

      listener.OnDownloadAdded(item.path_relative, size, position);
    }
  }

  void Enqueue(const char *uri, Path path_relative) {
    ScopeLock protect(mutex);
    queue.emplace_back(uri, path_relative);

    for (auto *listener : listeners)
      listener->OnDownloadAdded(path_relative, -1, -1);

    if (!IsBusy())
      Trigger();
  }

  void Cancel(Path relative_path) {
    ScopeLock protect(mutex);

    auto i = std::find(queue.begin(), queue.end(), relative_path);
    if (i == queue.end())
      return;

    if (i == queue.begin()) {
      /* current download; stop the thread to cancel the current file
         and restart the thread to continue downloading the following
         files */

      StandbyThread::StopAsync();
      StandbyThread::WaitStopped();

      if (!queue.empty())
        Trigger();
    } else {
      /* queued download; simply remove it from the list */
      queue.erase(i);
    }

    for (auto *listener : listeners)
      listener->OnDownloadComplete(relative_path, false);
  }

private:
  void ProcessQueue(Net::Session &session);
  void FailQueue();

protected:
  /* methods from class StandbyThread */
  void Tick() override;

private:
  /* methods from class OperationEnvironment */
  bool IsCancelled() const override {
    ScopeLock protect(const_cast<Mutex &>(mutex));
    return StandbyThread::IsStopped();
  }

  void SetProgressRange(unsigned range) override {
    ScopeLock protect(mutex);
    current_size = range;
  }

  void SetProgressPosition(unsigned position) override {
    ScopeLock protect(mutex);
    current_position = position;
  }
};

static bool
DownloadToFileTransaction(Net::Session &session,
                          const char *url, Path path,
                          char *md5_digest, OperationEnvironment &env)
{
  FileTransaction transaction(path);
  return DownloadToFile(session, url, transaction.GetTemporaryPath(),
                        md5_digest, env) &&
    transaction.Commit();
}

inline void
DownloadManagerThread::ProcessQueue(Net::Session &session)
{
  while (!queue.empty() && !StandbyThread::IsStopped()) {
    assert(current_size == -1);
    assert(current_position == -1);

    const Item &item = queue.front();
    current_position = 0;

    bool success = false;
    try {
      const ScopeUnlock unlock(mutex);
      success = DownloadToFileTransaction(session, item.uri.c_str(),
                                          LocalPath(item.path_relative.c_str()),
                                          nullptr, *this);
    } catch (const std::exception &exception) {
      LogError(exception);
    }

    current_size = current_position = -1;
    const AllocatedPath path_relative(std::move(queue.front().path_relative));
    queue.pop_front();

    for (auto *listener : listeners)
      listener->OnDownloadComplete(path_relative, success);
  }
}

inline void
DownloadManagerThread::FailQueue()
{
  while (!queue.empty() && !StandbyThread::IsStopped()) {
    const AllocatedPath path_relative(std::move(queue.front().path_relative));
    queue.pop_front();

    for (auto *listener : listeners)
      listener->OnDownloadComplete(path_relative, false);
  }
}

void
DownloadManagerThread::Tick()
{
  try {
    Net::Session session;
    ProcessQueue(session);
  } catch (const std::exception &exception) {
    LogError(exception);
    FailQueue();
  }
}

static DownloadManagerThread *thread;

bool
Net::DownloadManager::Initialise()
{
  assert(thread == nullptr);

  thread = new DownloadManagerThread();
  return true;
}

void
Net::DownloadManager::BeginDeinitialise()
{
  assert(thread != nullptr);

  thread->StopAsync();
}

void
Net::DownloadManager::Deinitialise()
{
  assert(thread != nullptr);

  thread->WaitStopped();
  delete thread;
}

bool
Net::DownloadManager::IsAvailable()
{
  assert(thread != nullptr);

  return true;
}

void
Net::DownloadManager::AddListener(DownloadListener &listener)
{
  assert(thread != nullptr);

  thread->AddListener(listener);
}

void
Net::DownloadManager::RemoveListener(DownloadListener &listener)
{
  assert(thread != nullptr);

  thread->RemoveListener(listener);
}

void
Net::DownloadManager::Enumerate(DownloadListener &listener)
{
  assert(thread != nullptr);

  thread->Enumerate(listener);
}

void
Net::DownloadManager::Enqueue(const char *uri, Path relative_path)
{
  assert(thread != nullptr);

  thread->Enqueue(uri, relative_path);
}

void
Net::DownloadManager::Cancel(Path relative_path)
{
  assert(thread != nullptr);

  thread->Cancel(relative_path);
}

#endif
