// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageHotplugMonitor.hpp"
#include "thread/Thread.hpp"
#include "system/EventFD.hxx"
#include "event/PipeEvent.hxx"
#include "ui/event/Globals.hpp"

#include <atomic>

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
  EventFD trigger_;
  EventFD shutdown_fd_;
  PipeEvent trigger_event_;

  int socket_fd_ = -1;
  std::atomic<bool> running_{false};

  void SetupSocket() noexcept;
  void CloseSocket() noexcept;

  void Run() noexcept override;
  void OnTriggerReady(unsigned) noexcept;
};
