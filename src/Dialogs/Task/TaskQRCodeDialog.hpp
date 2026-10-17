// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class OrderedTask;

/**
 * Show the task as a QR code another XCSoar or XCTrack can scan.
 *
 * Tells the pilot when the task cannot be represented: it is empty,
 * or a zone is not a cylinder, or the payload does not fit in a QR
 * code.
 */
void
ShowTaskQRCode(const OrderedTask &task) noexcept;
