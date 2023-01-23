/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

/**
* see Documentation https://
*/

#include "Device/Driver/Anemoi.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "thread/Cond.hxx"
#include "thread/Mutex.hxx"
#include "util/CharUtil.hxx"
#include "util/StaticFifoBuffer.hxx"
#include "util/Compiler.h"
// #include "NMEA/Checksum.hpp"  // aug: or whereis it?
#include "Units/System.hpp"

#include <tchar.h>
#include <stdio.h>

using std::string_view_literals::operator""sv;

/**
 * Device driver for Anemoi Wind.
 * @see http://www..pdf
 */

#define WITH_PORT 0
class AnemoiDevice final : public AbstractDevice {
  // unused up to now: Port &port;

  static constexpr std::byte StartByte{'$'}; //!< Command start character.

  //! Expected length of the message just receiving.
  //  size_t expected_msg_length{};
  size_t expected_msg_length{};
  //! Buffer which receives the messages send from the radio.
  StaticFifoBuffer<std::byte, 256u> rx_buf;

  //! Mutex to be locked to access response.
  Mutex response_mutex;

private:
  /**
   * Calculates the length of the message just receiving.
   *
   * @param data Pointer to the first character of the message.
   * @param length Number of characters received.
   * @return Expected message length.
   */
  static size_t ExpectedMsgLength(const std::byte *data, size_t length);
  /**
   * Calculates the length of the command message just receiving.
   *
   * @param code Command code received after the '$' character.
   * @return Expected message length after the code character.
   */
  static size_t ExpectedMsgLengthSentence(std::byte code);

  /**
   * Handle the anemoi sentence.
   *
   */
  static bool HandleSentence(const std::byte *data, unsigned msg_size,
                                struct NMEAInfo & info);

  static int16_t ReadInteger16(const std::byte **data);
  static int8_t ReadByte(const std::byte **data);

  static bool ParseAttitude(const std::byte *data, struct NMEAInfo &info);
  static bool ParseWind(const std::byte *data, struct NMEAInfo &info);
  static bool ParseData(const std::byte *data, struct NMEAInfo &info);


public:
  AnemoiDevice([[maybe_unused]] Port &_port) {}
  // port is unused: AnemoiDevice(Port &_port) : port(_port) {}
  
  /* virtual methods from class Device */
  virtual bool DataReceived(std::span<const std::byte> s,
                            struct NMEAInfo &info) noexcept override;
};

bool
AnemoiDevice::DataReceived(std::span<const std::byte> s,
                                  struct NMEAInfo &info) noexcept
{
    assert(!s.empty());

    const auto *data = s.data();
    const auto *const end = data + s.size();
    do {
      // Append new data to the buffer, as much as fits in there
      auto range = rx_buf.Write();
      if (rx_buf.IsFull()) {
        // Overflow: reset buffer to recover quickly
        rx_buf.Clear();
        expected_msg_length = 0;
        continue;
      }
      size_t nbytes = size_t(end - data);
      if (nbytes > range.size())
        return true;  // do nothing, avoid buffer overflow
      // size_t nbytes = std::min(range.size(), size_t(end - data));
      std::copy_n(data, nbytes, range.begin());
      data += nbytes;
      rx_buf.Append(nbytes);

      for (;;) {
      // Read data from buffer to handle the messages
        range = rx_buf.Read();
        if (range.empty())
          break;

        if (range.size() < expected_msg_length)
          break;

        expected_msg_length = ExpectedMsgLength(range.data(), range.size());
        if (range.size() >= expected_msg_length) {
          switch (*(const std::byte *)range.data()) {
          case StartByte: // Startbyte
            {
              const std::lock_guard lock{response_mutex};
              HandleSentence(range.data(), expected_msg_length, info);
            }
            break;
          default:
            break;
          }
          // Message handled -> remove message
          rx_buf.Consume(expected_msg_length);
          expected_msg_length = 0;
          // Received something from the radio -> the connection is alive
          info.alive.Update(info.clock);
        }
      }

    } while (data < end);
    return true;
}

