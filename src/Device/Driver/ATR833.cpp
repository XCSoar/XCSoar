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

  /**
   * Calculates the length of the message just receiving.
   *
   * @param data Pointer to the first character of the message.
   * @param length Number of characters received.
   * @return Expected message length.
   */
  static std::size_t ExpectedMsgLength(std::span<const std::byte> src) noexcept;

  /**
   * Calculates the length of the command message just receiving.
   *
   * @param code Command code received after the STX+'r' character.
   * @return Expected message length after the code character.
   */
  static std::size_t ExpectedMsgLengthCommand(std::byte code) noexcept;
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
    port.FullWrite(data, fill, env, std::chrono::seconds(2));
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
      const auto range = rx_buf.Read();;
      if (range.empty())
        break;

      const auto expected_msg_length = ExpectedMsgLength(range);
      if (range.size() < expected_msg_length)
        break;

      if (range.front() == STX) {
        HandleResponse(range.data(), info);
      }
      // Message handled -> remove message
      rx_buf.Consume(expected_msg_length);
    }
  } while (!s.empty());

  return true;
}

/**
  The expected length of a received message may change,
  when the first character is STX and the second character
  is not received yet.
*/
std::size_t
ATR833Device::ExpectedMsgLength(std::span<const std::byte> src) noexcept
{
  assert(!src.empty());

  if (src[0] == STX) {
    if (src.size() > 2) {
      return 3 + ExpectedMsgLengthCommand(src[2]);
    } else {
      // minimum 3 chars
      return 3;
    }
  } else
    return 1;
}

std::size_t
ATR833Device::ExpectedMsgLengthCommand(std::byte code) noexcept
{
  switch (code) {
  case SETACTIVE:
    // Active frequency
    return 2;
  case SETSTANDBY:
    // Standby frequency
    return 2;
  case EXCHANGE:
    // Exchange frequencies
    return 1;
  case ALLDATA:
    // receive full dataset (freq, VOL, etc...)
    return 12;
  case ACK:
    return 1;
  case NAK:
    return 2;
  case ALIVE:
    return 1;
  default:
    // Received unknown msg id (code)
    return 0;
  }
}

void
ATR833Device::HandleResponse(const std::byte *data, struct NMEAInfo &info)
{
  info.alive.Update(info.clock);

  if(data[2] != EXCHANGE && data[2] != ALLDATA &&
     data[2] != SETACTIVE && data[2] != SETSTANDBY)
    return;

  if(data[2] == SETACTIVE) {
    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency =
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(data[3]),
                                        static_cast<unsigned>(data[4]) * 5);
  }

  if(data[2] == SETSTANDBY) {
    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency =
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(data[3]),
                                        static_cast<unsigned>(data[4]) * 5);
  }

  if(data[2] == EXCHANGE) {
    const auto old_active_freq = info.settings.active_frequency;

    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency = info.settings.standby_frequency;

    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency = old_active_freq;
  }

  if(data[2] == ALLDATA) {
    /*
      byte 4: MHz active
      byte 5: kHz/5 active
      byte 6: MHz standby
      byte 7: kHz/5 standby     
      byte 8-15: not used by XCSoar (VOL, SQ, VOX, INT, DIM, EXT, spacing, dual)   
    */
    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency = 
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(data[3]),
                                        static_cast<unsigned>(data[4]) * 5);

    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency = 
      RadioFrequency::FromMegaKiloHertz(static_cast<unsigned>(data[5]),
                                        static_cast<unsigned>(data[6]) * 5);
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


