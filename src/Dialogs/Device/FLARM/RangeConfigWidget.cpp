// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

#include <fmt/format.h>

static const char *const flarm_setting_names[] = {
  "RANGE",
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

static unsigned
GetUnsignedValue(const FlarmDevice &device, const char *name,
                 unsigned default_value)
{
  if (const auto x = device.GetSetting(name)) {
    char *endptr;
    unsigned long y = strtoul(x->c_str(), &endptr, 10);
    if (endptr > x->c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

void
FLARMRangeConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  PopupOperationEnvironment env; 
  device.RequestAllSettings(flarm_setting_names, env);
  if (hardware.isPowerFlarm())
    device.RequestAllSettings(pf_setting_names, env);
  if (hardware.hasADSB())
    device.RequestAllSettings(adsb_setting_names, env);

  unsigned max_range = hardware.isPowerFlarm() ? 65535 : 25500;
  range = GetUnsignedValue(device, "RANGE", max_range);
  AddInteger(_("Range"), NULL, _T("%d m"), _T("%d"), 2000, max_range, 250, range);

  if (hardware.isPowerFlarm()) {
    vrange = GetUnsignedValue(device, "VRANGE", 500);
    AddInteger(_("Vertical range"), NULL, _T("%d m"), _T("%d"), 100, 2000, 100, vrange);
  }

  if (hardware.hasADSB()) {
    pcas_range = GetUnsignedValue(device, "PCASRANGE", 7408);
    pcas_vrange = GetUnsignedValue(device, "PCASVRANGE", 610);
    adsb_range = GetUnsignedValue(device, "ADSBRANGE", 65535);
    adsb_vrange = GetUnsignedValue(device, "ADSBVRANGE", 65535);
    AddInteger(_("PCAS range"), NULL, _T("%d m"), _T("%d"), 500, 9260, 500, pcas_range);
    AddInteger(_("PCAS vertical range"), NULL, _T("%d m"), _T("%d"), 250, 65535, 250, pcas_vrange);
    AddInteger(_("ADSB range"), NULL, _T("%d m"), _T("%d"), 500, 65535, 500, adsb_range);
    AddInteger(_("ADSB vertical range"), NULL, _T("%d m"), _T("%d"), 250, 65535, 250, adsb_vrange);
  }
}

bool
FLARMRangeConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;

  if (SaveValueInteger(Range, range)) {
    device.SendSetting("RANGE", fmt::format_int{range}.c_str(), env);
    changed = true;
  }
  if (hardware.hasADSB()) {
    if (SaveValueInteger(VRange, vrange)) {
      device.SendSetting("VRANGE", fmt::format_int{vrange}.c_str(), env);
      changed = true;
    }
  }

  if (hardware.hasADSB()) {
    if (SaveValueInteger(PCASRange, pcas_range)) {
      device.SendSetting("PCASRANGE", fmt::format_int{pcas_range}.c_str(), env);
      changed = true;
    }

    if (SaveValueInteger(PCASVRange, pcas_vrange)) {
      device.SendSetting("PCASVRANGE", fmt::format_int{pcas_vrange}.c_str(), env);
      changed = true;
    }

    if (SaveValueInteger(ADSBRange, adsb_range)) {
      device.SendSetting("ADSBRANGE", fmt::format_int{adsb_range}.c_str(), env);
      changed = true;
    }

    if (SaveValueInteger(ADSBVrange, adsb_vrange)) {
      device.SendSetting("ADSBVRANGE", fmt::format_int{adsb_vrange}.c_str(), env);
      changed = true;
    }
  }

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), _T("FLARM"));
  return false;
}