/**
  The expected length of a received message may change,
  when the first character is STX and the second character
  is not received yet.
*/
size_t
AnemoiDevice::ExpectedMsgLength(const std::byte *data,
                                             size_t length)
{
  size_t expected_length;

  assert(data != nullptr);
  assert(length > 0);

  if (data[0] == StartByte) {
    if (length > 1) {
      expected_length = ExpectedMsgLengthSentence(data[1]);
      if (expected_length > 0)
        expected_length += 4;
    } else {
      // minimum 2 chars
      expected_length = 4;
    }
  } else
    expected_length = 1;

  return expected_length;
}

size_t 
AnemoiDevice::ExpectedMsgLengthSentence(std::byte code)
{
  size_t expected_length;

  switch ((char)code) {
  case 'S':
    // Sensor health sentence
    expected_length = 2;
    break;
  case 'w':
  case 'W':
    // Wind sentence
    expected_length = 8;
    break;
  case 'a':
  case 'A':
    // 
    expected_length = 7;
    break;
  case 'd':
  case 'D':
    // 
    expected_length = 12;
    break;
  case 'M':
    //
    expected_length = 4;
    break;
  default:
    // Received unknown code
    expected_length = 0;
    break;
  }

  return expected_length;
}

/*
 * crc8.c
 *
 *
 *
 */
static const uint8_t CRC_TABLE[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31,
    0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, 0xE0, 0xE7, 0xEE, 0xE9,
    0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1,
    0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, 0xB7, 0xB0, 0xB9, 0xBE,
    0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16,
    0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, 0x89, 0x8E, 0x87, 0x80,
    0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8,
    0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, 0x19, 0x1E, 0x17, 0x10,
    0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F,
    0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, 0xAE, 0xA9, 0xA0, 0xA7,
    0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF,
    0xFA, 0xFD, 0xF4, 0xF3};

static uint8_t crc8ccitt(const void *data, size_t size) {

  uint8_t val = 0;
  uint8_t *pos = reinterpret_cast<uint8_t *>(const_cast<void *>(data));
  uint8_t *end = pos + size;
  while (pos < end) {
    val = CRC_TABLE[val ^ *pos];
    pos++;
  }
  return val;
}

int16_t
AnemoiDevice::ReadInteger16(const std::byte **data)
{
  // read 2 bytes
  uint16_t value =
    *reinterpret_cast<uint8_t *>(const_cast<std::byte *>((*data)++));
  value <<= 8;
  value |= 
    *reinterpret_cast<uint8_t *>(const_cast<std::byte *>((*data)++));

  return (int16_t)value;
}

int8_t
AnemoiDevice::ReadByte(const std::byte **data) 
{
  // read 1 byte
  return 
    *reinterpret_cast<int8_t *>(const_cast<std::byte *>((*data)++));
}

bool
AnemoiDevice::ParseAttitude(const std::byte *data, struct NMEAInfo & info)
{
// Attitude sentence
  auto value = ReadInteger16(&data);
  // Roll respect to Earth system - Phi [°] (i.e. +110)
  if (value >= -180 && value <= +180) {
    info.attitude.bank_angle_available.Update(info.clock);
    info.attitude.bank_angle = Angle::Degrees(value);
  }
  value = ReadByte(&data);
  // Pitch angle respect to Earth system - Theta [°] (i.e.+020)
  if (value >= -90 && value <= +90) {
    info.attitude.pitch_angle_available.Update(info.clock);
    info.attitude.pitch_angle = Angle::Degrees(value);
  }
  value = ReadInteger16(&data);
  // (True?) Heading
  if (value >= 0 && value <= 360) {
    info.attitude.heading = Angle::Degrees(value);
    info.attitude.heading_available.Update(info.clock);
  }

  value = ReadInteger16(&data);
  // Circle diameter - no vbariable in XCSoar up to now
  if (value >= 0 && value <= 1000) {
//    info.attitude.heading = Angle::Degrees(value);
//    info.attitude.heading_available.Update(info.clock);
  }

  return true;
}

