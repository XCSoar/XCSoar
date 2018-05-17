/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
  bool DataReceived(const void *data, size_t length, NMEAInfo &info) override;
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

  bool Send(Port &port, OperationEnvironment &env) {
    data[fill++] = checksum;
    return port.FullWrite(data, fill, env, 2000);
  }
};

bool
ATR833Device::DataReceived(const void *data, size_t length,
                           NMEAInfo &info)
{
  // actually made no use of radio information
  // TODO: interpret data delivered by ATR833 to display Radio settings...
  return true;
}

bool
ATR833Device::PutActiveFrequency(RadioFrequency frequency,
                                 const TCHAR *name,
                                 OperationEnvironment &env)
{
  ATRBuffer buffer(SETACTIVE);
  buffer.Put(frequency.GetKiloHertz() / 1000);
  buffer.Put((frequency.GetKiloHertz() % 1000) / 5);
  return buffer.Send(port, env);
}


bool
ATR833Device::PutStandbyFrequency(RadioFrequency frequency,
                                  const TCHAR *name,
                                  OperationEnvironment &env)
{
  ATRBuffer buffer(SETSTANDBY);
  buffer.Put(frequency.GetKiloHertz() / 1000);
  buffer.Put((frequency.GetKiloHertz() % 1000) / 5);
  return buffer.Send(port, env);
}


static Device *
ATR833CreateOnPort(const DeviceConfig &config, Port &com_port)
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


