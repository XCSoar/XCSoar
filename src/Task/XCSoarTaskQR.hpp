// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

class OrderedTask;
struct TaskBehaviour;

/** Prefix of a QR payload that carries an XCSoar task file. */
inline constexpr std::string_view XCSOAR_TASK_QR_PREFIX{"XCS:"};

/**
 * Encode a task as the text of an XCSoar QR code.
 *
 * The payload is @ref XCSOAR_TASK_QR_PREFIX followed by the same
 * compact task XML that a ".tsk" file stores, so lines, sectors,
 * keyholes and the task rules survive the trip to another XCSoar.
 *
 * @return the payload, or nullopt if the task cannot be written
 */
std::optional<std::string>
EncodeXCSoarTaskQRText(const OrderedTask &task);

/**
 * Read a task from the XML half of an XCSoar QR payload (no prefix).
 *
 * Throws if the text is not a task file.
 */
std::unique_ptr<OrderedTask>
DecodeXCSoarTaskQRText(std::string_view xml,
                       const TaskBehaviour &task_behaviour);