bool
AnemoiDevice::ParseWind(const std::byte *data, struct NMEAInfo & info)
{
// Wind sentence
  // Live wind:
  bool data_valid = true;
  auto winddir = ReadInteger16(&data);
  if (winddir < 0 || winddir > +360) {
    data_valid = false;
  }
  uint8_t windspeed = ReadByte(&data);
//  if (windspeed < 0 || windspeed > 200) {
  if (windspeed > 200) {
    data_valid = false;
  }
  if (data_valid) {
    info.ProvideExternalWind(
        SpeedVector(Angle::Degrees(winddir),
                    Units::ToSysUnit(windspeed, Unit::KILOMETER_PER_HOUR)));
    // TODO(August2111): this is really a workaround: 
    //         for the update thread is needed...
    info.location_available.Update(info.clock);
  }

  // Average wind:
  data_valid = true;
  winddir = ReadInteger16(&data);
  if (winddir < 0 || winddir > +360) {
    data_valid = false;
  }
  windspeed = ReadByte(&data);
  // if (windspeed < 0 || windspeed > 200) {
  if (windspeed > 200) {
    data_valid = false;
  }
  if (data_valid) info.ProvideExternalInstantaneousWind(
      SpeedVector(Angle::Degrees(winddir),
      Units::ToSysUnit(windspeed, Unit::KILOMETER_PER_HOUR)));

  // Heading
  auto heading = ReadInteger16(&data);
  if (heading >= 0 && heading <= +360) {
    info.attitude.heading = Angle::Degrees(heading);
    info.attitude.heading_available.Update(info.clock);
  }

  return true; // No other parser necessary
}

bool
AnemoiDevice::ParseData(const std::byte *data, struct NMEAInfo & info)
{
// Data sentence
  // vGND (km/h):
  auto value = ReadInteger16(&data);
  if (value >= 0 && value < 500) {
    info.ground_speed = Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR);
    info.ground_speed_available.Update(info.clock);
  }

  // vTAS (km/h):
  value = ReadInteger16(&data);
  if (value >= 0 && value < 500) {
    info.true_airspeed = Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR);
    info.airspeed_available.Update(info.clock);
    info.airspeed_real = true;
  }

  // track (deg):
  value = ReadInteger16(&data);
  if (value >= 0 && value < 500) {
    info.track = Angle::Degrees(value);
    info.track_available.Update(info.clock);
  }

  // heading (deg):
  value = ReadInteger16(&data);
  if (value >= 0 && value <= +360) {
    info.attitude.heading = Angle::Degrees(value);
    info.attitude.heading_available.Update(info.clock);
  }

  // Temperature (°C):
  value = ReadByte(&data);
  if (value >= -50 && value <= +100) {
    info.temperature.FromCelsius(value);
    info.temperature_available = true;
  }

  // Pitot calibration (%):
  value = ReadByte(&data);
  if (value >= 0 && value <= 100) {
    // info.attitude.heading = Angle::Degrees(value);
    // info.attitude.heading_available.Update(info.clock);
  }

  // FL:
  value = ReadInteger16(&data);
  if (value >= 0 && value <= 300) {
    info.baro_altitude = Units::ToSysUnit(value, Unit::FLIGHT_LEVEL);
    info.baro_altitude_available.Update(info.clock);
    // info.attitude.heading = Angle::Degrees(value);
    // info.attitude.heading_available.Update(info.clock);
  }

  return true; // No other parser necessary
}

bool
AnemoiDevice::HandleSentence(const std::byte *data, unsigned msg_size,
                               struct NMEAInfo & info)
{
  if (crc8ccitt((const void *)data, msg_size)) {
  printf("ERROR\n");
  return false;
}
  if (*data++ != StartByte)
    return false;

  switch (*data++) {
    // Attitude sentence
    case std::byte('a'):
    case std::byte('A'): 
      return ParseAttitude(data, info);
    // Wind sentence:
    case std::byte('w'):
    case std::byte('W'):
      return ParseWind(data, info);
    // Data sentence:
    case std::byte('d'):
    case std::byte('D'):
      return ParseData(data, info);
  default:
      return false;
      //break;
  }
}


static Device *
AnemoiCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new AnemoiDevice(com_port);
}

const struct DeviceRegister anemoi_driver = {
  _T("Anemoi"),
  _T("Anemoi"),
//  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  DeviceRegister::NO_TIMEOUT | DeviceRegister::RAW_GPS_DATA,
  AnemoiCreateOnPort,
};
