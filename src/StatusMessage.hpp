// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>
#include <tchar.h>

/**
 * Struct used to store status message items
 */
struct StatusMessage {
  /** English key */
  const TCHAR *key;

  /** What sound entry to play */
  const TCHAR *sound;

  bool visible;

  /** Delay for DoStatusMessage */
  std::chrono::steady_clock::duration delay;
};

[[gnu::pure]]
const StatusMessage &
FindStatusMessage(const TCHAR *key);
