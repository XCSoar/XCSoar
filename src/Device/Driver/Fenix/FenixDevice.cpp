// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FenixDevice.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Geo/SpeedVector.hpp"
#include "Units/System.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"
#include "Device/Util/NMEAReader.hpp"
#include "Device/RecordedFlight.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include <chrono>
#include <string.h>

using std::string_view_literals::operator""sv;

static bool ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double bearing, norm;

  bool bearing_valid = line.ReadChecked(bearing);
  bool norm_valid = line.ReadChecked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::Degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    return true;
  } else
  return false;
}

/*
 * \brief Contains flight parameters.
 *
 * $LXWP0,logger_status,tas,altitude,vario1,vario2,vario3,vario4,vario5,vario6,heading,wind_direction,wind_speed*CS
 *
 * PARAMETER         | TYPE    | DESCRIPTION
 * ------------------+---------+-----------------------------------------
 * logger_status     | char    | ‘Y’ logger is recording, ‘N’ logger is not recording
 * tas               | float   | True airspeed in km/h
 * altitude          | float   | Altitude in meters
 * vario 1,2,3,4,5,6 | float   | Vario measurements in last second (6 measurements)
 * heading           | int16_t | True heading. -1 if compass not connected
 * wind_direction    | string  | Wind direction in degrees, blank if wind is 0
 * wind_speed        | string  | Wind speed in km/h, blank if wind is 0
 *
 */
bool FenixDevice::LXWP0(NMEAInputLine &line, NMEAInfo &info)
{
  line.Skip();

  double airspeed;
  bool tas_available = line.ReadChecked(airspeed);
  if (tas_available && (airspeed < -50 || airspeed > 250))
  {
    /* implausible */
    return false;
  }
  double value;
  if (line.ReadChecked(value))
  {
    info.ProvidePressureAltitude(value);
  }

  if (tas_available)
  {
    /*
     * Call ProvideTrueAirspeed() after ProvidePressureAltitude() to use
     * the provided altitude (if available)
     */
    info.ProvideTrueAirspeed(Units::ToSysUnit(airspeed, Unit::KILOMETER_PER_HOUR));
  }

  if (line.ReadChecked(value))
  {
    info.ProvideTotalEnergyVario(value);
  }
  line.Skip(6);

  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
  {
    info.ProvideExternalWind(wind);
  }
  return true;
}
/*
 * \brief Contains basic device information
 *
 * $LXWP1,device_type,serial_nr,fw_version,hw_version*CS
 *
 *
 * PARAMETER       | TYPE     | DESCRIPTION
 * ----------------+----------+-----------------------------------------
 * device_type     | string   | Device type (example: Fenix)
 * serial_nr       | uint32_t | Serial number
 * fw_version      | float    | Firmware version
 * hw_version      | float    | Hardware version
 *
 */
bool  FenixDevice::LXWP1(NMEAInputLine &line, NMEAInfo &info)
{
  info.device.product = line.ReadView();
  info.device.serial = line.ReadView();
  info.device.software_version = line.ReadView();
  info.device.hardware_version = line.ReadView();
  return true;
}

/*
 * \brief Contains polar data for calculation of speed command and volume of vario
 *
 * $LXWP2,mc,load,bugs,polarA,polarB,polarC,volume*CS
 *
 *
 * PARAMETER       | TYPE     | DESCRIPTION
 * ----------------+----------+-----------------------------------------
 * mc              | float    | MacCready setting
 * load            | float    | Load coefficient (total flight mass divided by polar reference mass)
 * bugs            | uint16_t | Bugs factor in %
 * polarA,B,C      | float    | Polar coefficients Av2+Bv+C
 * volume          | uint8_t  | Volume of vario tone
 *
 */
