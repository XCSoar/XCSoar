// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <optional>
#include <string>

class OrderedTask;

/**
 * Encode a task as the text of an XCTrack QR code ("XCTSK:" plus the
 * version-2 JSON).
 *
 * Only cylinder zones are written, which is what
 * DecodeXCTrackTask() reads back.  A line, sector or keyhole becomes
 * a different task if it is forced into that shape, so those tasks
 * are refused.
 *
 * @return the payload, or nullopt if the task is empty, a zone is not
 * a cylinder, or a radius or elevation is outside the range XCTrack
 * accepts
 */
std::optional<std::string>
EncodeXCTrackQRText(const OrderedTask &task);
