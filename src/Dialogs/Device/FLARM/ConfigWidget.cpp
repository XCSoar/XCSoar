// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigWidget.hpp"
#include "RangeConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Traffic.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "FLARM/Hardware.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "lib/fmt/ToBuffer.hxx"

#include <fmt/format.h>

FlarmHardware hardware;

static const char *const flarm_setting_names[] = {
  "BAUD",
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
  NULL
};

void
FLARMConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  PopupOperationEnvironment env; 
  device.RequestAllSettings(flarm_setting_names, env);
  if (hardware.isPowerFlarm()) {
    device.RequestAllSettings(pf_setting_names, env);
    baud1 = device.GetUnsignedValue("BAUD1", 2);
    baud2 = device.GetUnsignedValue("BAUD2", 2);
  } else {
    baud = device.GetUnsignedValue("BAUD", 2);
  }

  unsigned thre_default = hardware.isPowerFlarm() ? 255 : 1;
  thre = device.GetUnsignedValue("THRE", thre_default);
  acft = device.GetUnsignedValue("ACFT", 0);
  log_int = device.GetUnsignedValue("LOGINT", 2);
  priv = device.GetUnsignedValue("PRIV", 0) == 1;
  notrack = device.GetUnsignedValue("NOTRACK", 0) == 1;

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


  if (hardware.isPowerFlarm()) {
    AddEnum(_("Baud rate port 1"), NULL, baud_list_pf, baud1);
    AddEnum(_("Baud rate port 2"), NULL, baud_list_pf, baud2);
  } else {
    AddEnum(_("Baud rate"), NULL, baud_list, baud);
    AddDummy();
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

  if (hardware.isPowerFlarm()) {
    if (SaveValueEnum(Baud1, baud1)) {
      device.SendSetting("BAUD1", fmt::format_int{baud1}.c_str(), env);
      changed = true;
    }
    if (SaveValueEnum(Baud2, baud2)) {
      device.SendSetting("BAUD2", fmt::format_int{baud2}.c_str(), env);
      changed = true;
    }
  } else {
    if (SaveValueEnum(Baud1, baud)) {
      device.SendSetting("BAUD", fmt::format_int{baud}.c_str(), env);
      changed = true;
    }
  }

  if (SaveValueEnum(Thre, thre)) {
    device.SendSetting("THRE", fmt::format_int{thre}.c_str(), env);
    changed = true;
  }

  if (SaveValueEnum(Acft, acft)) {
    device.SendSetting("ACFT",  fmt::format_int{acft}.c_str(), env);
    changed = true;
  }

  if (SaveValueInteger(LogInt, log_int)) {
    device.SendSetting("LOGINT", fmt::format_int{log_int}.c_str(), env);
    changed = true;
  }

  if (SaveValue(Priv, priv)) {
    device.SendSetting("PRIV", fmt::format_int{priv}.c_str(), env);
    changed = true;
  }

  if (SaveValue(NoTrack, notrack)) {
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