bool FenixDevice::LXWP2(NMEAInputLine &line, NMEAInfo &info)
{
  double value;
  // MacCready value
  if (line.ReadChecked(value))
  {
     info.settings.ProvideMacCready(value, info.clock);
  }

  // Ballast
  if (line.ReadChecked(value))
  {
      info.settings.ProvideBallastOverload(value, info.clock);
  }

  // Bugs
  if (line.ReadChecked(value))
  {
    if (value <= 1.5 && value >= 1.0)
    {
        info.settings.ProvideBugs(2 - value, info.clock);
    }
    else
    {
      info.settings.ProvideBugs((100 - value) / 100., info.clock);
    }
  }

  line.Skip(3);

  unsigned volume;
  if (line.ReadChecked(volume))
  {
      info.settings.ProvideVolume(volume, info.clock);
  }
  return true;
}
/*
 * \brief Contains settings parameters
 *
 * $LXWP3,alt_offset,sc_mode,filter,reserved,te_level,int_time,range,sc_silence,sc_switch_mode,sc_speed,polar_name*CS
 *
 *
 * PARAMETER       | TYPE     | DESCRIPTION
 * ----------------+----------+-----------------------------------------
 * alt_offset      | int16_t  | Daily offset of altitude due to daily QNH for flying QFE
 * sc_mode         | uint8_t  | 0 = manual, 1 = circling, 2 = speed
 * filter          | float    | Vario filter in seconds
 * reserved        |          |
 * te_level        | uint16_t | TE level in %
 * int_time        | uint16_t | Vario integration time in seconds
 * range           | float    | Vario scale range in m/s
 * sc_silence      | float SC | silence band in m/s
 * sc_switch_mode  | uint8_t  | 0 = off, 1 = on, 2 = toggle
 * sc_speed        | uint16_t | SC speed. Works when sc mode is set to 2 (speed)
 * polar_name      | string   | Name of polar
 *
 */
bool FenixDevice::LXWP3(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // Altitude offset -> QNH
  if (line.ReadChecked(value))
  {
    value = Units::ToSysUnit(-value, Unit::FEET);
    auto qnh = AtmosphericPressure::PressureAltitudeToStaticPressure(value);
    info.settings.ProvideQNH(qnh, info.clock);
  }

  return true;
}

bool FenixDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  bool ret = false;
  if (!VerifyNMEAChecksum(String))
  {
      return ret;
  }

  NMEAInputLine line(String);

  const auto type = line.ReadView();

  if (type == "$LXWP0"sv)
  {
    ret = LXWP0(line, info);
  }
  else if (type == "$LXWP1"sv)
  {
    ret =  LXWP1(line, info);
  }
  else if (type == "$LXWP2"sv)
  {
    ret = LXWP2(line, info);
  }
  else if (type == "$LXWP3"sv)
  {
    ret = LXWP3(line, info);
  }
  return ret;
}

