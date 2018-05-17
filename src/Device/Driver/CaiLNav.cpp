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

#include "Device/Driver/CaiLNav.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Operation/Operation.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"

static void
FormatLatitude(char *buffer, size_t buffer_size, Angle latitude )
{
  // Calculate Latitude sign
  char sign = latitude.IsNegative() ? 'S' : 'N';

  double mlat(latitude.AbsoluteDegrees());

  int dd = (int)mlat;
  // Calculate minutes
  double mins = (mlat - dd) * 60.0;

  // Save the string to the buffer
  snprintf(buffer, buffer_size, "%02d%06.3f,%c", dd, mins, sign);
}

static void
FormatLongitude(char *buffer, size_t buffer_size, Angle longitude)
{
  // Calculate Longitude sign
  char sign = longitude.IsNegative() ? 'W' : 'E';

  double mlong(longitude.AbsoluteDegrees());

  int dd = (int)mlong;
  // Calculate minutes
  double mins = (mlong - dd) * 60.0;
  // Save the string to the buffer
  snprintf(buffer, buffer_size, "%02d%06.3f,%c", dd, mins, sign);
}

static void
PortWriteNMEANoChecksum(Port &port, const char *line,
                        OperationEnvironment &env)
{
  // reasonable hard-coded timeout; Copied from ::PortWriteNMEA()
  const unsigned timeout_ms = 1000;

  port.FullWrite(line, strlen(line), env, timeout_ms);
}

/*
$GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,*hh<CR><LF>

<1>  UTC Time of position fix, hhmmss format
<2>  Status, A = Valid position, V = NAV receiver warning
<3>  Latitude,ddmm.mmm format (leading zeros will be transmitted)
<4>  Latitude hemisphere, N or S
<5>  Longitude,dddmm.mmm format (leading zeros will be transmitted)
<6>  Longitude hemisphere, E or W
<7>  Speed over ground, 0.0 to 999.9 knots
<8>  Course over ground 000.0 to 359.9 degrees, true
     (leading zeros will be transmitted)
<9>  UTC date of position fix, ddmmyy format
<10> Magnetic variation, 000.0 to 180.0 degrees
     (leading zeros will be transmitted)
<11> Magnetic variation direction, E or W
     (westerly variation adds to true course)
*/
static bool
FormatGPRMC(char *buffer, size_t buffer_size, const MoreData &info)
{
  char lat_buffer[20];
  char long_buffer[20];

  const GeoPoint location = info.location_available
    ? info.location
    : GeoPoint::Zero();

  FormatLatitude(lat_buffer, sizeof(lat_buffer), location.latitude);
  FormatLongitude(long_buffer, sizeof(long_buffer), location.longitude);

  const BrokenDateTime now = info.time_available &&
    info.date_time_utc.IsDatePlausible()
    ? info.date_time_utc
    : BrokenDateTime::NowUTC();

  snprintf(buffer, buffer_size,
           "GPRMC,%02u%02u%02u,%c,%s,%s,%05.1f,%05.1f,%02u%02u%02u,,",
           now.hour, now.minute, now.second,
           info.location.IsValid() ? 'A' : 'V',
           lat_buffer,
           long_buffer,
           (double)Units::ToUserUnit(info.ground_speed, Unit::KNOTS),
           (double)info.track.Degrees(),
           now.day, now.month, now.year % 100);

  return true;
}

/*
$GPRMB,<1>,,,,<5>,,,,,<10>,<11>,,<13>*hh<CR><LF>

<1>  Position Valid (A = valid, V = invalid)
<5>  Destination waypoint identifier, three digits
     (leading zeros will be transmitted)
<10> Range from present position to distination waypoint, format XXXX.X,
     nautical miles (leading zeros will be transmitted)
<11> Bearing from present position to destination waypoint, format XXX.X,
     degrees true (leading zeros will be transmitted)
<13> Arrival flag <A = arrival, V = not arrival)
*/
static bool
FormatGPRMB(char *buffer, size_t buffer_size, const GeoPoint& here,
            const AGeoPoint &destination)
{
  if (!here.IsValid() || !destination.IsValid())
    return false;

  const GeoVector vector(here, destination);
  const bool has_arrived = vector.distance < 1000; // < 1km ?

  snprintf(buffer, buffer_size, "GPRMB,%c,,,,,,,,,%06.1f,%04.1f,%c",
           here.IsValid() ? 'A' : 'V',
           (double)Units::ToUserUnit(vector.distance, Unit::NAUTICAL_MILES),
           (double)vector.bearing.Degrees(),
           has_arrived ? 'A' : 'V');

  return true;
}

/*
$PCAIB,<1>,<2>,<CR><LF>

<1>  Destination waypoint elevation in meters, format XXXXX
     (leading zeros will be transmitted)
     <2>  Destination waypoint  attribute word, format XXXXX
     (leading zeros will be transmitted)
*/
static bool
FormatPCAIB(char *buffer, size_t buffer_size, const AGeoPoint& destination)
{
  if (!destination.IsValid())
    return false;

  // Generic waypoint.
  unsigned flags = 1 << 8;

  snprintf(buffer, buffer_size, "$PCAIB,%04d,%04u\r\n",
           (int)destination.altitude,
           flags);

  return true;
}

/*
$PCAIC,<1>,<2>,<3>,<CR><LF>

<1>  Length of final leg of task, tenths of km
     (leading zeros will be transmitted)
<2>  Course of final leg of task, degrees true
     (leading zeros will be transmitted)
<3>  Elevation of the last task point, meters
     (leading zeros will be transmitted)

The fields <1>,<2>, and <3> are valid only if field <1> is non zero.
All these conditions must be met for field <1> to be non zero:

1. A task is chosen.
2. The current leg is the second to last leg.
3. The active point is the second to last task point.
*/

class CaiLNavDevice final : public AbstractDevice {
  Port &port;

public:
  CaiLNavDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};

void
CaiLNavDevice::OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated)
{
  NullOperationEnvironment env;
  char buffer[100];

  const GeoPoint here = basic.location_available
    ? basic.location
    : GeoPoint::Invalid();

  const ElementStat &current_leg = calculated.task_stats.current_leg;
  AGeoPoint destination =
    AGeoPoint(current_leg.location_remaining,
              current_leg.solution_planned.min_arrival_altitude);

  if (FormatGPRMC(buffer, sizeof(buffer), basic))
    PortWriteNMEA(port, buffer, env);

  if (FormatGPRMB(buffer, sizeof(buffer), here, destination))
    PortWriteNMEA(port, buffer, env);

  if (FormatPCAIB(buffer, sizeof(buffer), destination))
    PortWriteNMEANoChecksum(port, buffer, env);
}

static Device *
CaiLNavCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new CaiLNavDevice(com_port);
}

const struct DeviceRegister cai_lnav_driver = {
  _T("cai_lnav"),
  _T("Cambridge L-Nav"),
  DeviceRegister::NO_TIMEOUT,
  CaiLNavCreateOnPort,
};
