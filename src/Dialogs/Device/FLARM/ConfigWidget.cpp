// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Traffic.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "FLARM/Hardware.hpp"

FlarmHardware hardware;

static const char *const flarm_setting_names[] = {
  "BAUD",
  "PRIV",
  "THRE",
  "RANGE",
  "ACFT",
  "LOGINT",
  "NOTRACK",
  NULL
};

static const char *const pf_setting_names[] = {
  "VRANGE",
  NULL
};

static const char *const adsb_setting_names[] = {
  "PCASRANGE",
  "PCASVRANGE",
  "ADSBRANGE",
  "ADSBVRANGE",
  NULL
};

void
FLARMConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  PopupOperationEnvironment env; 
  device.RequestAllSettings(flarm_setting_names, env);
  if (hardware.isPowerFlarm())
    device.RequestAllSettings(pf_setting_names, env);
  if (hardware.hasADSB())
    device.RequestAllSettings(adsb_setting_names, env);

  baud = device.GetUnsignedValue("BAUD", 2);
  priv = device.GetUnsignedValue("PRIV", 0) == 1;
  thre = device.GetUnsignedValue("THRE", 2);
  unsigned max_range = hardware.isPowerFlarm() ? 65535 : 25500;
  range = device.GetUnsignedValue("RANGE", max_range);
  acft = device.GetUnsignedValue("ACFT", 0);
  log_int = device.GetUnsignedValue("LOGINT", 2);
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
  AddBoolean(_("Stealth mode"), NULL, priv);
  AddInteger(_("Threshold"), NULL, _T("%d m/s"), _T("%d"), 1, 10, 1, thre);
  AddInteger(_("Range"), NULL, _T("%d m"), _T("%d"), 2000, max_range, 250, range);

  if (hardware.isPowerFlarm()) {
    vrange = device.GetUnsignedValue("VRANGE", 500);
    AddInteger(_("Vertical range"), NULL, _T("%d m"), _T("%d"), 100, 2000, 100, vrange);
  }
  if (hardware.hasADSB()) {
    pcas_range = device.GetUnsignedValue("PCASRANGE", 7408);
    pcas_vrange = device.GetUnsignedValue("PCASVRANGE", 610);
    adsb_range = device.GetUnsignedValue("ADSBRANGE", 65535);
    adsb_vrange = device.GetUnsignedValue("ASBVRANGE", 65535);
    AddInteger(_("PCAS range"), NULL, _T("%d m"), _T("%d"), 500, 9260, 500, pcas_range);
    AddInteger(_("PCAS vertical range"), NULL, _T("%d m"), _T("%d"), 250, 65535, 250, pcas_vrange);
    AddInteger(_("ADSB range"), NULL, _T("%d m"), _T("%d"), 500, 65535, 500, adsb_range);
    AddInteger(_("ADSB vertical range"), NULL, _T("%d m"), _T("%d"), 250, 65535, 250, adsb_vrange);
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
  AddBoolean(_("No tracking mode"), NULL, notrack);

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

  if (SaveValue(Priv, priv)) {
    buffer.UnsafeFormat("%u", priv);
    device.SendSetting("PRIV", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(Thre, thre)) {
    buffer.UnsafeFormat("%u", thre);
    device.SendSetting("THRE", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(Range, range)) {
    buffer.UnsafeFormat("%u", range);
    device.SendSetting("RANGE", buffer, env);
    changed = true;
  }

  if (hardware.hasADSB()) {
    if (SaveValueInteger(VRange, vrange)) {
      buffer.UnsafeFormat("%u", vrange);
      device.SendSetting("VRANGE", buffer, env);
      changed = true;
    }
  }

  if (hardware.hasADSB()) {
    if (SaveValueInteger(PCASRange, pcas_range)) {
      buffer.UnsafeFormat("%u", pcas_range);
      device.SendSetting("PCASRANGE", buffer, env);
      changed = true;
    }

    if (SaveValueInteger(PCASVRange, pcas_vrange)) {
      buffer.UnsafeFormat("%u", pcas_vrange);
      device.SendSetting("PCASVRANGE", buffer, env);
      changed = true;
    }

    if (SaveValueInteger(ADSBRange, adsb_range)) {
      buffer.UnsafeFormat("%u", adsb_range);
      device.SendSetting("ADSBRANGE", buffer, env);
      changed = true;
    }

    if (SaveValueInteger(ADSBVrange, adsb_vrange)) {
      buffer.UnsafeFormat("%u", adsb_vrange);
      device.SendSetting("ADSBVRANGE", buffer, env);
      changed = true;
    }
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
