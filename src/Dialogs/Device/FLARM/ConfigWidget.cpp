// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigWidget.hpp"
#include "RangeConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Traffic.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "FLARM/Hardware.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "system/Sleep.h"
#include "util/ConvertString.hpp"

#include <fmt/format.h>

FlarmHardware hardware;

static const char *const flarm_setting_names[] = {
  "BAUD",
  "NMEAOUT",
  "THRE",
  "ACFT",
  "LOGINT",
  "PRIV",
  "NOTRACK",
  NULL
};

static const char *const pf_setting_names[] = {
  "BAUD1",
  "BAUD2",
  "NMEAOUT1",
  "NMEAOUT2",
  "ID",
  "XPDR",
  NULL
};

// Helper function to map NMEAOUT raw value to enum value
// According to FTD-014:
// 0: no output
// 1-4: Protocol 4 (different sentence outputs)
// 5: GDL90 - Protocol 4 (only for PowerFLARM Fusion on NMEAOUT2)
// 40-44: Like 0-4 but selects version 4/5 of the protocol
// 60-64: Like 0-4 but selects version 6 of the protocol
// 70-74: Like 0-4 but selects version 7 of the protocol
// 80-84: Like 0-4 but selects version 8 of the protocol
// 90-94: Like 0-4 but selects version 9 of the protocol
static unsigned
MapNmeaOutToEnum(unsigned nmeaout_raw) noexcept
{
    if (nmeaout_raw == 0)
      return 0;  // No output
    else if (nmeaout_raw <= 4)
      return 4;  // Protocol 4 (different sentence outputs)
    else if (nmeaout_raw == 5)
      return 4;  // GDL90 - Protocol 4 (only for PowerFLARM Fusion on NMEAOUT2)
    else if (nmeaout_raw >= 40 && nmeaout_raw <= 44)
      return 44; // Protocol 4/5 (like 0-4 but protocol version 4/5)
    else if (nmeaout_raw >= 60 && nmeaout_raw <= 64)
      return 64; // Protocol 6 (like 0-4 but protocol version 6)
    else if (nmeaout_raw >= 70 && nmeaout_raw <= 74)
      return 74; // Protocol 7 (like 0-4 but protocol version 7)
    else if (nmeaout_raw >= 80 && nmeaout_raw <= 84)
      return 84; // Protocol 8 (like 0-4 but protocol version 8)
    else if (nmeaout_raw >= 90 && nmeaout_raw <= 94)
      return 94; // Protocol 9 (like 0-4 but protocol version 9)
    else {
      return 0;
    }
}

// Helper function to map enum value back to raw value
// Returns the second value in each protocol range (e.g., 91 for Protocol 9, 81 for Protocol 8)
static unsigned
MapEnumToNmeaOut(unsigned enum_value) noexcept
{
  if (enum_value == 0)
    return 0;  // No output
  else if (enum_value == 4)
    return 1;  // Protocol 4: range 1-4, send 1
  else if (enum_value == 44)
    return 41; // Protocol 4/5: range 40-44, send 41
  else if (enum_value == 64)
    return 61; // Protocol 6: range 60-64, send 61
  else if (enum_value == 74)
    return 71; // Protocol 7: range 70-74, send 71
  else if (enum_value == 84)
    return 81; // Protocol 8: range 80-84, send 81
  else if (enum_value == 94)
    return 91; // Protocol 9: range 90-94, send 91
  else
    return 0;  // Invalid enum value - default to 0 (no output)
}