bool FenixDevice::ReadFlightList([[maybe_unused]] RecordedFlightList &flight_list, OperationEnvironment &env)
{
  std::string *retMessage;
  std::size_t found = 0;
  PortNMEAReader reader(port, env);
  std::string delimiter = ",";
  size_t pos;
  uint8_t counter = 0;
  uint8_t flightsInLogbook = 0;
  std::string strTemp;


  port.StopRxThread();
  PortWriteNMEA(port, "RCDT,GET,INFO", env);
  do
  {
    port.Flush();
    char *ret = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(ret);
    found = retMessage->find("RCDT");
  }while(found == std::string::npos);

  PortWriteNMEA(port, "RCDT,GET,LOGBOOK,0", env);

  do
  {
    port.Flush();
    char *ret = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(ret);
    found = retMessage->find("RCDT");
  }while(found == std::string::npos);

  std::string text = *retMessage;
  text =  text + ",";
  text.erase(0,std::string("RCDT,ANS,LOGBOOK,").size());


  while ((pos = text.find(delimiter)) != std::string::npos)
  {
    strTemp = text.substr(0, pos);
    text.erase(0, pos + delimiter.length());

    if(counter == ANS_LOGBOOK::FLIGHTS_IN_LOGBOOK)
    {
        flightsInLogbook = stoi(strTemp);
    }
    counter ++;
  }
  env.SetProgressRange(flightsInLogbook);

  for (uint8_t i = 0; i < flightsInLogbook; i++)
  {
    RecordedFlightInfo flight_info;
    env.SetProgressPosition(i);
    std::string message = "RCDT,GET,LOGBOOK," + std::to_string(i);
    PortWriteNMEA(port, message.c_str(), env);

    do
    {
      port.Flush();
      char *ret = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
      retMessage = new std::string(ret);
      found = retMessage->find("RCDT");
    }while(found == std::string::npos);


    std::string text = *retMessage;
    text =  text + ",";
    text.erase(0,std::string("RCDT,ANS,LOGBOOK,").size());
    counter = 0;

    while ((pos = text.find(delimiter)) != std::string::npos)
    {
      strTemp = text.substr(0, pos);
      text.erase(0, pos + delimiter.length());

      if(counter == ANS_LOGBOOK::IGC_FILE_NAME)
      {
        std::memcpy(flight_info.internal.fenix.filename ,strTemp.c_str(), sizeof(flight_info.internal.fenix.filename));
      }
      else if(counter == ANS_LOGBOOK::DATE_OF_FLIGHT)
      {
        BrokenDate date;
        date.year = 2000u + stoi(strTemp.substr(6,2));
        date.month = stoi(strTemp.substr(3,2));
        date.day = stoi(strTemp.substr(0,2));
        flight_info.date = date;
      }
      else if(counter == ANS_LOGBOOK::TAKE_OF_TIME)
      {
        BrokenTime time;
        std::chrono::seconds sec(stoi(strTemp));
        time.hour = std::chrono::duration_cast<std::chrono::hours>(sec).count();
        time.minute = std::chrono::duration_cast<std::chrono::minutes>(sec).count() % 60u;
        time.second = sec.count() % 60u;
        flight_info.start_time = time;
      }
      else if(counter == ANS_LOGBOOK::LANDING_TIME)
      {
        BrokenTime time;
        std::chrono::seconds sec(stoi(strTemp));
        time.hour = std::chrono::duration_cast<std::chrono::hours>(sec).count();
        time.minute = std::chrono::duration_cast<std::chrono::minutes>(sec).count() % 60u;
        time.second = sec.count() % 60u;
        flight_info.end_time = time;
      }
      else if(counter == ANS_LOGBOOK::FILE_SIZE)
      {
        flight_info.internal.fenix.filesize = stoi(strTemp);
      }
      counter ++;
    }

    flight_list.append(flight_info);
  }
  port.StartRxThread();
  return true;
}

bool FenixDevice::DownloadFlight([[maybe_unused]]  const RecordedFlightInfo &flight, Path path, OperationEnvironment &env)
{
  FileOutputStream fos(path);
  BufferedOutputStream os(fos);
  env.SetProgressRange(100);
  port.Flush();
  PortNMEAReader reader(port, env);
  uint32_t flightPart = 0;
  std::string *retMessage;
  std::size_t found = 0;
  uint8_t calculated_checksum = 0;
  uint8_t message_checksum = 0;
  uint32_t rx_filesize = 0;


  port.StopRxThread();

  env.SetProgressRange(flight.internal.fenix.filesize);

  while (true)
  {
//     Create header for getting IGC file data
    std::string message = "RCDT,GET,FLIGHT," + std::string(flight.internal.fenix.filename) + "," + std::to_string(flightPart);
    PortWriteNMEA(port, message.c_str(), env);

    std::byte a[500];
    std::span<std::byte> dest(a);
    try{
      port.Flush();
      port.FullRead(dest,env, std::chrono::seconds(20));

      retMessage = new std::string((char*)dest.data());

      found = retMessage->find("RCDT");
      if(found != std::string::npos)
      {
        retMessage->erase(0,found);
        found = retMessage->find('*');
        if(found != std::string::npos)
        {
          retMessage->erase(found + 3u);
          message_checksum = std::stoul(retMessage->substr(found + 1u, found + 3u),0,16);

          calculated_checksum = NMEAChecksum({retMessage->c_str(), retMessage->size() - 3});

          if(calculated_checksum == message_checksum)
          {
            //remove header
            std::string header = "RCDT,ANS,FLIGHT," + std::to_string(flightPart) + ",";
            retMessage->erase(0,header.size());
            found = retMessage->find('*');
            os.Write(retMessage->substr(0,found));
            rx_filesize += retMessage->substr(0,found).size();
            flightPart++;
            fos.Sync();
          }
          else
          {
            throw std::runtime_error("checksum not ok");
          }
        }
        else
        {
            throw std::runtime_error("received meesage not ok");
        }
      }
      else
      {
        throw std::runtime_error("no message received");
      }
    }
    catch(const std::exception& e)
    {
      return false;
    }

    env.SetProgressPosition(rx_filesize);

    if(rx_filesize >= flight.internal.fenix.filesize)
    {
      break;
    }

  }
  os.Flush();
  fos.Commit();
  return true;
}

