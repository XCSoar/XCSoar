// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"

class DeviceBlackboard;
struct NMEAInfo;

class DeviceDataEditor {
  DeviceBlackboard &blackboard;

  const std::lock_guard<Mutex> lock;

  NMEAInfo &basic;

public:
  DeviceDataEditor(DeviceBlackboard &blackboard,
                   std::size_t idx) noexcept;

  void Commit() const noexcept;

  NMEAInfo *operator->() const noexcept {
    return &basic;
  }

  NMEAInfo &operator*() const noexcept {
    return basic;
  }
};
