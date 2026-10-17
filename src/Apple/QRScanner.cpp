// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/QRScanner.hpp"

/**
 * No camera yet.  Fill this file in with AVCaptureMetadataOutput and
 * pass the string to ReceiveTaskQRCode() (any thread is fine; it
 * posts to the UI).  Leave the vario's AVAudioSession alone.
 * HaveQRScanner() should then return true once the UI is up, and the
 * task-manager buttons appear on their own.
 */

bool
HaveQRScanner() noexcept
{
  return false;
}

void
ScanTaskQRCode() noexcept
{
}
