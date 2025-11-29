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

static const char *const lxnav_vario_setting_names[] = {
  "BRGPS",
  "BRPDA",
  "VOL",
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

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), "Vega");
  return false;
}
