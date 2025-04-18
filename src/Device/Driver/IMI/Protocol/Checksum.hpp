// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

namespace IMI {

/**
 * @brief Calculates IMI CRC value
 *
 * @param message Message for which CRC should be provided
 * @param bytes The size of the message
 *
 * @return IMI CRC value
 */
IMIWORD
CRC16Checksum(const void *message, unsigned bytes);

IMIBYTE
FixChecksum(const void *message, unsigned bytes);

} // namespace IMI
