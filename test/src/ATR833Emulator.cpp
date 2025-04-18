// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ATR833Emulator.hpp"
#include "Device/Driver/ATR833/Buffer.hpp"
#include "Device/Driver/ATR833/Protocol.hpp"

#include <fmt/format.h>

using namespace ATR833;

inline std::size_t
ATR833Emulator::Handle(std::span<const std::byte> src) noexcept
{
  if (src.empty())
    return 0;

  if (src.front() == STX) {
    if (src.size() < 3)
      return 0;

    if (src[1] != SYNC)
      return 1;

    switch (src[2]) {
    case ALIVE:
      return WithSTX<0>(src, [this](auto){
        fmt::print("alive\n");
        port->Write(ATRBuffer{ALIVE}.Finish());
      });

    case EXCHANGE:
      return WithSTX<0>(src, [this](auto){
        fmt::print("exchange\n");
        using std::swap;
        swap(active_frequency, standby_frequency);
      });

    case SETACTIVE:
      return WithSTX<2>(src, [this](std::span<const std::byte, 2> payload){
        fmt::print("setactive\n");
        active_frequency = ReadRadioFrequency(payload.subspan<0, 2>());
      });

    case SETSTANDBY:
      return WithSTX<2>(src, [this](std::span<const std::byte, 2> payload){
        fmt::print("setstandby\n");
        standby_frequency = ReadRadioFrequency(payload.subspan<0, 2>());
      });

    case REQUESTDATA:
      return WithSTX<0>(src, [this](auto){
        fmt::print("requestdata\n");

        ATRBuffer b{ALLDATA};
        b.Put(active_frequency);
        b.Put(standby_frequency);
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        b.Put(std::byte{});
        port->Write(b.Finish());
      });

    default:
      fmt::print("unknown command: {:#x}\n", src[2]);
      return 3;
    }
  } else
    return 1;
}

bool ATR833Emulator::DataReceived(std::span<const std::byte> s) noexcept
{
  do {
    const auto nbytes = buffer.MoveFrom(s);
    if (nbytes == 0) {
      fmt::print("buffer full\n");
      buffer.Clear();
      continue;
    }

    s = s.subspan(nbytes);

    while (true) {
      const auto consumed = Handle(buffer.Read());
      if (consumed == 0)
        break;

      buffer.Consume(consumed);
    }
  }  while (!s.empty());

  return true;
}
