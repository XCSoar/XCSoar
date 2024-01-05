// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace FLARM {

struct FrameHeader;

/**
 * Calculates the CRC value of the FrameHeader and an optional payload
 * @param header FrameHeader to calculate the CRC for
 * @return CRC value
 */
[[gnu::pure]]
uint16_t
CalculateCRC(const FrameHeader &header, std::span<const std::byte> payload) noexcept;

} // namespace FLARM