bool FenixDevice::PutMacCready([[maybe_unused]]  double mac_cready,[[maybe_unused]]  OperationEnvironment &env)
{
  std::string *retMessage;
  bool ret = false;
  std::size_t found = 0;
  uint8_t receive_counter = 0;
  PortNMEAReader reader(port, env);

  port.StopRxThread();
  std::string message = "RCDT,SET,MC_BAL," + std::to_string(mac_cready) + ",,,,,,";
  PortWriteNMEA(port, message.c_str(), env);

  do
  {
    port.Flush();
    char *readLine = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(readLine);
    found = retMessage->find("RCDT,ANS,OK");
    if(found != std::string::npos)
    {
      ret = true;
    }
    receive_counter++;
  }while((found == std::string::npos) && (receive_counter++ <= 3u));

  return ret;
}

bool FenixDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  std::string *retMessage;
  bool ret = false;
  std::size_t found = 0;
  uint8_t receive_counter = 0;
  PortNMEAReader reader(port, env);

  port.StopRxThread();
  std::string message = "RCDT,SET,MC_BAL,,," + std::to_string((1.0 - bugs) * 100.0) + ",,,,";
  PortWriteNMEA(port, message.c_str(), env);

  do
  {
    port.Flush();
    char *readLine = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(readLine);
    found = retMessage->find("RCDT,ANS,OK");
    if(found != std::string::npos)
    {
      ret = true;
    }
    receive_counter++;
  }while((found == std::string::npos) && (receive_counter++ <= 3u));

  return ret;
}

bool FenixDevice::PutBallast([[maybe_unused]] double fraction,[[maybe_unused]]  double overload, OperationEnvironment &env)
{
  std::string *retMessage;
  bool ret = false;
  std::size_t found = 0;
  uint8_t receive_counter = 0;
  PortNMEAReader reader(port, env);

  port.StopRxThread();
  std::string message = "RCDT,SET,MC_BAL,," + std::to_string(fraction * 100.0) + ",,,,,";
  PortWriteNMEA(port, message.c_str(), env);

  do
  {
    port.Flush();
    char *readLine = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(readLine);
    found = retMessage->find("RCDT,ANS,OK");
    if(found != std::string::npos)
    {
      ret = true;
    }
    receive_counter++;
  }while((found == std::string::npos) && (receive_counter++ <= 3u));

  return ret;
}

bool FenixDevice::PutQNH(const AtmosphericPressure &pressure, OperationEnvironment &env)
{
  std::string *retMessage;
  bool ret = false;
  std::size_t found = 0;
  uint8_t receive_counter = 0;
  PortNMEAReader reader(port, env);

  port.StopRxThread();
  std::string message = "RCDT,SET,MC_BAL,,,,,,," + std::to_string(pressure.GetHectoPascal());
  PortWriteNMEA(port, message.c_str(), env);

  do
  {
    port.Flush();
    char *readLine = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(readLine);
    found = retMessage->find("RCDT,ANS,OK");
    if(found != std::string::npos)
    {
      ret = true;
    }
    receive_counter++;
  }while((found == std::string::npos) && (receive_counter++ <= 3u));

  return ret;
}

bool FenixDevice::PutVolume([[maybe_unused]] unsigned volume, [[maybe_unused]] OperationEnvironment &env)
{
  std::string *retMessage;
  bool ret = false;
  std::size_t found = 0;
  uint8_t receive_counter = 0;
  PortNMEAReader reader(port, env);

  port.StopRxThread();
  std::string message = "RCDT,SET,MC_BAL,,,,,," + std::to_string(volume) + ",";
  PortWriteNMEA(port, message.c_str(), env);

  do
  {
    port.Flush();
    char *readLine = reader.ReadLine(TimeoutClock(std::chrono::seconds(2)));
    retMessage = new std::string(readLine);
    found = retMessage->find("RCDT,ANS,OK");
    if(found != std::string::npos)
    {
      ret = true;
    }
    receive_counter++;
  }while((found == std::string::npos) && (receive_counter++ <= 3u));

  return ret;
}
