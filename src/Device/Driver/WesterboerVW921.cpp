/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Device/Driver/WesterboerVW921.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "Util/FifoBuffer.hpp"
#include "OS/ByteOrder.hpp"
#include "Units/System.hpp"

#include <tchar.h>
#include <stdio.h>
#include <stdint.h>

/**
 * Device driver for Westerboer VW921 and VW922.
 */
class WesterboerVW921Device : public AbstractDevice
{
  FifoBuffer<char, 256u> buffer;

public:
  virtual bool DataReceived(const void *data, size_t length,
                            struct NMEAInfo &info) override;

  void SentenceReceived(unsigned sentence_number,
                        const void *data, size_t length, struct NMEAInfo &info);

  void SentenceZero(const void *data, size_t length, struct NMEAInfo &info);
  void SentenceOne(const void *data, size_t length, struct NMEAInfo &info);
  void SentenceTwo(const void *data, size_t length, struct NMEAInfo &info);
};

static uint8_t
CalculateChecksum(const void *_data, size_t length)
{
  const uint8_t *data = (const uint8_t *)_data;

  uint8_t checksum = 0;
  for (unsigned i = 0; i < length; ++i, ++data)
    checksum ^= *data;

  return checksum;
}

static bool
CheckChecksum(const void *data, size_t length)
{
  return CalculateChecksum(data, length) == 0;
}

bool
WesterboerVW921Device::DataReceived(const void *_data, size_t length,
                                    struct NMEAInfo &info)
{
  assert(_data != NULL);
  assert(length > 0);

  bool result = false;

  const char *data = (const char *)_data, *end = data + length;

  do {
    // append new data to buffer, as much as fits there
    auto range = buffer.Write();
    if (range.IsEmpty()) {
      // overflow: reset buffer to recover quickly
      buffer.Clear();
      continue;
    }

    size_t nbytes = std::min(size_t(range.size), size_t(end - data));
    memcpy(range.data, data, nbytes);
    data += nbytes;
    buffer.Append(nbytes);

    while (true) {
      // read data from the buffer, to see if there's a dollar character
      range = buffer.Read();
      if (range.IsEmpty())
        break;

      // Search for the dollar sign (sync byte)
      char *dollar = (char *)memchr(range.data, '$', range.size);
      if (dollar == NULL)
        // no dollar sign here: wait for more data
        break;

      // Make sure there are at least 5 bytes in the buffer to
      // read the sentence header
      unsigned remaining_length = range.size - (dollar - range.data);
      if (remaining_length < 5)
        break;

      // Check this is a Westerboer sentence
      if (dollar[1] != 'w') {
        // Skip this sentence
        buffer.Consume(dollar - range.data + 1);
        continue;
      }

      // Read the length of the sentence
      uint8_t sentence_length = (uint8_t)dollar[2];

      // Check if the sentence was completely received already
      if (remaining_length < sentence_length)
        break;

      if (!CheckChecksum(dollar, sentence_length)) {
        // Skip this sentence
        buffer.Consume(dollar - range.data + 1);
        continue;
      }

      // Read the sentence identification number
      uint8_t sentence_number = (uint8_t)dollar[4];

      // Parse the sentence
      SentenceReceived(sentence_number, dollar, sentence_length, info);

      buffer.Consume(dollar - range.data + sentence_length);

      result = true;
    }
  } while (data < end);

  return result;
}

static float
ReadFloat(const void *data)
{
  // Read the 4 big endian bytes from the buffer
  uint32_t value = ReadUnalignedBE32((const uint32_t *)data);

  // Convert to little endian
  value = ToLE32(value);

  // Extract the components
  uint32_t exponent = (value & 0xFF000000) >> 24;
  bool sign = (value & 0x00800000) != 0;
  uint32_t mantisse = (value & 0x007FFFFF);

  // Adjust exponent offset
  exponent -= sign ? 1 : 2;

  // Assemble the result
  uint32_t result = mantisse | (exponent << 23);
  if (sign)
    result = result | 0x80000000;

  // Convert little endian to system
  result = FromLE32(result);

  // Return result as float
  return *((float *)&result);
}

void
WesterboerVW921Device::SentenceReceived(unsigned sentence_number,
                                        const void *data, size_t length,
                                        struct NMEAInfo &info)
{
  info.alive.Update(info.clock);

  switch (sentence_number) {
  case 0:
    SentenceZero(data, length, info);
    break;
  case 1:
    SentenceOne(data, length, info);
    break;
  case 2:
    SentenceTwo(data, length, info);
    break;
  default:
    break;
  }
}

void
WesterboerVW921Device::SentenceZero(const void *_data, size_t length,
                                    struct NMEAInfo &info)
{
  const uint8_t *data = (const uint8_t *)_data;

  /*
  Received sentence #0 with length = 45
  24 77 2d 3c 00 10 82 07   c8 6c 28 00 00 bd ff 46
  00 04 02 00 00 46 00 00   00 12 00 01 00 00 00 00
  00 3f 0b 80 62 d5 b1 7d   5a 0d cd a6 ff
  */

  // 0 - 4        Header
  // 24 77 2d 3c 00

  // 5      Byte  Flight status (Bitmask)
  // 6      Byte  Flight status 2 (Bitmask)
  // Bit 7 : External Switch (1=Climbing, 0=Cruise)

  uint8_t flight_status2 = *(data + 6);
  if ((flight_status2 & (1 << 7)) == 0) {
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  } else {
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
  }