void
FLARMConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  PopupOperationEnvironment env;
  // Request and wait for all settings to be received before displaying
  // RequestAllSettings already waits for each setting with 500ms timeout
  device.RequestAllSettings(flarm_setting_names, env);

  // Initialize availability flags
  nmeaout_available = false;
  nmeaout1_available = false;
  nmeaout2_available = false;
  icaoid_available = false;
  transponder_available = false;

  if (hardware.isPowerFlarm()) {
    device.RequestAllSettings(pf_setting_names, env);
    // Read settings after they've been requested and waited for
    baud1 = device.GetUnsignedValue("BAUD1", 2);
    baud2 = device.GetUnsignedValue("BAUD2", 2);

    // Only read NMEAOUT if it exists, don't use fallback
    nmeaout1_available = device.SettingExists("NMEAOUT1");
    if (nmeaout1_available) {
      nmeaout1_raw = device.GetUnsignedValue("NMEAOUT1", 0);
      nmeaout1 = MapNmeaOutToEnum(nmeaout1_raw);
    } else {
      nmeaout1_raw = 0;
      nmeaout1 = 0;
    }

    nmeaout2_available = device.SettingExists("NMEAOUT2");
    if (nmeaout2_available) {
      nmeaout2_raw = device.GetUnsignedValue("NMEAOUT2", 0);
      nmeaout2 = MapNmeaOutToEnum(nmeaout2_raw);
    } else {
      nmeaout2_raw = 0;
      nmeaout2 = 0;
    }
  } else {
    baud = device.GetUnsignedValue("BAUD", 2);

    // Only read NMEAOUT if it exists, don't use fallback
    nmeaout_available = device.SettingExists("NMEAOUT");
    if (nmeaout_available) {
      nmeaout_raw = device.GetUnsignedValue("NMEAOUT", 0);
      nmeaout = MapNmeaOutToEnum(nmeaout_raw);
    } else {
      nmeaout_raw = 0;
      nmeaout = 0;
    }
  }

  unsigned thre_default = hardware.isPowerFlarm() ? 255 : 1;
  thre = device.GetUnsignedValue("THRE", thre_default);
  acft = device.GetUnsignedValue("ACFT", 0);
  log_int = device.GetUnsignedValue("LOGINT", 2);
  priv = device.GetUnsignedValue("PRIV", 0) == 1;
  notrack = device.GetUnsignedValue("NOTRACK", 0) == 1;

  // ID setting already requested via RequestAllSettings for PowerFLARM
  if (hardware.isPowerFlarm()) {
    icaoid_available = device.SettingExists("ID");
    if (icaoid_available) {
      if (const auto x = device.GetSetting("ID")) {
        icaoid.SetASCII(x->c_str());
      } else {
        icaoid_available = false;
      }
    }
  }

  // XPDR already requested via RequestAllSettings for PowerFLARM
  if (hardware.hasADSB() || hardware.isPowerFlarm()) {
    transponder_available = device.SettingExists("XPDR");
    if (transponder_available) {
      transponder_type = device.GetUnsignedValue("XPDR", 0);
    }
  }

  static constexpr StaticEnumChoice baud_list[] = {
    { 0, _T("4800") },
    { 1, _T("9600") },
    { 2, _T("19200") },
    { 4, _T("38400") },
    { 5, _T("57600") },
    nullptr
  };

  static constexpr StaticEnumChoice baud_list_pf[] = {
    { 0, _T("4800") },
    { 1, _T("9600") },
    { 2, _T("19200") },
    { 4, _T("38400") },
    { 5, _T("57600") },
    { 6, _T("115200") },
    { 7, _T("230400") },
    nullptr
  };


  // NMEAOUT protocol version enum: one value per protocol version
  // According to FTD-014:
  // 0: no output, 1-4: Protocol 4, 40-44: Protocol 4/5, 60-64: Protocol 6,
  // 70-74: Protocol 7, 80-84: Protocol 8, 90-94: Protocol 9
  static constexpr StaticEnumChoice nmeaout_list[] = {
    { 0, _T("No output") },
    { 4, _T("Protocol 4") },
    { 44, _T("Protocol 4/5") },
    { 64, _T("Protocol 6") },
    { 74, _T("Protocol 7") },
    { 84, _T("Protocol 8") },
    { 94, _T("Protocol 9") },
    nullptr
  };

  if (icaoid_available) {
    AddText(_("ICAO ID"), _("24-bit ICAO aircraft address (hexadecimal)"), icaoid);
  }

  static constexpr StaticEnumChoice acft_list[] = {
    { FlarmTraffic::AircraftType::UNKNOWN, N_("Unknown") },
    { FlarmTraffic::AircraftType::GLIDER, N_("Glider") },
    { FlarmTraffic::AircraftType::TOW_PLANE, N_("Tow plane") },
    { FlarmTraffic::AircraftType::HELICOPTER, N_("Helicopter") },
    { FlarmTraffic::AircraftType::PARACHUTE, N_("Parachute") },
    { FlarmTraffic::AircraftType::DROP_PLANE, N_("Drop plane") },
    { FlarmTraffic::AircraftType::HANG_GLIDER, N_("Hang glider") },
    { FlarmTraffic::AircraftType::PARA_GLIDER, N_("Paraglider") },
    { FlarmTraffic::AircraftType::POWERED_AIRCRAFT, N_("Powered aircraft") },
    { FlarmTraffic::AircraftType::JET_AIRCRAFT, N_("Jet aircraft") },
    { FlarmTraffic::AircraftType::FLYING_SAUCER, N_("Flying saucer") },
    { FlarmTraffic::AircraftType::BALLOON, N_("Balloon") },
    { FlarmTraffic::AircraftType::AIRSHIP, N_("Airship") },
    { FlarmTraffic::AircraftType::UAV, N_("Unmanned aerial vehicle") },
    { FlarmTraffic::AircraftType::STATIC_OBJECT, N_("Static object") },
    nullptr
  };

  AddEnum(_("Type"), NULL, acft_list, acft);

  if (hardware.isPowerFlarm()) {
    AddEnum(_("Baud rate port 1"), NULL, baud_list_pf, baud1);
    AddEnum(_("Baud rate port 2"), NULL, baud_list_pf, baud2);
    if (nmeaout1_available)
      AddEnum(_("Protocol version port 1"), _("NMEAOUT1: Sets protocol version for port 1"), nmeaout_list, nmeaout1);
    if (nmeaout2_available)
      AddEnum(_("Protocol version port 2"), _("NMEAOUT2: Sets protocol version for port 2"), nmeaout_list, nmeaout2);
  } else {
    AddEnum(_("Baud rate"), NULL, baud_list, baud);
    AddDummy();
    if (nmeaout_available)
      AddEnum(_("Protocol version"), _("NMEAOUT: Sets protocol version"), nmeaout_list, nmeaout);
  }

  WndProperty *wp_threshold = AddEnum(_("Threshold"),
                                      _("Select a speed threshold."));
  if (wp_threshold != nullptr) {
    DataFieldEnum &df = *(DataFieldEnum *)wp_threshold->GetDataField();
    if (hardware.isPowerFlarm()) {
      df.AddChoice(255, _T("Automatic"));
    }
    TCHAR buffer[64];
    for (unsigned i = 0; i <= 20; ++i) {
      StringFormatUnsafe(buffer, _T("%u m/s"), i);
      df.AddChoice(i, buffer);
    }
    df.SetValue(thre);
    wp_threshold->RefreshDisplay();
  }

  if (transponder_available) {
    static constexpr StaticEnumChoice transponder_list[] = {
      { 0, _T("None") },
      { 1, _T("Mode-C") },
      { 2, _T("Mode-S") },
      { 3, _T("ADS-B Out") },
      nullptr
  };
    AddEnum(_("Transponder type"), _("Type of transponder/ADS-B equipment"), transponder_list, transponder_type);
  }

  AddInteger(_("Logger interval"), NULL, _T("%d s"), _T("%d"),
             1, 8, 1, log_int);
  AddBoolean(_("Stealth mode"), NULL, priv);
  AddBoolean(_("No tracking mode"), NULL, notrack);

  AddButton(_("Range setup"), [this](){
    FLARMRangeConfigWidget widget(GetLook(), device, hardware);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        _T("FLARM range setup"), widget);
  });
}

