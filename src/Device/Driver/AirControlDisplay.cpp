// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/AirControlDisplay.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Atmosphere/Pressure.hpp"
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"
#include "TransponderMode.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"
#include "util/StaticString.hxx"
#include "util/Macros.hpp"
#include "Formatter/NMEAFormatter.hpp"
#include "NMEA/MoreData.hpp"
#include "Operation/Operation.hpp"
#include "time/PeriodClock.hpp"

using std::string_view_literals::operator""sv;

static bool
ParsePAAVS(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  const auto type = line.ReadView();

  if (type == "ALT"sv) {
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
  } else if (type == "COM"sv) {
    /*
    $PAAVS,COM,<CHN1>,<CHN2>,<RXVOL1>,<RXVOL2>,<DWATCH>,<RX1>,<RX2>,<TX1>
     <CHN1> Primary radio channel;
            25kHz frequencies and 8.33kHz channels as unsigned integer
            values between 118000 and 136990
     <CHN2> Secondary radio channel;
            25kHz frequencies and 8.33kHz channels as unsigned integer
            values between 118000 and 136990
     <RXVOL1> Primary radio channel volume (Unsigned integer values, 0–100)
     <RXVOL2> Secondary radio channel volume (Unsigned integer values, 0–100)
     <DWATCH> Dual watch mode (0 = off; 1 = on)
     <RX1> Primary channel rx state (0 = no signal rec; 1 = signal rec)
     <RX2> Secondary channel rx state (0 = no signal rec; 1 = signal rec)
     <TX1> Transmit active (0 = no transmission; 1 = transmitting signal)
     */

    if (line.ReadChecked(value)) {
      info.settings.has_active_frequency.Update(info.clock);
      info.settings.active_frequency = RadioFrequency::FromKiloHertz(value);
    }

    if (line.ReadChecked(value)) {
      info.settings.has_standby_frequency.Update(info.clock);
      info.settings.standby_frequency = RadioFrequency::FromKiloHertz(value);
    }

    unsigned volume;
    if (line.ReadChecked(volume))
      info.settings.ProvideVolume(volume, info.clock);
  } else if (type == "XPDR"sv) {
    /*
    $PAAVS,XPDR,<SQUAWK>,<ACTIVE>,<ALTINH>,<ALT>,<SPI>,<ALLCALLSINH>
    <SQUAWK> Squawk code value;
             Octal unsigned integer value between 0000 and 7777 (digits 0–7).
    <ACTIVE> Active flag;
             0: standby (transponder is switched off / "SBY" mode)
             1: active (transponder is switched on / "ALT" or "ON" mode
                dependent of ALTINH)
    <ALTINH> Altitude inhibit flag;
             0: transmit altitude ("ALT" mode if active)
             1: do not transmit altitude ("ON" mode if active)
    <ALT>    Transmitted altitude in FL (integer value)
    <SPI>    Special Position Ident flag
             0: not set
             1: set ("IDENT")
    <ALLCALLSINH> 
             Allcalls inhibit flag
             0: not set
             1: set ("GND Mode")
     */
    unsigned code_value;
    if (line.ReadChecked(code_value)) {
      StaticString<16> buffer;
      buffer.Format(_T("%04u"), code_value);
      TransponderCode parsed_code = TransponderCode::Parse(buffer);

      if (!parsed_code.IsDefined())
        return false;

      info.settings.transponder_code = parsed_code;
      info.settings.has_transponder_code.Update(info.clock);
    }

    unsigned active = 0;
    unsigned altitude_inhibit = 0;
    unsigned special_position_ident = 0;
    unsigned allcalls_inhibit = 0;

    bool has_active = line.ReadChecked(active);
    bool has_altitude_inhibit = line.ReadChecked(altitude_inhibit);
    line.Skip();
    bool has_special_position_ident = line.ReadChecked(special_position_ident);
    bool has_allcalls_inhibit = line.ReadChecked(allcalls_inhibit);

    if (has_active &&
        has_altitude_inhibit &&
        has_special_position_ident &&
        has_allcalls_inhibit) {
      if (special_position_ident == 1) {
        info.settings.transponder_mode.mode = TransponderMode::IDENT;
      } else if (active == 0) {
          info.settings.transponder_mode.mode = TransponderMode::SBY;
      } else if (allcalls_inhibit == 1) {
        info.settings.transponder_mode.mode = TransponderMode::GND;
      } else if (active == 1 && altitude_inhibit == 1) {
        info.settings.transponder_mode.mode = TransponderMode::ON;
      } else if (active == 1 && altitude_inhibit == 0) {
        info.settings.transponder_mode.mode = TransponderMode::ALT;
      } else {
        info.settings.transponder_mode.mode = TransponderMode::UNDEFINED;
      }
      info.settings.has_transponder_mode.Update(info.clock);
    }
  } else {
    return false;
  }

  return true;
}

class ACDDevice : public AbstractDevice {
  Port &port;
  PeriodClock status_clock;

public:
  ACDDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;
  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const char *name,
                           OperationEnvironment &env) override;
  bool ExchangeRadioFrequencies(OperationEnvironment &env,
                                NMEAInfo &info) override;
  bool PutTransponderCode(TransponderCode code, OperationEnvironment &env) override;
  void OnCalculatedUpdate(const MoreData &basic,
                          [[maybe_unused]] const DerivedInfo &calculated) override;
};

bool
ACDDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  char buffer[100];
  unsigned qnh = uround(pres.GetPascal());
  sprintf(buffer, "PAAVC,S,ALT,QNH,%u", qnh);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  char buffer[100];
  sprintf(buffer, "PAAVC,S,COM,RXVOL1,%u", volume);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::PutStandbyFrequency(RadioFrequency frequency,
                                   [[maybe_unused]] const char *name,
                                   OperationEnvironment &env)
{
  char buffer[100];
  unsigned freq = frequency.GetKiloHertz();
  sprintf(buffer, "PAAVC,S,COM,CHN2,%u", freq);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::PutTransponderCode(TransponderCode code, OperationEnvironment &env)
{
  char buffer[100];
  sprintf(buffer, "PAAVC,S,XPDR,SQUAWK,%04o", code.GetCode());
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
ACDDevice::ExchangeRadioFrequencies(OperationEnvironment &env,
                                    [[maybe_unused]] NMEAInfo &info)
{
  const char *sentence = "PAAVX,COM,XCHN";
  PortWriteNMEA(port, sentence, env);
  return true;
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

void
ACDDevice::OnCalculatedUpdate(const MoreData &basic, 
                              [[maybe_unused]] const DerivedInfo &calculated)
{
  NullOperationEnvironment env;

  if (basic.gps.fix_quality != FixQuality::NO_FIX &&
      status_clock.CheckUpdate(std::chrono::seconds(1))) {

    char buffer[100];

    FormatGPRMC(buffer, sizeof(buffer), basic);
    PortWriteNMEA(port, buffer, env);

    FormatGPGSA(buffer, sizeof(buffer), basic);
    PortWriteNMEA(port, buffer, env);

    FormatGPGGA(buffer, sizeof(buffer), basic);
    PortWriteNMEA(port, buffer, env);
  }
}

static Device *
AirControlDisplayCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new ACDDevice(com_port);
}

const struct DeviceRegister acd_driver = {
  _T("ACD"),
  _T("Air Control Display"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  AirControlDisplayCreateOnPort,
};
