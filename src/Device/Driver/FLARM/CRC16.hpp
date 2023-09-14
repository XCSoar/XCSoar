// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <cstddef>

namespace FLARM {

struct FrameHeader;

/**
 * Calculates the CRC value of the FrameHeader and an optional payload
 * @param header FrameHeader to calculate the CRC for
 * @param data Optional pointer to the first byte of the payload
 * @param length Optional length of the payload
 * @return CRC value
 */
[[gnu::pure]]
uint16_t CalculateCRC(const FrameHeader &header, const void *data = nullptr,
                      size_t length = 0);

} // namespace FLARM