  // 7      Byte  GPS status (Bitmask)
  // Bit 3: GPS-Status-Flag ("A"-valid position/"V"- NAV receiver warning)
  // 8      Byte  GPS status 2 (Bitmask)

  // 9     Word  Flight time (sec)
  // 11     Word  Distance to next target (0.1 km)
  // 6c 28 00 00

  // 13     Int   STD altitude (m)
  // 15     Int   QFE-Höhe h (m)
  // b8 ff 00 00

  int16_t std_altitude =
      ReadUnalignedLE16((const uint16_t *)(const void *)(data + 13));
  info.ProvidePressureAltitude(fixed(std_altitude));

  // 17     Int   Windempfehlung (km/ h)
  // 19     Int   delta speed (km/ h)
  // 21     Int   Gleitpath deviation (m)
  // 00 00 00 00 00 00

  // 23     Int   avg. Vario (0,1 m/s)
  // 25     Int   Nettovario (0,1 m/s)
  // 27     Int   Vario (0,1 m/s)
  // 00 00 11 00 f7 ff

  int16_t netto_vario =
      ReadUnalignedLE16((const uint16_t *)(const void *)(data + 25));
  info.ProvideNettoVario(fixed(netto_vario) / 10);

  int16_t vario =
      ReadUnalignedLE16((const uint16_t *)(const void *)(data + 27));
  info.ProvideTotalEnergyVario(fixed(vario) / 10);

  // 29     Int   True Air Speed TAS (0,1 m/s)
  // 31     Int   Groundspeed GS (0,1 m/s)
  // 00 00 e6 00

  // Flight tests by Kimmo Hytönen have shown that the Westerboer documentation
  // is wrong and that the TAS is actually transmitted in 0.1 km/h instead.
  int16_t tas =
      ReadUnalignedLE16((const uint16_t *)(const void *)(data + 29));
  info.ProvideTrueAirspeed(Units::ToSysUnit(fixed(tas) / 10,
                                            Unit::KILOMETER_PER_HOUR));

  int16_t ground_speed =
      ReadUnalignedLE16((const uint16_t *)(const void *)(data + 31));

  info.ground_speed = fixed(ground_speed) / 10;
  info.ground_speed_available.Update(info.clock);

  // Next 12 bytes are zero if GPS connection is bad:
  // 33     Int   Track (0.1 deg)
  // eb 03
  // 35     Float Latitude (arc, -pi/2 .. +pi/2)
  // 80 62 d6 ce = N504606
  // 39     Float Longitude (arc, -pi .. +pi)
  // 7d 59 e4 01 = E 60601
  // 43     Int   DiffAngle (0.1 deg)
  // 62 fd

  uint8_t gps_status = *(data + 7);
  if ((gps_status & (1 << 3)) != 0) {
    // GPS has a valid fix

    int16_t track =
        ReadUnalignedLE16((const uint16_t *)(const void *)(data + 31));

    info.track = Angle::Degrees(fixed(track) / 10);
    info.track_available.Update(info.clock);

    info.location.latitude = Angle::Radians(fixed(ReadFloat(data + 35)));
    info.location.longitude = Angle::Radians(fixed(ReadFloat(data + 39)));

    info.location_available.Update(info.clock);
  }
}

void
WesterboerVW921Device::SentenceOne(const void *_data, size_t length,
                                   struct NMEAInfo &info)
{
  const uint8_t *data = (const uint8_t *)_data;

  // Sentence one exists in multiple versions based on firmware version
  if (length == 31) {
    // According to test device

    fixed wing_loading = fixed(ReadFloat(data + 13));
    info.settings.ProvideWingLoading(wing_loading, info.clock);

    uint8_t polar_type = *(data + 30);
    if (polar_type == 0)
      info.settings.ProvideBugs(fixed(1), info.clock);
    else if (polar_type == 1)
      info.settings.ProvideBugs(fixed(0.85), info.clock);
    else if (polar_type == 2)
      info.settings.ProvideBugs(fixed(0.95), info.clock);

  } else if (length == 26) {
    // According to version 3.30 documentation

    int16_t wing_loading =
        ReadUnalignedLE16((const uint16_t *)(const void *)(data + 11));
    info.settings.ProvideWingLoading(fixed(wing_loading) / 10, info.clock);

    uint8_t polar_type = *(data + 13);
    if (polar_type == 0)
      info.settings.ProvideBugs(fixed(1), info.clock);
    else if (polar_type == 1)
      info.settings.ProvideBugs(fixed(0.95), info.clock);
    else if (polar_type == 2)
      info.settings.ProvideBugs(fixed(0.85), info.clock);
  }
}

void
WesterboerVW921Device::SentenceTwo(const void *_data, size_t length,
                                   struct NMEAInfo &info)
{
  const uint8_t *data = (const uint8_t *)_data;

  uint8_t mc_value = *(data + 13);
  info.settings.ProvideMacCready(fixed(mc_value) / 2, info.clock);
}

static Device *
WesterboerVW921CreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new WesterboerVW921Device();
}

const struct DeviceRegister westerboer_vw921_driver = {
  _T("VW921"),
  _T("Westerboer VW921/VW922"),
  DeviceRegister::RAW_GPS_DATA | DeviceRegister::RECEIVE_SETTINGS,
  WesterboerVW921CreateOnPort,
};