bool
FLARMConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;

  unsigned control_index = 0;

  if (icaoid_available) {
    StaticString<32> new_icaoid = icaoid;
    if (SaveValue(control_index, new_icaoid)) {
      const WideToUTF8Converter icaoid_utf8(new_icaoid.c_str());
      if (icaoid_utf8.IsValid()) {
        device.SendSetting("ID", icaoid_utf8, env);
        icaoid = new_icaoid;
        changed = true;
      }
    }
    control_index++;
  }

  if (SaveValueEnum(control_index, acft)) {
    device.SendSetting("ACFT", fmt::format_int(acft).c_str(), env);
    changed = true;
  }
  control_index++;

  if (hardware.isPowerFlarm()) {
    if (SaveValueEnum(control_index, baud1)) {
      device.SendSetting("BAUD1", fmt::format_int{baud1}.c_str(), env);
      changed = true;
    }
    control_index++;

    if (SaveValueEnum(control_index, baud2)) {
      device.SendSetting("BAUD2", fmt::format_int{baud2}.c_str(), env);
      changed = true;
    }
    control_index++;

    if (nmeaout1_available) {
      if (SaveValueEnum(control_index, nmeaout1)) {
        nmeaout1_raw = MapEnumToNmeaOut(nmeaout1);
        device.SendSetting("NMEAOUT1", fmt::format_int(nmeaout1_raw).c_str(), env);
        /* If device responds with ERROR, WaitForSetting will timeout
           but we'll see the error in the FLARM log */
        if (device.WaitForSetting("NMEAOUT1", 1000)) {
          unsigned received = device.GetUnsignedValue("NMEAOUT1", 0);
          if (received != nmeaout1_raw) {
            nmeaout1 = MapNmeaOutToEnum(received);
          }
          nmeaout1_raw = received;
        }
        changed = true;
      }
      control_index++;
    }

    if (nmeaout2_available) {
      if (SaveValueEnum(control_index, nmeaout2)) {
        nmeaout2_raw = MapEnumToNmeaOut(nmeaout2);
        device.SendSetting("NMEAOUT2", fmt::format_int(nmeaout2_raw).c_str(), env);
        if (device.WaitForSetting("NMEAOUT2", 1000)) {
          unsigned received = device.GetUnsignedValue("NMEAOUT2", 0);
          if (received != nmeaout2_raw) {
            nmeaout2 = MapNmeaOutToEnum(received);
          }
          nmeaout2_raw = received;
        }
        changed = true;
      }
      control_index++;
    }

    if (SaveValueEnum(control_index, thre)) {
      device.SendSetting("THRE", fmt::format_int(thre).c_str(), env);
      changed = true;
    }
    control_index++;
  } else {
    if (SaveValueEnum(control_index, baud)) {
      device.SendSetting("BAUD", fmt::format_int{baud}.c_str(), env);
      changed = true;
    }
    control_index++;

    // Dummy control
    control_index++;

    if (nmeaout_available) {
      if (SaveValueEnum(control_index, nmeaout)) {
        nmeaout_raw = MapEnumToNmeaOut(nmeaout);
        device.SendSetting("NMEAOUT", fmt::format_int(nmeaout_raw).c_str(), env);
        if (device.WaitForSetting("NMEAOUT", 1000)) {
          unsigned received = device.GetUnsignedValue("NMEAOUT", 0);
          if (received != nmeaout_raw) {
            nmeaout = MapNmeaOutToEnum(received);
          }
          nmeaout_raw = received;
        }
        changed = true;
      }
      control_index++;
    }

    if (SaveValueEnum(control_index, thre)) {
      device.SendSetting("THRE", fmt::format_int(thre).c_str(), env);
      changed = true;
    }
    control_index++;
  }

  if (transponder_available) {
    if (SaveValueEnum(control_index, transponder_type)) {
      device.SendSetting("XPDR", fmt::format_int(transponder_type).c_str(), env);
      changed = true;
    }
    control_index++;
  }

  if (SaveValueInteger(control_index, log_int)) {
    device.SendSetting("LOGINT", fmt::format_int{log_int}.c_str(), env);
    changed = true;
  }
  control_index++;

  if (SaveValue(control_index, priv)) {
    device.SendSetting("PRIV", fmt::format_int{priv}.c_str(), env);
    changed = true;
  }
  control_index++;

  if (SaveValue(control_index, notrack)) {
    device.SendSetting("NOTRACK", fmt::format_int{notrack}.c_str(), env);
    changed = true;
  }

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), _T("FLARM"));
  return false;
}
