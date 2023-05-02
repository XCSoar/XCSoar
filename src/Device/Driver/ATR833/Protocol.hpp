// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RadioFrequency.hpp"
#include "util/Concepts.hxx"

#include <cstddef>
#include <span>

namespace ATR833 {

static constexpr std::byte STX{0x02};
static constexpr std::byte SYNC{'r'};

static constexpr std::byte ACK{0x06};
static constexpr std::byte NAK{0x15};
static constexpr std::byte ALIVE{0x10};
static constexpr std::byte EXCHANGE{0x11};
static constexpr std::byte SETSTANDBY{0x12};
static constexpr std::byte SETACTIVE{0x13};
static constexpr std::byte ALLDATA{0x42};
static constexpr std::byte REQUESTDATA{0x82};

/**
 * Decode a message starting with STX,SYNC,id and pass the payload to
 * the given function.
 *
 * @return the number of bytes consumed (including the STX,SYNC,id
 * header) or 0 if the message is incomplete
 */
template<std::size_t size>
static constexpr std::size_t
WithSTX(std::span<const std::byte> src,
        Invocable<std::span<const std::byte, size>> auto f) noexcept
{
  if (src.size() < size)
    return 0;

  // skip STX, SYNC, id
  src = src.subspan(3);

  f(src.first<size>());

  return 3 + size;
}

static constexpr RadioFrequency
ReadRadioFrequency(std::span<const std::byte, 2> src) noexcept
{
  return RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(src[0]),
                                           static_cast<unsigned>(src[1]) * 5);

}

} // namespace ATR833
