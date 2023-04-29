// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/ATR833.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "RadioFrequency.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Operation.hpp"
#include "time/TimeoutClock.hpp"
#include "util/StaticFifoBuffer.hxx"

#include <cstdint>

static constexpr std::byte STX{0x02};
static constexpr std::byte SYNC{'r'};

class ATR833Device final : public AbstractDevice {
  static constexpr std::byte ACK{0x06};
  static constexpr std::byte NAK{0x15};
  static constexpr std::byte ALIVE{0x10};
  static constexpr std::byte EXCHANGE{0x11};
  static constexpr std::byte SETSTANDBY{0x12};
  static constexpr std::byte SETACTIVE{0x13};
  static constexpr std::byte ALLDATA{0x42};
  static constexpr std::byte REQUESTDATA{0x82};

  Port &port;
  /**
   * Buffer which receives the messages send from the radio.
  */ 
  StaticFifoBuffer<std::byte, 256u> rx_buf;

public:
  explicit ATR833Device(Port &_port):port(_port) {}

public:
  bool DataReceived(std::span<const std::byte> s,
                    NMEAInfo &info) noexcept override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
  bool EnableNMEA(OperationEnvironment &env) override;
  void OnSysTicker() override;
  void LinkTimeout() override;

private:
  PeriodClock status_clock;

  /**
   * Handles responses from the radio.
   */
  void HandleResponse(const std::byte *data, struct NMEAInfo &info);

  static std::size_t HandleSTX(std::span<const std::byte> src, NMEAInfo &info) noexcept;

  /**
   * Handle a raw message data received on the port.  It may be called
   * repeatedly until all data has been consumed.
   *
   * @return the number of bytes consumed or 0 if the message is
   * incomplete
   */
  static std::size_t HandleMessage(std::span<const std::byte> src, NMEAInfo &info) noexcept;
};

class ATRBuffer {
  uint8_t fill = 0;
  std::byte checksum{};
  std::byte data[32];

public:
  explicit constexpr ATRBuffer(std::byte msg_id) noexcept {
    data[fill++] = STX;
    Put(SYNC);
    Put(msg_id);
  }

  constexpr void Put(std::byte byte) noexcept {
    data[fill++] = byte;
    checksum ^= byte;
    if (byte == STX) {
      data[fill++] = byte;
      checksum ^= byte;
    }
  }

  void Send(Port &port, OperationEnvironment &env) {
    data[fill++] = checksum;
    port.FullWrite(std::span{data}.first(fill), env, std::chrono::seconds(2));
  }
};

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
    info.settings.active_frequency =
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(src[3]),
                                        static_cast<unsigned>(src[4]) * 5);
    return 5;

  case SETSTANDBY:
    // Standby frequency
    if (src.size() < 5)
      return 0;

    info.alive.Update(info.clock);
    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency =
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(src[3]),
                                        static_cast<unsigned>(src[4]) * 5);
    return 5;

  case EXCHANGE:
    // Exchange frequencies
    info.alive.Update(info.clock);
    info.settings.swap_frequencies.Update(info.clock);
    return 3;

  case ALLDATA:
    // receive full dataset (freq, VOL, etc...)
    return src.size() < 15 ? 0 : 15;

  case ACK:
    return src.size() < 4 ? 0 : 4;

  case NAK:
    return src.size() < 5 ? 0 : 5;

  case ALIVE:
    return src.size() < 4 ? 0 : 4;

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

static Device *
ATR833CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new ATR833Device(com_port);
}

const DeviceRegister atr833_driver = {
  _T("ATR833"),
  _T("ATR833"),
  DeviceRegister::RAW_GPS_DATA,
  ATR833CreateOnPort,
};
