// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Buffer.hpp"
#include "Protocol.hpp"
#include "RadioFrequency.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Operation.hpp"
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

  switch (src[2]) {
  case SETACTIVE:
    // Active frequency
    if (src.size() < 5)
      return 0;

    info.alive.Update(info.clock);
    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency = ReadRadioFrequency(src.subspan<3, 2>());
    return 5;

  case SETSTANDBY:
    // Standby frequency
    if (src.size() < 5)
      return 0;

    info.alive.Update(info.clock);
    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency = ReadRadioFrequency(src.subspan<3, 2>());
    return 5;

  case EXCHANGE:
    // Exchange frequencies
    info.alive.Update(info.clock);
    info.settings.swap_frequencies.Update(info.clock);
    return 3;

  case ALLDATA:
    /*
      byte 4: MHz active
      byte 5: kHz/5 active
      byte 6: MHz standby
      byte 7: kHz/5 standby
      byte 8-15: not used by XCSoar (VOL, SQ, VOX, INT, DIM, EXT, spacing, dual)
    */
    if (src.size() < 15)
      return 0;

    info.alive.Update(info.clock);

    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency = ReadRadioFrequency(src.subspan<3, 2>());
    info.settings.standby_frequency = ReadRadioFrequency(src.subspan<5, 2>());

    return 15;

  case ACK:
    return src.size() < 4 ? 0 : 4;

  case NAK:
    return src.size() < 5 ? 0 : 5;

  case ALIVE:
    if (src.size() < 4)
      return 0;

    info.alive.Update(info.clock);
    return 4;

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
  buffer.Put(static_cast<std::byte>(frequency.GetKiloHertz() / 1000));
  buffer.Put(static_cast<std::byte>((frequency.GetKiloHertz() % 1000) / 5));
  buffer.Send(port, env);
  return true;
}

bool
ATR833Device::PutStandbyFrequency(RadioFrequency frequency,
                                  [[maybe_unused]] const TCHAR *name,
                                  OperationEnvironment &env)
{
  ATRBuffer buffer(SETSTANDBY);
  buffer.Put(static_cast<std::byte>(frequency.GetKiloHertz() / 1000));
  buffer.Put(static_cast<std::byte>((frequency.GetKiloHertz() % 1000) / 5));
  buffer.Send(port, env);
  return true;
}

void
ATR833Device::LinkTimeout()
{
  NullOperationEnvironment env;

  ATRBuffer{ALIVE}.Send(port, env);
  ATRBuffer{REQUESTDATA}.Send(port, env);
}

bool
ATR833Device::EnableNMEA(OperationEnvironment &env)
{
  ATRBuffer{ALIVE}.Send(port, env);
  ATRBuffer{REQUESTDATA}.Send(port, env);

  return true;
}

void
ATR833Device::OnSysTicker()
{
  // ALIVE shall be send every 5 seconds
  if (status_clock.CheckUpdate(std::chrono::seconds(5))) {
    NullOperationEnvironment env;

    ATRBuffer{ALIVE}.Send(port, env);

    ATRBuffer{REQUESTDATA}.Send(port, env);
  }
}
