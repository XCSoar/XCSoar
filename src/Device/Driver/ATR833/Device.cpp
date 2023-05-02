// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Buffer.hpp"
#include "Protocol.hpp"
#include "RadioFrequency.hpp"
#include "NMEA/Info.hpp"
#include "Device/Port/Port.hpp"
#include "time/TimeoutClock.hpp"

#include <cstdint>

using namespace ATR833;

bool
ATR833Device::DataReceived(std::span<const std::byte> s,
                           [[maybe_unused]] NMEAInfo &info) noexcept
{
  assert(!s.empty());

  do {
    // Append new data to the buffer, as much as fits in there
    const auto nbytes = rx_buf.MoveFrom(s);
    if (nbytes == 0) {
      rx_buf.Clear();
      continue;
    }

    s = s.subspan(nbytes);

    for (;;) {
      // Read data from buffer to handle the messages
      const auto consumed = HandleMessage(rx_buf.Read(), info);
      if (consumed == 0)
        // need more data
        break;

      // Message handled -> remove message
      rx_buf.Consume(consumed);
    }
  } while (!s.empty());

  return true;
}

inline std::size_t
ATR833Device::HandleSTX(std::span<const std::byte> src, NMEAInfo &info) noexcept
{
  if (src.size() < 3)
    return 0;

  if (src[1] != SYNC)
    return 1;

  switch (src[2]) {
  case SETACTIVE:
    // Active frequency
    return WithSTX<2>(src, [&info](std::span<const std::byte, 2> payload){
      info.alive.Update(info.clock);

      if (auto f = ReadRadioFrequency(payload.subspan<0, 2>()); f.IsDefined()) {
        info.settings.has_active_frequency.Update(info.clock);
        info.settings.active_frequency = f;
      }
    });

  case SETSTANDBY:
    // Standby frequency
    return WithSTX<2>(src, [&info](std::span<const std::byte, 2> payload){
      info.alive.Update(info.clock);

      if (auto f = ReadRadioFrequency(payload.subspan<0, 2>()); f.IsDefined()) {
        info.settings.has_standby_frequency.Update(info.clock);
        info.settings.standby_frequency = f;
      }
    });

  case EXCHANGE:
    // Exchange frequencies
    return WithSTX<0>(src, [&info](auto){
      info.alive.Update(info.clock);
      info.settings.swap_frequencies.Update(info.clock);
    });

  case ALLDATA:
    /*
      byte 4: MHz active
      byte 5: kHz/5 active
      byte 6: MHz standby
      byte 7: kHz/5 standby
      byte 8-15: not used by XCSoar (VOL, SQ, VOX, INT, DIM, EXT, spacing, dual)
    */
    return WithSTX<12>(src, [&info](std::span<const std::byte, 12> payload){
      info.alive.Update(info.clock);

      if (auto f = ReadRadioFrequency(payload.subspan<0, 2>()); f.IsDefined()) {
        info.settings.has_active_frequency.Update(info.clock);
        info.settings.active_frequency = f;
      }

      if (auto f = ReadRadioFrequency(payload.subspan<2, 2>()); f.IsDefined()) {
        info.settings.has_standby_frequency.Update(info.clock);
        info.settings.standby_frequency = f;
      }
    });

  case ACK:
    return WithSTX<1>(src, [](auto){});

  case NAK:
    return WithSTX<2>(src, [](auto){});

  case ALIVE:
    return WithSTX<0>(src, [&info](auto){
      info.alive.Update(info.clock);
    });

  default:
    // Received unknown msg id (code)
    return 3;
  }
}

inline std::size_t
ATR833Device::HandleMessage(std::span<const std::byte> src,
                            NMEAInfo &info) noexcept
{
  if (src.empty())
    return 0;

  switch (src.front()) {
  case STX:
    return HandleSTX(src, info);

  default:
    return 1;
  }
}

bool
ATR833Device::PutActiveFrequency(RadioFrequency frequency,
                                 [[maybe_unused]] const TCHAR *name,
                                 OperationEnvironment &env)
{
  ATRBuffer buffer(SETACTIVE);
  buffer.Put(frequency);
  port.FullWrite(buffer.Finish(), env, std::chrono::seconds{2});
  return true;
}

bool
ATR833Device::PutStandbyFrequency(RadioFrequency frequency,
                                  [[maybe_unused]] const TCHAR *name,
                                  OperationEnvironment &env)
{
  ATRBuffer buffer(SETSTANDBY);
  buffer.Put(frequency);
  port.FullWrite(buffer.Finish(), env, std::chrono::seconds{2});
  return true;
}

void
ATR833Device::LinkTimeout()
{
  port.Write(ATRBuffer{ALIVE}.Finish());
  port.Write(ATRBuffer{REQUESTDATA}.Finish());
}

bool
ATR833Device::EnableNMEA(OperationEnvironment &env)
{
  port.FullWrite(ATRBuffer{ALIVE}.Finish(), env, std::chrono::seconds{2});
  port.FullWrite(ATRBuffer{REQUESTDATA}.Finish(), env, std::chrono::seconds{2});

  return true;
}

void
ATR833Device::OnSysTicker()
{
  // ALIVE shall be send every 5 seconds
  if (status_clock.CheckUpdate(std::chrono::seconds(5))) {
    port.Write(ATRBuffer{ALIVE}.Finish());
    port.Write(ATRBuffer{REQUESTDATA}.Finish());
  }
}
