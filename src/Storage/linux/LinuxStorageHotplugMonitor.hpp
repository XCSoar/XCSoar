// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageHotplugMonitor.hpp"
#include "thread/Thread.hpp"
#include "system/EventFD.hxx"

#include <atomic>

#ifdef USE_POLL_EVENT
#include "event/PipeEvent.hxx"
#include "ui/event/Globals.hpp"
#else
#include "ui/event/Notify.hpp"
#endif

class LinuxStorageHotplugMonitor final :
  public StorageHotplugMonitor, private Thread
{
public:
  explicit LinuxStorageHotplugMonitor(StorageHotplugHandler &handler);
  ~LinuxStorageHotplugMonitor() override;

  void Start() noexcept override;
  void Stop() noexcept override;

private:
  StorageHotplugHandler &handler_;
  EventFD shutdown_fd_;

#ifdef USE_POLL_EVENT
  EventFD trigger_;
  PipeEvent trigger_event_;
#else
  UI::Notify ui_notify_;
#endif

  int socket_fd_ = -1;
  std::atomic<bool> running_{false};

  void SetupSocket() noexcept;
  void CloseSocket() noexcept;

  void Run() noexcept override;

#ifdef USE_POLL_EVENT
  void OnTriggerReady(unsigned) noexcept;
#endif

  void DispatchTrigger() noexcept;
};
