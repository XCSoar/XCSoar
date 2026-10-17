// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

class OrderedTask;
struct TaskBehaviour;

/** Prefix of a QR payload that carries a SeeYou CUP task. */
inline constexpr std::string_view CUP_TASK_QR_PREFIX{"CUP:"};

/**
 * Encode a task as the text of a SeeYou CUP QR code.
 *
 * The payload is @ref CUP_TASK_QR_PREFIX followed by a CUP file: the
 * waypoints, then one racing or assigned-area task.  Lines, cylinders,
 * sectors and the standard BGA areas are written.  A zone SeeYou
 * cannot describe, a start-height limit above the ground, or a
 * finish-height limit is refused, because loading the file would
 * change the task.
 *
 * @return the payload, or nullopt if the task cannot be written
 */
std::optional<std::string> EncodeCupTaskQRText(const OrderedTask &task);

/**
 * Read a task from the CUP half of a SeeYou QR payload (no prefix).
 *
 * Throws if the text is not a task file.
 */
std::unique_ptr<OrderedTask>
DecodeCupTaskQRText(std::string_view cup, const TaskBehaviour &task_behaviour);
