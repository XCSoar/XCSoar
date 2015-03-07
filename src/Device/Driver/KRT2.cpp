/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Device/Driver/KRT2.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "RadioFrequency.hpp"
#include "Util/TrivialArray.hpp"
#include "Util/StringUtil.hpp"
#include "NMEA/Info.hpp"

#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <algorithm>

static const char SYNC = 'S';
static const char SYNC_ACK = 0x01;
static const char STX = 0x02;
static const char ACK = 0x06;
static const char NAK = 0x15;

class KRT2Device : public AbstractDevice {
  Port &port;
  TrivialArray<char, 32> input_buffer;
  bool received_squelch;  // received volume settigs from radio at least once
  char squelch;           // squelch setting as received from radio
  char vox;               // VOX_ setting as received from radio

public:
  KRT2Device(Port &_port)
    : port(_port), received_squelch(false), squelch(0), vox(0) {}

protected:
  bool PutFrequency(RadioFrequency frequency, const TCHAR *name, char cmd);
  bool ParseFrame(struct NMEAInfo &info);

public:
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
  bool DataReceived(const void *data, size_t length,
                    struct NMEAInfo &info) override;
};

static RadioFrequency
DecodeFrequency(char mhz, char khz)
{
  RadioFrequency rf;
  rf.SetKiloHertz(static_cast<unsigned>(mhz) * 1000 +
                  static_cast<unsigned>(khz) * 5);
  return rf;
}

/**
 * Get Frame size for known frames including STX and cmd.
 */
static unsigned
FrameSize(char cmd)
{
  switch (cmd) {
  case 'C':  // initialize
    return 11;
  case 'A':  // set volume
    return 6;
  case 'U':  // set active freq
  case 'R':  // set standby freq
    return 13;
  case 'Z':  // save feq. + name in memory
    return 14;
  default:
    return 2;
  }
}

bool
KRT2Device::PutFrequency(RadioFrequency frequency, const TCHAR *name,
                         char cmd)
{
  char mhz = frequency.GetKiloHertz() / 1000;
  char khz = (frequency.GetKiloHertz() % 1000) / 5;
  char szTmp[13] = {
      STX, cmd, mhz, khz,
      ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
      static_cast<char>(mhz ^ khz)   // checksum
  };
  ::CopyASCII(szTmp + 4, 8, name, name + _tcslen(name));
  port.Write(szTmp, sizeof(szTmp));
  return true;
}

bool
KRT2Device::ParseFrame(struct NMEAInfo &info)
{
  bool result = false;

  switch (input_buffer[1]) {
  case 'A':  // set volume, squelch, intercom-VOX
    if (input_buffer[3] + input_buffer[4] == input_buffer[5]) {
      received_squelch = true;
      squelch = input_buffer[3];
      vox = input_buffer[4];
      info.settings.ProvideVolume(input_buffer[2] * 5 - 1, info.clock);
      result = true;
    }
    break;
  case 'U':  // active frequency changed
  case 'R':  // standby frequency changed
    {
      char mhz = input_buffer[2];
      char khz = input_buffer[3];
      char crc = input_buffer[12];
      if ((mhz ^ khz) == crc) {
        RadioFrequency rf = DecodeFrequency(mhz, khz);
        // TODO: store frequency in info
        result = rf.IsDefined();
      }
    }
    break;
  case 'C':
    // TODO: exchange stored active and passive frequency
  default:
    // ignore this frame
    break;
  }
  return result;
}

bool
KRT2Device::PutVolume(unsigned volume, OperationEnvironment &env)
{
  if (!received_squelch)
    return false;  // don't want to override squelch and VOX_ settings

  assert(volume <= 100);
  char szTmp[6] = {
    STX, 'A',
    // 0..4 -> 1, 5..9 -> 2, ..., 95..100->20
    static_cast<char>(std::min(volume / 5 + 1, 20u)),  // volume 1..20
    squelch,       // squelch 1..10
    vox,           // VOX_ 1..10
    static_cast<char>(squelch + vox)  // checksum
  };
  port.Write(szTmp, sizeof(szTmp));
  return true;
}

bool
KRT2Device::PutActiveFrequency(RadioFrequency frequency,
                               const TCHAR *name,
                               OperationEnvironment &env)
{
  return PutFrequency(frequency, name, 'U');
}

bool
KRT2Device::PutStandbyFrequency(RadioFrequency frequency,
                                const TCHAR *name,
                                OperationEnvironment &env)
{
  return PutFrequency(frequency, name, 'R');
}

bool
KRT2Device::DataReceived(const void *_data, size_t length,
                         struct NMEAInfo &info)
{
  assert(length > 0);

  const char *data = static_cast<const char *>(_data);
  const char *end = data + length;
  bool result = false;

  unsigned expected_size = 0;
  do {
    if (!input_buffer.empty()) {
      input_buffer.append(*data);
      if (!expected_size)
        expected_size = FrameSize(input_buffer[1]);

      if (input_buffer.size() == expected_size) {
        // frame complete
        result |= ParseFrame(info);
        input_buffer.clear();
      } else if (input_buffer.full()) {
        // too much data (will never happen when buffer >= max(expected_size))
        input_buffer.clear();
      }
    } else if (*data == SYNC) {
      // reply to SYNC from radio
      port.Write(SYNC_ACK);
    } else if (*data == STX) {
      // found start of new frame
      input_buffer.append(*data);
      expected_size = 0;
    } else if (*data == ACK) {
      // previous command accepted
    } else if (*data == NAK) {
      // previous command rejected
    }
  } while (++data != end);

  return result;
}

static Device *
KRT2CreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new KRT2Device(com_port);
}

const struct DeviceRegister krt2_driver = {
  _T("KRT2"),
  _T("KRT2"),
  DeviceRegister::NO_TIMEOUT |
  DeviceRegister::SEND_SETTINGS |
  DeviceRegister::RECEIVE_SETTINGS |
  DeviceRegister::RAW_GPS_DATA,
  KRT2CreateOnPort,
};
