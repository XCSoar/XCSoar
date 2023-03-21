// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/ATR833.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "RadioFrequency.hpp"

static constexpr uint8_t STX = 0x02;
static constexpr uint8_t SYNC = 'r';

class ATR833Device final : public AbstractDevice {
  static constexpr uint8_t SETSTANDBY = 0x12;
  static constexpr uint8_t SETACTIVE = 0x13;

  Port &port;

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
};

class ATRBuffer {
  uint8_t fill;
  uint8_t checksum;
  uint8_t data[32];

public:
  explicit ATRBuffer(uint8_t msg_id):fill(0), checksum(0) {
    data[fill++] = STX;
    Put(SYNC);
    Put(msg_id);
  }

  void Put(uint8_t byte) {
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
ATR833Device::DataReceived(std::span<const std::byte>,
                           [[maybe_unused]] NMEAInfo &info) noexcept
{
  // actually made no use of radio information
  // TODO: interpret data delivered by ATR833 to display Radio settings...
  return true;
}

bool
ATR833Device::PutActiveFrequency(RadioFrequency frequency,
                                 [[maybe_unused]] const TCHAR *name,
                                 OperationEnvironment &env)
{
  ATRBuffer buffer(SETACTIVE);
  buffer.Put(frequency.GetKiloHertz() / 1000);
  buffer.Put((frequency.GetKiloHertz() % 1000) / 5);
  buffer.Send(port, env);
  return true;
}


bool
ATR833Device::PutStandbyFrequency(RadioFrequency frequency,
                                  [[maybe_unused]] const TCHAR *name,
                                  OperationEnvironment &env)
{
  ATRBuffer buffer(SETSTANDBY);
  buffer.Put(frequency.GetKiloHertz() / 1000);
  buffer.Put((frequency.GetKiloHertz() % 1000) / 5);
  buffer.Send(port, env);
  return true;
}

static Device *
ATR833CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new ATR833Device(com_port);
}


const DeviceRegister atr833_driver = {
  _T("ATR833"),
  _T("ATR833"),
  DeviceRegister::NO_TIMEOUT |
  DeviceRegister::RAW_GPS_DATA,
  ATR833CreateOnPort,
};


