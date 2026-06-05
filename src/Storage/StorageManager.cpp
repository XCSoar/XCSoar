// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageManager.hpp"
#include "PlatformStorageHotplugMonitor.hpp"
#include "PlatformStorageMonitor.hpp"
#include "LogFile.hpp"
#include "thread/Debug.hpp"

#include <algorithm>
#include <cassert>
#include <exception>
#include <unordered_map>
#include <utility>

StorageManager::StorageManager(NotifyCallback notify_callback)
  : notify_callback_(std::move(notify_callback))
{
  monitor_ = CreatePlatformStorageHotplugMonitor(*this);
  enumerator_ = CreatePlatformStorageMonitor();
}

StorageManager::~StorageManager()
{
  shutting_down_.store(true);
  StopMonitoring();

  if (refresh_thread_.joinable())
    refresh_thread_.join();
}

void
StorageManager::StartMonitoring() noexcept
try
{
  /* Populate the device list before starting the hotplug monitor
     so that GetDevices() returns a valid snapshot immediately. */
  RefreshDevices();

  if (monitor_)
    monitor_->Start();
}
catch (...) {
  LogError(std::current_exception());
}

void
StorageManager::StopMonitoring() noexcept
{
  if (monitor_)
    monitor_->Stop();
}

std::vector<std::shared_ptr<StorageDevice>>
StorageManager::GetDevices() const noexcept
{
  std::lock_guard<std::mutex> lock(devices_mutex_);
  return devices_;
}

bool
StorageManager::ApplyEnumeratedDevices(
    std::vector<std::shared_ptr<StorageDevice>> new_devices,
    StorageChange &change)
{
  std::unordered_map<std::string, std::shared_ptr<StorageDevice>> old_map;
  {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    old_map.reserve(devices_.size());
    for (const auto &d : devices_)
      old_map.emplace(d->Id(), d);
  }

  std::unordered_map<std::string, std::shared_ptr<StorageDevice>> new_map;
  new_map.reserve(new_devices.size());
  for (const auto &d : new_devices)
    new_map.emplace(d->Id(), d);

  if (old_map.size() == new_map.size() &&
      std::all_of(old_map.begin(), old_map.end(),
                  [&new_map](const auto &p) {
                    auto it = new_map.find(p.first);
                    if (it == new_map.end())
                      return false;
                    /* Detect permission state changes: a device that
                       previously needed permission but no longer does
                       counts as a change. */
                    if (p.second->NeedsPermission() !=
                        it->second->NeedsPermission())
                      return false;
                    /* Detect tree URI changes (e.g. user picked a
                       different folder on the same volume via
                       "Change folder"). */
                    if (p.second->Name() != it->second->Name())
                      return false;
                    return true;
                  })) {
    change.added.clear();
    change.removed.clear();
    change.access_granted.clear();
    return false;
  }

  change.added.clear();
  change.removed.clear();
  change.access_granted.clear();

  for (const auto &p : new_map) {
    auto it = old_map.find(p.first);
    if (it == old_map.end()) {
      change.added.push_back(p.second);
    } else if (it->second->NeedsPermission() &&
               !p.second->NeedsPermission()) {
      /* Device was known but lacked permission; now it has access. */
      change.access_granted.push_back(p.second);
    } else if (it->second->Name() != p.second->Name()) {
      /* Same device but different tree URI (e.g. user picked a
         different folder).  Treat as a new access grant so
         listeners refresh. */
      change.access_granted.push_back(p.second);
    }
  }

  for (const auto &p : old_map) {
    if (new_map.find(p.first) == new_map.end())
      change.removed.push_back(p.second);
  }

  {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    devices_ = std::move(new_devices);
  }

  return true;
}

bool
StorageManager::EnumerateAndApply(StorageChange &change)
{
  std::lock_guard<std::mutex> lock(enumerate_mutex_);
  return ApplyEnumeratedDevices(enumerator_->Enumerate(), change);
}

bool
StorageManager::RefreshDevices()
{
  if (!enumerator_)
    return false;

  /* Background refresh owns enumeration while running. */
  if (refresh_running_.load())
    return false;

  StorageChange change;
  const bool changed = EnumerateAndApply(change);

  if (!changed) {
    last_change_.added.clear();
    last_change_.removed.clear();
    last_change_.access_granted.clear();
    return false;
  }

  last_change_ = std::move(change);
  return true;
}

