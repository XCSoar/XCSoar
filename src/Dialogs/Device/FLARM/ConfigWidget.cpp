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

void
FLARMConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  PopupOperationEnvironment env; 
  device.RequestAllSettings(flarm_setting_names, env);

  baud = device.GetUnsignedValue("BAUD", 2);
  thre = device.GetUnsignedValue("THRE", 1);
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

  AddEnum(_("Baud rate"), NULL, baud_list, baud);
  AddInteger(_("Threshold"), NULL, _T("%d m/s"), _T("%d"), 1, 10, 1, thre);

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
  NarrowString<32> buffer;

  if (SaveValueEnum(Baud, baud)) {
    buffer.UnsafeFormat("%u", baud);
    device.SendSetting("BAUD", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(Thre, thre)) {
    buffer.UnsafeFormat("%u", thre);
    device.SendSetting("THRE", buffer, env);
    changed = true;
  }

  if (SaveValueEnum(Acft, acft)) {
    buffer.UnsafeFormat("%u", acft);
    device.SendSetting("ACFT", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(LogInt, log_int)) {
    buffer.UnsafeFormat("%u", log_int);
    device.SendSetting("LOGINT", buffer, env);
    changed = true;
  }

  if (SaveValue(Priv, priv)) {
    buffer.UnsafeFormat("%u", priv);
    device.SendSetting("PRIV", buffer, env);
    changed = true;
  }

  if (SaveValue(NoTrack, notrack)) {
    buffer.UnsafeFormat("%u", notrack);
    device.SendSetting("NOTRACK", buffer, env);
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
