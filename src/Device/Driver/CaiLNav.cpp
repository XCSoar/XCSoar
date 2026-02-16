// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/CaiLNav.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Operation/Operation.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "Formatter/NMEAFormatter.hpp"
#include "util/SpanCast.hxx"

static void
PortWriteNMEANoChecksum(Port &port, const char *line,
                        OperationEnvironment &env)
{
  // reasonable hard-coded timeout; Copied from ::PortWriteNMEA()
  constexpr auto timeout = std::chrono::seconds(1);

  port.FullWrite(AsBytes(std::string_view{line}), env, timeout);
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

  FormatGPRMC(buffer, sizeof(buffer), basic);
  PortWriteNMEA(port, buffer, env);

  if (FormatGPRMB(buffer, sizeof(buffer), here, destination))
    PortWriteNMEA(port, buffer, env);

  if (FormatPCAIB(buffer, sizeof(buffer), destination))
    PortWriteNMEANoChecksum(port, buffer, env);
}

static Device *
CaiLNavCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new CaiLNavDevice(com_port);
}

const struct DeviceRegister cai_lnav_driver = {
  "cai_lnav",
  "Cambridge L-Nav",
  DeviceRegister::NO_TIMEOUT,
  CaiLNavCreateOnPort,
};
