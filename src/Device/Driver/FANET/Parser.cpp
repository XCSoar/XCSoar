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

#include "Device.hpp"
#include "NMEA/Info.hpp"
#include "FANET/FanetAddress.hpp"

bool
FanetDevice::AddStationToList(StationsList &stations_list, double clock,
                              const FanetStation station) {
  FanetStation *fanet_slot = stations_list.FindStations(station.address);

  if (fanet_slot == nullptr) {
    fanet_slot = stations_list.AllocateStation();
    if (fanet_slot == nullptr)
      // no more slots available
      return false;

    fanet_slot->Clear();
    fanet_slot->address = station.address;
    fanet_slot->location = station.location;
    fanet_slot->location_available = station.location_available;
  }

  fanet_slot->valid.Update(clock);
  fanet_slot->Update(station);

  return true;
}

bool
FanetDevice::IntegrityCheck(char *cmd_str) {
  if (strlen(cmd_str) == 0)
    return false;

  for(char *ptr = cmd_str; *ptr != '\0'; ptr++)
  {
    if((*ptr >= '0' && *ptr <= '9') ||
       (*ptr >= 'A' && *ptr <= 'F') ||
       (*ptr >= 'a' && *ptr <= 'f') ||
       (*ptr == ',')) {
      continue;
    }
    return false;
  }
  return true;
}

char *
FanetDevice::RemoveExtraChars(char *cmd_str) {
  char *ptr = strchr(cmd_str, '\r');
  if(ptr == nullptr)
    ptr = strchr(cmd_str, '\n');
  if(ptr != nullptr)
    *ptr = '\0';
  while(*cmd_str == ' ')
    cmd_str++;
  return cmd_str;
}

GeoPoint
FanetDevice::Payload2GeoPoint(const uint8_t *buf) {
  if(buf == nullptr)
    return GeoPoint::Invalid();

  // Integer values
  unsigned int lati = buf[2] << 16 | buf[1] << 8 | buf[0];
  if(lati & 0x00800000)
    lati |= 0xFF000000;
  unsigned int loni = buf[5] << 16 | buf[4] << 8 | buf[3];
  if(loni & 0x00800000)
    loni |= 0xFF000000;

  return GeoPoint(Angle::Degrees((float)loni / 46603.0f), Angle::Degrees((float)lati / 93206.0f));
}

float
FanetDevice::Payload2Float(uint8_t byte, float scale) {
  auto value = (float)(byte & 0x7F);
  if(byte & 1 << 7)
    return (value * scale);
  else
    return value;
}

bool
FanetDevice::DecodeServicePayload(uint8_t *payload, int16_t payload_length, FanetStation *station) {
  if(payload == nullptr || payload_length == 0)
    return false;

  uint16_t payload_pos = 1;

  // Skip extended header
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_EHEADER))
  {
    // TODO: decode extended header byte if set, currently not needed/used
    payload_pos++;
  }

  // Return if no location
  if(payload_pos + 6 > payload_length)
    return false;

  station->location = Payload2GeoPoint(&payload[payload_pos]);
  if( ! station->location.IsValid())
    return false;
  station->location_available = true;
  payload_pos += 6;

  /*
   * Temperature (+1byte in 0.5 degree, 2-Complement)
   */
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_TEMP))
  {
    if(payload_pos + 1 > payload_length)
      return false;
    station->temperature = (float)payload[payload_pos++] / 2.0f;
  }

  /*
   * 	Wind (+3byte: 1byte Heading in 360/256 degree, 1byte speed and 1byte
	 *  gusts in 0.2km/h (each: bit 7 scale 5x or 1x, bit 0-6))
   */
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_WIND))
  {
    if(payload_pos + 3 > payload_length)
      return false;

    station->wind_dir_deg = (((float)payload[payload_pos++]) / 0.7083333334f);
    station->wind_speed_kmph =
        Payload2Float(payload[payload_pos++], 5.0f) * 0.2f;
    station->wind_gust_kmph = Payload2Float(payload[payload_pos++], 5.0f) * 0.2f;
  }

  /*
   * Humidity (+1byte: in 0.4% (%rh*10/4))
   */
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_HUMIDITY))
  {
    if(payload_pos + 1 > payload_length)
      return false;
    station->rel_humidity = payload[payload_pos++] / 2.5f;
  }

  /*
   * Barometric pressure normailized (+2byte: in 10Pa, offset by 430hPa,
   * unsigned little endian (hPa-430)*10)
   */
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_PRESSURE))
  {
    if(payload_pos + 2 > payload_length)
      return false;
    int lsb = payload[payload_pos++];
    int msb = payload[payload_pos++];
    station->preasure_hpa = (((msb << 8) | lsb) / 10.0) + 430.0;
  }

  /*
   * State of Charge  +1byte
   * (lower 4 bits: 0x00 = 0%, 0x01 = 6.666%, .. 0x0F = 100%)
   */
  if(IsNthBitSet(payload[0], FANET_SERVICE_BIT_SOC))
  {
    if(payload_pos + 1 > payload_length)
      return false;
    station->soc_percent =
        Payload2Float(payload[payload_pos++], 1.0f) * 100.0f / 15.0f;
  }

  return true;
}

