// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LXNAVVarioConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "util/NumberParser.hpp"
#include "Math/Util.hpp"
#include "NMEA/InputLine.hpp"

static const char *const lxnav_vario_setting_names[] = {
  "BRGPS",
  "BRPDA",
  "VOL",
  "ALTOFF",
  NULL
};

static bool
RequestAllSettings(LXDevice &device)
{
  PopupOperationEnvironment env;

  for (auto i = lxnav_vario_setting_names; *i != NULL; ++i)
    if (!device.RequestLXNAVVarioSetting(*i, env))
      return false;

  return true;
}

static unsigned
WaitUnsignedValue(LXDevice &device, const char *name,
                  unsigned default_value)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitLXNAVVarioSetting(name, env, 500);
  if (!x.empty()) {
    char *endptr;
    /* VOL is a float (e.g., "70.5"), so parse as double first */
    double d = ParseDouble(x.c_str(), &endptr);
    if (endptr > x.c_str())
      return uround(d);
  }

  return default_value;
}

static void
WaitAltoffValues(LXDevice &device, int &error, int &qnh, int &takeoff)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitLXNAVVarioSetting("ALTOFF", env, 500);
  if (!x.empty()) {
    NMEAInputLine line(x.c_str());
    double d;

    /* Parse first value: Alt offset error */
    if (line.ReadChecked(d))
      error = iround(d);
    else
      error = 0;

    /* Parse second value: Alt offset qnh */
    if (line.ReadChecked(d))
      qnh = iround(d);
    else
      qnh = 0;

    /* Parse third value: Alt take off */
    if (line.ReadChecked(d))
      takeoff = iround(d);
    else
      takeoff = 0;

    return;
  }

  error = qnh = takeoff = 0;
}

void
LXNAVVarioConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  RequestAllSettings(device);

  brgps = WaitUnsignedValue(device, "BRGPS", 5);
  brpda = WaitUnsignedValue(device, "BRPDA", 5);

  static constexpr StaticEnumChoice baud_list[] = {
    { 0, "4800" },
    { 1, "9600" },
    { 2, "19200" },
    { 3, "38400" },
    { 4, "57600" },
    { 5, "115200" },
    { 6, "230400" },
    { 7, "256000" },
    { 8, "460800" },
    { 9, "500k" },
    { 10, "1M" },
    { 0 }
  };

  AddEnum(_("GPS baud rate"), NULL, baud_list, brgps);
  AddEnum(_("PDA baud rate"), NULL, baud_list, brpda);

  volume = WaitUnsignedValue(device, "VOL", 50);
  AddInteger(_("Volume"), NULL, "%u %%", "%u", 0, 100, 1, volume);

  WaitAltoffValues(device, altoff_error, altoff_qnh, altoff_takeoff);
  AddInteger(_("Alt offset error"), NULL, "%d m", "%d", -1000, 1000, 1, altoff_error);
  AddInteger(_("Alt offset QNH"), NULL, "%d m", "%d", -1000, 1000, 1, altoff_qnh);
  AddInteger(_("Alt take off"), NULL, "%d m", "%d", -1000, 1000, 1, altoff_takeoff);
}

bool
LXNAVVarioConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;
  StaticString<32> buffer;

  if (SaveValueEnum(BRGPS, brgps)) {
    buffer.UnsafeFormat("%u", brgps);
    device.SendLXNAVVarioSetting("BRGPS", buffer, env);
    changed = true;
  }

  if (SaveValueEnum(BRPDA, brpda)) {
    buffer.UnsafeFormat("%u", brpda);
    device.SendLXNAVVarioSetting("BRPDA", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(VOL, volume)) {
    buffer.UnsafeFormat("%u", volume);
    device.SendLXNAVVarioSetting("VOL", buffer, env);
    changed = true;
  }

  bool altoff_changed = SaveValueInteger(ALTOFF_ERROR, altoff_error);
  altoff_changed |= SaveValueInteger(ALTOFF_QNH, altoff_qnh);
  altoff_changed |= SaveValueInteger(ALTOFF_TAKEOFF, altoff_takeoff);
  if (altoff_changed) {
    buffer.UnsafeFormat("%d,%d,%d", altoff_error, altoff_qnh, altoff_takeoff);
    device.SendLXNAVVarioSetting("ALTOFF", buffer, env);
    changed = true;
  }

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), "LXNAV Vario");
  return false;
}
