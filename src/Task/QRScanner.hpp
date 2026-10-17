// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Scan one task QR code with the built-in camera.
 *
 * Android implements this in Android/QRScanner.cpp.  Apple's file is
 * Apple/QRScanner.cpp.  Every other target links Task/QRScanner.cpp,
 * which reports that there is no camera.  A new port implements the
 * two functions below and lists its file instead of Task/QRScanner.cpp
 * in build/main.mk.  Pass the scanned text to ReceiveTaskQRCode();
 * that may run on any thread.
 */

/**
 * Can ScanTaskQRCode() open a camera right now?
 *
 * True when this port has a scanner and the UI is up.  Android is
 * true once native_view exists.  Callers add the Scan QR Code button
 * only when this is true.  Camera permission is separate: if the user
 * denies it, ScanTaskQRCode() returns without a task.
 */
[[gnu::pure]]
bool
HaveQRScanner() noexcept;

/**
 * Open the camera and scan one code.  Returns immediately.  Nothing
 * happens when the user cancels or denies the camera permission.
 */
void
ScanTaskQRCode() noexcept;