/*
 *  Service(type 4 - weather info):
 *  #FNF src_manufacturer,src_id,broadcast,signature,type,payloadlength,payload
 */
bool
FanetDevice::ParseRxPacket(char *cmd_str, StationsList &stations_list,
                           double clock) {
  // Prepare the line
  cmd_str = RemoveExtraChars(cmd_str);
  if ( ! IntegrityCheck(cmd_str))
    return false;

  char *start_ptr = (char *)cmd_str;
  char *end_ptr;

  if(! start_ptr)
    return false;

  // Manufacturer(1 byte)
  uint8_t manufacturer = strtol(start_ptr, &end_ptr, 16);
  if(start_ptr == end_ptr)
    return false;

  start_ptr = strchr(start_ptr, ',') + 1;  // comma after src_manufacturer
  if (start_ptr == nullptr)
    return false;

  // Id(2 bytes)
  uint16_t id = strtol(start_ptr, &end_ptr, 16);
  if(start_ptr == end_ptr)
    return false;

  /*
   * Skip broadcast(1 byte) and signature(2 bytes)
   *  comma after src_id
   *  comma after broadcast
   *  comma after signature
   */
  for (int i = 0; i < 3; ++i) {
    start_ptr = strchr(start_ptr, ',') + 1;
    if (start_ptr == nullptr)
      return false;
  }

  // Type(1 byte)
  uint8_t type = strtol(start_ptr, &end_ptr, 16);
  if(start_ptr == end_ptr)
    return false;

  start_ptr = strchr(start_ptr, ',') + 1;  // comma after type
  if (start_ptr == nullptr)
    return false;

  // Payload length(1 byte)
  uint16_t payload_length = strtol(start_ptr, &end_ptr, 16);
  if(start_ptr == end_ptr)
    return false;

  /*
   * Standard limitation of LoRa buffers of 256 bytes and the maximum length
   * of the FANET MAC header of 11 bytes reduces the permitted payload length
   * to 245 bytes.
   */
  if(payload_length > 245)
    return false;

  // Payload(payload_length bytes)
  auto* payload = new uint8_t[payload_length];
  start_ptr = strchr(start_ptr, ',') + 1;  // comma after payloadlength
  if (start_ptr == nullptr)
    return false;

  // Extract payload
  for(int i = 0; i < payload_length; i++)
  {
    char sstr[3] = {start_ptr[i * 2], start_ptr[i * 2 + 1], '\0'};
    if(strlen(sstr) != 2)
      return false;

    payload[i] = strtol(sstr,  &end_ptr,  16);
    if(start_ptr == end_ptr)
      return false;
  }

  switch (type)
  {
    // We only decode type 4 (service / weather data) for now
    case FANET_TYPE_SERVICE:
      FanetStation station;
      station.address = FanetAddress(manufacturer, id);
      if(DecodeServicePayload(payload, payload_length, &station))
        return AddStationToList(stations_list, clock, station);
      return false;
    default:
      return false;
  }
}

/*
 * https://github.com/3s1d/fanet-stm32/raw/master/fanet_module.pdf
 * https://github.com/3s1d/fanet-stm32/blob/master/Src/fanet/radio/protocol.txt
 */
bool
FanetDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if(StringIsEqual(line, "#FNF", strlen("#FNF"))) {
    return ParseRxPacket(const_cast<char *>(&line[strlen("#FNF") + 1]),
                         info.fanet.stations, info.clock);
  }

  return false;
}