void
StorageManager::RunBackgroundRefresh() noexcept
try
{
  refresh_pending_.store(false);

  StorageChange change;
  const bool changed = EnumerateAndApply(change);

  if (changed && !shutting_down_.load()) {
    {
      std::lock_guard<std::mutex> lock(refresh_mutex_);
      pending_change_ = std::move(change);
    }

    notify_callback_();
  }

  if (refresh_pending_.exchange(false) && !shutting_down_.load())
    RunBackgroundRefresh();
  else
    refresh_running_.store(false);
}
catch (...) {
  refresh_running_.store(false);
  LogError(std::current_exception());
}

void
StorageManager::OnStorageTopologyChanged() noexcept
try
{
  // Called from platform monitor (may be non-UI thread). Launch
  // a background enumeration task to avoid blocking the UI thread.
  if (!enumerator_ || shutting_down_.load())
    return;

  if (refresh_running_.load()) {
    refresh_pending_.store(true);
    return;
  }

  bool expected = false;
  if (!refresh_running_.compare_exchange_strong(expected, true)) {
    refresh_pending_.store(true);
    return;
  }

  try {
    /* The previous worker has finished (refresh_running_ was false),
       but its std::thread object is still joinable.  Move-assigning
       onto a joinable std::thread calls std::terminate(), so join the
       completed thread before starting a new one.  This happens when a
       single hotplug action emits multiple topology events. */
    if (refresh_thread_.joinable())
      refresh_thread_.join();

    refresh_thread_ = std::thread([this]() noexcept {
      RunBackgroundRefresh();
    });
  } catch (...) {
    refresh_running_.store(false);
    LogError(std::current_exception());
  }
}
catch (...) {
  LogError(std::current_exception());
}

const StorageManager::StorageChange &
StorageManager::GetLastChange() const noexcept
{
  assert(InMainThread());
  return last_change_;
}

void
StorageManager::DispatchEvents(const StorageChange &change) noexcept
{
  assert(InMainThread());

  /* Iterate over a copy for safety against removal during
     iteration. */
  const auto copy = event_listeners_;

  if (!change.added.empty()) {
    StorageEventInfo info;
    info.type = StorageEvent::Available;
    info.devices = change.added;
    for (auto *l : copy)
      l->OnStorageEvent(info);
  }

  if (!change.removed.empty()) {
    StorageEventInfo info;
    info.type = StorageEvent::Removed;
    info.devices = change.removed;
    for (auto *l : copy)
      l->OnStorageEvent(info);
  }

  if (!change.access_granted.empty()) {
    StorageEventInfo info;
    info.type = StorageEvent::AccessGranted;
    info.devices = change.access_granted;
    for (auto *l : copy)
      l->OnStorageEvent(info);
  }
}

void
StorageManager::ProcessPendingChanges() noexcept
try
{
  assert(InMainThread());

  if (shutting_down_.load())
    return;

  // Prefer pending change (produced by background task). If none,
  // fall back to immediate RefreshDevices() for callers that expect
  // synchronous behaviour.
  StorageChange change;
  {
    std::lock_guard<std::mutex> lock(refresh_mutex_);
    change = std::move(pending_change_);
    pending_change_ = StorageChange{};
  }

  if (change.added.empty() && change.removed.empty() &&
      change.access_granted.empty()) {
    const bool changed = RefreshDevices();
    if (!changed)
      return;
    change = last_change_;
  } else {
    last_change_ = change;
  }

#ifndef NDEBUG
  assert(!calling_listeners_);
  calling_listeners_ = true;
#endif

  DispatchEvents(change);

#ifndef NDEBUG
  calling_listeners_ = false;
#endif
}
catch (...) {
  LogError(std::current_exception());
}

void
StorageManager::AddEventListener(StorageEventListener &listener) noexcept
{
  assert(InMainThread());
  assert(std::find(event_listeners_.begin(), event_listeners_.end(),
                   &listener) == event_listeners_.end());
  event_listeners_.append(&listener);
}

void
StorageManager::RemoveEventListener(StorageEventListener &listener) noexcept
{
  assert(InMainThread());
  auto i = std::find(event_listeners_.begin(), event_listeners_.end(),
                     &listener);
  assert(i != event_listeners_.end());
  event_listeners_.quick_remove(
    std::distance(event_listeners_.begin(), i));
}
