// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadManager.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "Init.hpp"
#include "CoDownloadToFile.hpp"
#include "lib/curl/Global.hxx"
#include "Operation/ProgressListener.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "thread/Mutex.hxx"
#include "thread/SafeList.hxx"
#include "co/InjectTask.hxx"

#include <string>
#include <list>
#include <algorithm>
#include <utility>

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
   * The download #queue is also protected by #mutex.
   */
  int64_t current_size = -1, current_position = -1;

  std::list<Item> queue;

  ThreadSafeList<Net::DownloadListener *> listeners;

  bool shutting_down = false;

public:
  void BeginShutdown() noexcept;
  void AddListener(Net::DownloadListener &listener) noexcept {
    listeners.Add(&listener);
  }

  void RemoveListener(Net::DownloadListener &listener) noexcept {
    listeners.Remove(&listener);
  }

  void Enumerate(Net::DownloadListener &listener) noexcept {
    const std::lock_guard lock{mutex};

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

  void Enqueue(const char *uri, Path path_relative) noexcept {
    {
      const std::lock_guard lock{mutex};

      if (shutting_down)
        return;

      /* skip duplicates already in the queue (e.g. a file re-enqueued
         before its previous download completed) */
      if (std::find(queue.begin(), queue.end(), path_relative) != queue.end())
        return;

      queue.emplace_back(uri, path_relative);

      if (!task && current_position == -1)
        Start();
    }

    listeners.ForEach([path_relative](auto *listener){
      listener->OnDownloadAdded(path_relative, -1, -1);
    });
  }

  void Cancel(Path relative_path) noexcept {
    bool cancel_active = false;
    bool cancelled = false;

    {
      const std::lock_guard lock{mutex};

      auto i = std::find(queue.begin(), queue.end(), relative_path);
      if (i == queue.end())
        return;

      if (i != queue.begin()) {
        /* queued download; simply remove it from the list */
        queue.erase(i);
        cancelled = true;
      } else {
        cancel_active = true;
      }
    }

    if (cancel_active) {
      /* If #task is already idle, the coroutine finished and
         OnCompletion() is in flight (InjectTask clears #alive before
         invoking the callback).  Do not advance the queue here or we
         race with OnCompletion() and trip assert(!task) in Start(). */
      if (!task)
        return;

      /* Re-check that the requested item is still the active download;
         OnCompletion() may have finished it and started the next one
         while we were waiting to call task.Cancel() */
      {
        const std::lock_guard lock{mutex};

        if (queue.empty() || queue.front() != relative_path) {
          auto i = std::find(queue.begin(), queue.end(), relative_path);
          if (i == queue.end())
            return;

          if (i != queue.begin()) {
            queue.erase(i);
            cancelled = true;
          }

          return;
        }
      }

      /* Cancel the coroutine without holding #mutex: task.Cancel() blocks on
         the event loop, which may be in OnCompletion() waiting for #mutex */
      task.Cancel();

      const std::lock_guard lock{mutex};

      auto i = std::find(queue.begin(), queue.end(), relative_path);
      if (i == queue.end())
        return;

      cancelled = true;

      current_size = current_position = -1;
      queue.erase(i);

      if (!queue.empty() && !task && current_position == -1)
        Start();
    }

    if (cancelled) {
      listeners.ForEach([relative_path](auto *listener){
        listener->OnDownloadError(relative_path, {});
      });
    }
  }

private:
  void Start() noexcept;
  void OnCompletion(std::exception_ptr error) noexcept;

  /* methods from class ProgressListener */
  void SetProgressRange(unsigned range) noexcept override {
    const std::lock_guard lock{mutex};
    current_size = range;
  }

  void SetProgressPosition(unsigned position) noexcept override {
    const std::lock_guard lock{mutex};
    current_position = position;
  }
};

static Co::InvokeTask
DownloadToFile(CurlGlobal &curl,
               const char *url, AllocatedPath path,
               std::array<std::byte, 32> *sha256,
               ProgressListener &progress)
{
  const auto ignored_response = co_await
    Net::CoDownloadToFile(curl, url, nullptr, nullptr,
                          path, sha256, progress);
}

void
DownloadManagerThread::BeginShutdown() noexcept
{
  bool cancel_active = false;

  {
    const std::lock_guard lock{mutex};

    if (shutting_down)
      return;

    shutting_down = true;
    cancel_active = bool(task);
  }

  if (cancel_active)
    task.Cancel();

  const std::lock_guard lock{mutex};
  queue.clear();
  current_size = current_position = -1;
}

void
DownloadManagerThread::Start() noexcept
{
  if (shutting_down)
    return;
  assert(!queue.empty());
  assert(!task);
  assert(current_size == -1);
  assert(current_position == -1);

  const Item &item = queue.front();
  current_position = 0;

  auto destination = LocalPath(item.path_relative.c_str());
  if (const auto parent = destination.GetParent(); parent != nullptr)
    Directory::CreateRecursive(parent);

  task.Start(DownloadToFile(*Net::curl, item.uri.c_str(),
                            std::move(destination),
                            nullptr, *this),
             BIND_THIS_METHOD(OnCompletion));
}

void
DownloadManagerThread::OnCompletion(std::exception_ptr error) noexcept
{
  AllocatedPath path_relative;

  {
    const std::lock_guard lock{mutex};

    assert(!queue.empty());

    path_relative = std::move(queue.front().path_relative);
    queue.pop_front();

    current_size = current_position = -1;

    /* start the next download before notifying listeners; this keeps
       #task alive while callbacks run and closes the race where
       Enqueue() could call Start() during completion handling */
    if (!queue.empty() && !task)
      Start();
  }

  if (error) {
    LogError(error);
    listeners.ForEach([path=Path{path_relative}, &error](auto *listener){
      listener->OnDownloadError(path, error);
    });
  } else {
    listeners.ForEach([path=Path{path_relative}](auto *listener){
      listener->OnDownloadComplete(path);
    });
  }
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
  if (thread != nullptr)
    thread->BeginShutdown();
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
