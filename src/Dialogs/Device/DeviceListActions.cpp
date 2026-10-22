// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceListActions.hpp"
#include "BlueFly/BlueFlyDialogs.hpp"
#include "GDL90/ConfigurationDialog.hpp"
#include "LX/ManageLX16xxDialog.hpp"
#include "LX/ManageLXNAVVarioDialog.hpp"
#include "LX/ManageNanoDialog.hpp"
#include "ManageCAI302Dialog.hpp"
#include "ManageFlarmDialog.hpp"
#include "ManageI2CPitotDialog.hpp"
#include "Stratux/ConfigurationDialog.hpp"
#include "Vega/VegaDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Features.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Device/Config.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/State.hpp"
#include "FLARM/Version.hpp"
#include "Language/Language.hpp"
#include "NMEA/DeviceInfo.hpp"
#include "NMEA/Info.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "util/StaticString.hxx"

void
ManageSelectedDevice(DeviceBlackboard &device_blackboard,
                     MultipleDevices *devices, const DialogLook &look,
                     unsigned current) noexcept
{
  if (devices == nullptr || current >= NUMDEV)
    return;

  DeviceDescriptor &descriptor = (*devices)[current];
  if (!descriptor.IsManageable())
    return;

#ifdef ANDROID
  const auto &config = descriptor.GetConfig();
  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2 ||
      (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR &&
       config.press_use == DeviceConfig::PressureUse::PITOT)) {
    ManageI2CPitotDialog(UIGlobals::GetMainWindow(), look, descriptor);
    return;
  }
#endif

  if (descriptor.IsDriver("GDL90")) {
    ManageGDL90Dialog();
    return;
  }

  if (descriptor.GetState() != PortState::READY) {
    ShowMessageBox(_("Device is not connected"), _("Manage"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  if (!descriptor.Borrow()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  MessageOperationEnvironment env;
  const ScopeReturnDevice return_device{descriptor, env};

  Device *device = descriptor.GetDevice();
  if (device == nullptr)
    return;

  if (descriptor.IsDriver("CAI 302"))
    ManageCAI302Dialog(UIGlobals::GetMainWindow(), look, *device);
  else if (descriptor.IsDriver("Stratux"))
    ManageStratuxDialog(*device);
  else if (descriptor.IsDriver("FLARM")) {
    FlarmVersion version;
    FlarmHardware hardware;
    FlarmState state;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(current);
      version = basic.flarm.version;
      state = basic.flarm.state;
    }

    ManageFlarmDialog(*device, version, hardware, state);
  } else if (descriptor.IsDriver("LX")) {
    DeviceInfo info, secondary_info;

    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(current);
      info = basic.device;
      secondary_info = basic.secondary_device;
    }

    LXDevice &lx_device = *(LXDevice *)device;
    if (lx_device.IsLXNAVVario())
      ManageLXNAVVarioDialog(lx_device, info, secondary_info);
    else if (lx_device.IsNano())
      ManageNanoDialog(lx_device, info);
    else if (lx_device.IsLX16xx())
      ManageLX16xxDialog(lx_device, info);
  } else if (descriptor.IsDriver("Vega"))
    dlgConfigurationVarioShowModal(*device);
  else if (descriptor.IsDriver("BlueFly"))
    dlgConfigurationBlueFlyVarioShowModal(*device);
}

void
DebugSelectedDevice(MultipleDevices *devices, unsigned current) noexcept
{
  if (devices == nullptr || current >= NUMDEV)
    return;

  DeviceDescriptor &device = (*devices)[current];
  if (!device.GetConfig().UsesPort() ||
      device.GetState() != PortState::READY)
    return;

  static constexpr unsigned MINUTES = 10;

  device.EnableDumpTemporarily(std::chrono::minutes(MINUTES));

  StaticString<256> msg;
  msg.Format(_("Communication with this device will be logged for the next %u minutes."),
             MINUTES);
  ShowMessageBox(msg, _("Debug"), MB_OK | MB_ICONINFORMATION);
}
