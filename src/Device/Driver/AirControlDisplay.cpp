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

#include "Device/Driver/AirControlDisplay.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/MoreData.hpp"
#include "Operation/Operation.hpp"
#include "Atmosphere/Pressure.hpp"
#include "RadioFrequency.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

static bool
ParsePAAVS(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "ALT")) {
    /*
    $PAAVS,ALT,<ALTQNE>,<ALTQNH>,<QNH>
     <ALTQNE> Current QNE altitude in meters with two decimal places
     <ALTQNH> Current QNH altitude in meters with two decimal places
     <QNH> Current QNH setting in pascal (unsigned integer (e.g. 101325))
    */
    if (line.ReadChecked(value))
      info.ProvidePressureAltitude(value);

    if (line.ReadChecked(value))
      info.ProvideBaroAltitudeTrue(value);

    if (line.ReadChecked(value)) {
      auto qnh = AtmosphericPressure::Pascal(value);
      info.settings.ProvideQNH(qnh, info.clock);
    }
  } else {
    // ignore responses from COM and XPDR
    return false;
  }

  return true;
}

class ACDDevice : public AbstractDevice {
  Port &port;

public:
  ACDDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};

bool
ACDDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  char buffer[100];
  unsigned qnh = uround(pres.GetPascal());
  sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnh);
  return PortWriteNMEA(port, buffer, env);
}

bool
ACDDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char buffer[100];
  sprintf(buffer, "PAAVC,S,COM,RXVOL1,%u", volume);
  return PortWriteNMEA(port, buffer, env);
}

bool
ACDDevice::PutStandbyFrequency(RadioFrequency frequency,
                                   const TCHAR *name,
                                   OperationEnvironment &env)
{
  char buffer[100];
  unsigned freq = frequency.GetKiloHertz();
  sprintf(buffer, "PAAVC,S,COM,CHN2,%u", freq);
  return PortWriteNMEA(port, buffer, env);
}

bool
ACDDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  if (line.ReadCompare("$PAAVS"))
    return ParsePAAVS(line, info);
  else
    return false;
}

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

/*
 * $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a,m,*hh
 *
 * Field Number:
 *  1) UTC Time
 *  2) Status, V=Navigation receiver warning A=Valid
 *  3) Latitude
 *  4) N or S
 *  5) Longitude
 *  6) E or W
 *  7) Speed over ground, knots
 *  8) Track made good, degrees true
 *  9) Date, ddmmyy
 * 10) Magnetic Variation, degrees
 * 11) E or W
 * 12) FAA mode indicator (NMEA 2.3 and later)
 * 13) Checksum
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
 * $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
 *
 * Field Number:
 *  1) Universal Time Coordinated (UTC)
 *  2) Latitude
 *  3) N or S (North or South)
 *  4) Longitude
 *  5) E or W (East or West)
 *  6) GPS Quality Indicator,
 *     0 - fix not available,
 *     1 - GPS fix,
 *     2 - Differential GPS fix
 *     (values above 2 are 2.3 features)
 *     3 = PPS fix
 *     4 = Real Time Kinematic
 *     5 = Float RTK
 *     6 = estimated (dead reckoning)
 *     7 = Manual input mode
 *     8 = Simulation mode
 *  7) Number of satellites in view, 00 - 12
 *  8) Horizontal Dilution of precision (meters)
 *  9) Antenna Altitude above/below mean-sea-level (geoid) (in meters)
 * 10) Units of antenna altitude, meters
 * 11) Geoidal separation, the difference between the WGS-84 earth
 *     ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level
 *     below ellipsoid
 * 12) Units of geoidal separation, meters
 * 13) Age of differential GPS data, time in seconds since last SC104
 *     type 1 or 9 update, null field when DGPS is not used
 * 14) Differential reference station ID, 0000-1023
 * 15) Checksum
 */
static bool
FormatGPGGA(char *buffer, size_t buffer_size, const MoreData &info)
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
           "GPGGA,%02u%02u%02u,%c,%s,%s,%u,%2u,%.1f,%.1f,%c,%.1f,%c,,",
           now.hour, now.minute, now.second,
           info.location.IsValid() ? 'A' : 'V',
           lat_buffer,
           long_buffer,
           (int)info.gps.fix_quality,
           info.gps.satellites_used,
           info.gps.hdop,
           info.gps_altitude,
           'M',
           info.gps_altitude,
           'M');

  return true;
}

/*
 * $GPGSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
 *
 * Field Number:
 *  1) Selection mode
 *         M=Manual, forced to operate in 2D or 3D
 *         A=Automatic, 3D/2D
 *  2) Mode (1 = no fix, 2 = 2D fix, 3 = 3D fix)
 *  3) ID of 1st satellite used for fix
 *  4) ID of 2nd satellite used for fix
 *  ...
 *  14) ID of 12th satellite used for fix
 *  15) PDOP
 *  16) HDOP
 *  17) VDOP
 *  18) checksum
 */
static bool
FormatGPGSA(char *buffer, size_t buffer_size, const MoreData &info)
{
  // don't format GPGSA string if no sat id's are available
  if(!info.gps.satellite_ids_available)
    return false;

  int gps_status;

  if (!info.alive || !info.location_available)
    gps_status = 1;
  else if (!info.gps_altitude_available)
    gps_status = 2;
  else
    gps_status = 3;

  snprintf(buffer, buffer_size,
           "GPGSA,A,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.1f,%.1f,%.1f",
           gps_status,
           info.gps.satellite_ids[0],
           info.gps.satellite_ids[1],
           info.gps.satellite_ids[2],
           info.gps.satellite_ids[3],
           info.gps.satellite_ids[4],
           info.gps.satellite_ids[5],
           info.gps.satellite_ids[6],
           info.gps.satellite_ids[7],
           info.gps.satellite_ids[8],
           info.gps.satellite_ids[9],
           info.gps.satellite_ids[10],
           info.gps.satellite_ids[11],
           info.gps.pdop,
           info.gps.hdop,
           info.gps.vdop);

  return true;
}

void
ACDDevice::OnCalculatedUpdate(const MoreData &basic,
                        const DerivedInfo &calculated)
{
  NullOperationEnvironment env;
  char buffer[100];

  if (FormatGPRMC(buffer, sizeof(buffer), basic))
    PortWriteNMEA(port, buffer, env);

  if (FormatGPGSA(buffer, sizeof(buffer), basic))
    PortWriteNMEA(port, buffer, env);

  if (FormatGPGGA(buffer, sizeof(buffer), basic))
    PortWriteNMEA(port, buffer, env);
}

static Device *
AirControlDisplayCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new ACDDevice(com_port);
}

const struct DeviceRegister acd_driver = {
  _T("ACD"),
  _T("Air Control Display"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  AirControlDisplayCreateOnPort,
};
