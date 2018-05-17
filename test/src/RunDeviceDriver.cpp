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

#include "NMEA/Info.hpp"
#include "Device/Port/NullPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Device/Config.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Input/InputEvents.hpp"
#include "OS/Args.hpp"
#include "Util/ConvertString.hpp"

#include <stdio.h>

const struct DeviceRegister *driver;

static void
Dump(GeoPoint location)
{
  int latitude = (int)(location.latitude.Degrees() * 3600);
  char north_or_south = latitude < 0 ? 'S' : 'N';
  latitude = abs(latitude);

  int longitude = (int)(location.longitude.Degrees() * 3600);
  char east_or_west = latitude < 0 ? 'W' : 'E';
  longitude = abs(longitude);

  printf("%d.%02u.%02u%c;%d.%02u.%02u%c",
         latitude / 3600, (latitude / 60) % 60, latitude % 60, north_or_south,
         longitude / 3600, (longitude / 60) % 60, longitude % 60, east_or_west);
}

static void
Dump(const ExternalSettings &settings)
{
  if (settings.mac_cready_available)
    printf("MacCready=%.1f\n", (double)settings.mac_cready);

  if (settings.ballast_fraction_available)
    printf("Ballast=%.1f\n", (double)settings.ballast_fraction);

  if (settings.bugs_available)
    printf("Bugs=%.1f\n", (double)settings.bugs);

  if (settings.qnh_available)
    printf("QNH=%f\n", (double)settings.qnh.GetHectoPascal());
}

static void
Dump(const NMEAInfo &basic)
{
  if (basic.date_time_utc.IsDatePlausible())
    printf("Date=%02u.%02u.%04u\n",
           basic.date_time_utc.day, basic.date_time_utc.month, basic.date_time_utc.year);

  if (basic.time_available)
    printf("Time=%02u:%02u:%02u\n",
           basic.date_time_utc.hour, basic.date_time_utc.minute, basic.date_time_utc.second);

  if (!basic.alive)
    printf("GPS not connected\n");
  else if (!basic.gps.satellites_used_available)
    printf("GPS connected\n");
  else
    printf("GPS connected, %d satellites\n", basic.gps.satellites_used);

  if (basic.location_available) {
    printf("Position=");
    Dump(basic.location);
    printf("\n");
  }

  if (basic.track_available)
    printf("TrackBearing=%d\n", (int)basic.track.Degrees());

  if (basic.ground_speed_available)
    printf("GroundSpeed=%d\n", (int)basic.ground_speed);

  if (basic.airspeed_available) {
    printf("TrueAirspeed=%d\n", (int)basic.true_airspeed);
    printf("IndicatedAirspeed=%d\n",
           (int)basic.indicated_airspeed);
  }

  if (basic.gps_altitude_available)
    printf("GPSAltitude=%d\n", (int)basic.gps_altitude);

  if (basic.static_pressure_available)
    printf("StaticPressure=%f hPa\n",
           (double)basic.static_pressure.GetHectoPascal());

  if (basic.pressure_altitude_available)
    printf("PressureAltitude=%d\n", (int)basic.pressure_altitude);

  if (basic.baro_altitude_available)
    printf("BaroAltitude=%d\n", (int)basic.baro_altitude);

  if (basic.total_energy_vario_available)
    printf("TotalEnergyVario=%.1f\n", (double)basic.total_energy_vario);

  if (basic.netto_vario_available)
    printf("NettoVario=%.1f\n", (double)basic.netto_vario);

  if (basic.external_wind_available)
    printf("Wind=%d/%d\n",
           (int)basic.external_wind.bearing.Degrees(),
           (int)basic.external_wind.norm);

  if (basic.temperature_available)
    printf("OutsideAirTemperature=%d\n", (int)basic.temperature.ToKelvin());

  if (basic.humidity_available)
    printf("RelativeHumidity=%d\n", (int)basic.humidity);

  const DeviceInfo &device = basic.device;
  if (!device.product.empty())
    printf("Device.Product=%s\n", device.product.c_str());
  if (!device.serial.empty())
    printf("Device.Serial=%s\n", device.serial.c_str());
  if (!device.hardware_version.empty())
    printf("Device.HardwareVersion=%s\n", device.hardware_version.c_str());
  if (!device.software_version.empty())
    printf("Device.SoftwareVersion=%s\n", device.software_version.c_str());

  const DeviceInfo &device2 = basic.secondary_device;
  if (!device2.product.empty())
    printf("SecondaryDevice.Product=%s\n", device2.product.c_str());
  if (!device2.serial.empty())
    printf("SecondaryDevice.Serial=%s\n", device2.serial.c_str());
  if (!device2.hardware_version.empty())
    printf("SecondaryDevice.HardwareVersion=%s\n",
           device2.hardware_version.c_str());
  if (!device2.software_version.empty())
    printf("SecondaryDevice.SoftwareVersion=%s\n",
           device2.software_version.c_str());

  const FlarmData &flarm = basic.flarm;
  if (flarm.status.available) {
    printf("FLARM rx=%u tx=%u\n", flarm.status.rx, flarm.status.tx);
    printf("FLARM gps=%u\n", (unsigned)flarm.status.gps);
    printf("FLARM alarm=%u\n", (unsigned)flarm.status.alarm_level);
    printf("FLARM traffic=%zu\n", flarm.traffic.list.size());
  }

  if (basic.engine_noise_level_available)
    printf("ENL=%u\n", basic.engine_noise_level);

  if (basic.voltage_available)
    printf("Battery=%fV\n", (double)basic.voltage);

  if (basic.battery_level_available)
    printf("Battery=%f%%\n", (double)basic.battery_level);

  Dump(basic.settings);
}

int main(int argc, char **argv)
{
  NarrowString<1024> usage;
  usage = "DRIVER\n\n"
          "Where DRIVER is one of:";
  {
    const DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != nullptr; ++i) {
      WideToUTF8Converter driver_name(driver->name);
      usage.AppendFormat("\n\t%s", (const char *)driver_name);
    }
  }

  Args args(argc, argv, usage);
  tstring driver_name = args.ExpectNextT();
  args.ExpectEnd();

  driver = FindDriverByName(driver_name.c_str());
  if (driver == nullptr) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return 1;
  }

  DeviceConfig config;
  config.Clear();

  NullPort port;
  Device *device = driver->CreateOnPort != nullptr
    ? driver->CreateOnPort(config, port)
    : nullptr;

  NMEAParser parser;

  NMEAInfo data;
  data.Reset();

  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), stdin) != nullptr) {
    StripRight(buffer);

    if (device == nullptr || !device->ParseNMEA(buffer, data))
      parser.ParseLine(buffer, data);
  }

  Dump(data);

  return EXIT_SUCCESS;
}
