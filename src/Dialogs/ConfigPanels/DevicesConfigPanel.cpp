/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/List.hpp"
#include "Form/DockWindow.hpp"
#include "Widgets/DeviceEditWidget.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "Asset.hpp"
#include "Protection.hpp"
#include "DevicesConfigPanel.hpp"
#include "Language/Language.hpp"
#include "Compatibility/string.h"
#include "Util/Macros.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"

#ifdef _WIN32_WCE
#include "Device/Windows/Enumerator.hpp"
#endif

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#ifdef IOIOLIB
#include "Device/Port/AndroidIOIOUartPort.hpp"
#endif
#endif


static const struct {
  DeviceConfig::PortType type;
  const TCHAR *label;
} port_types[] = {
  { DeviceConfig::PortType::DISABLED, N_("Disabled") },
#ifdef _WIN32_WCE
  { DeviceConfig::PortType::AUTO, N_("GPS Intermediate Driver") },
#endif
#ifdef ANDROID
  { DeviceConfig::PortType::INTERNAL, N_("Built-in GPS") },
#endif

  /* label not translated for now, until we have a TCP port
     selection UI */
  { DeviceConfig::PortType::TCP_LISTENER, _T("TCP port") },

  { DeviceConfig::PortType::SERIAL, NULL } /* sentinel */
};

/** the number of fixed port types (excludes Serial, Bluetooth and IOIOUart) */
static const unsigned num_port_types = ARRAY_SIZE(port_types) - 1;

class DevicesConfigPanel : public XMLWidget {
  DeviceConfig device_config[NUMDEV];

  unsigned current_device;

  gcc_pure
  DeviceEditWidget &GetEditWidget() {
    DockWindow *dock = (DockWindow *)form.FindByName(_T("edit"));
    assert(dock != NULL);
    return *(DeviceEditWidget *)dock->GetWidget();
  }

  bool SaveDeviceConfig();

public:
  const DeviceConfig &GetDeviceConfig(unsigned i) const {
    assert(i < NUMDEV);

    return device_config[i];
  }

  void ShowDevice(unsigned idx);

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static DevicesConfigPanel *instance;

bool
DevicesConfigPanel::SaveDeviceConfig()
{
  bool changed = false, require_restart = false;
  DeviceEditWidget &widget = GetEditWidget();
  if (!widget.Save(changed, require_restart))
    return false;

  if (changed) {
    device_config[current_device] = widget.GetConfig();
    DevicePortChanged = true;
  }

  return true;
}

void
DevicesConfigPanel::ShowDevice(unsigned idx)
{
  assert(idx < NUMDEV);

  if (idx == current_device)
    return;

  if (!SaveDeviceConfig())
    return;

  current_device = idx;
  GetEditWidget().SetConfig(device_config[current_device]);
}

static void
DeviceListCursorCallback(unsigned idx)
{
  instance->ShowDevice(idx);
}

static void
PaintDeviceListItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const DeviceConfig &config = instance->GetDeviceConfig(idx);

  const UPixelScalar margin = Layout::Scale(2);

  TCHAR buffer1[256], buffer2[256];
  const TCHAR *name = config.GetPortName(buffer1, 128);

  if (config.UsesDriver()) {
    _sntprintf(buffer2, 128, _("%s on %s"), config.driver_name.c_str(), name);
    name = buffer2;
  }

  canvas.text(rc.left + margin, rc.top + margin, name);
}

void
DevicesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_DEVICESCONFIGPANEL") :
                               _T("IDR_XML_DEVICESCONFIGPANEL_L"));

  current_device = 0;

  for (unsigned i = 0; i < NUMDEV; ++i)
    Profile::GetDeviceConfig(i, device_config[i]);

  DockWindow *dock = (DockWindow *)form.FindByName(_T("edit"));
  assert(dock != NULL);
  DeviceEditWidget *edit = new DeviceEditWidget(device_config[0]);
  dock->SetWidget(edit);

  WndListFrame *list = (WndListFrame *)form.FindByName(_T("list"));
  assert(list != NULL);
  list->SetPaintItemCallback(PaintDeviceListItem);
  list->SetLength(2);
  list->SetCursorCallback(DeviceListCursorCallback);

  LoadFormProperty(form, _T("prpSetSystemTimeFromGPS"),
                   CommonInterface::GetComputerSettings().set_system_time_from_gps);

  LoadFormProperty(form, _T("prpIgnoreNMEAChecksum"),
                   NMEAParser::ignore_checksum);
}


bool
DevicesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  if (!SaveDeviceConfig())
    return false;

  changed |= SaveFormProperty(form, _T("prpSetSystemTimeFromGPS"),
                              szProfileSetSystemTimeFromGPS,
                              CommonInterface::SetComputerSettings().set_system_time_from_gps);

  changed |= SaveFormProperty(form, _T("prpIgnoreNMEAChecksum"),
                              szProfileIgnoreNMEAChecksum,
                              NMEAParser::ignore_checksum);

  if (DevicePortChanged) {
    changed = true;
    for (unsigned i = 0; i < NUMDEV; ++i)
      Profile::SetDeviceConfig(i, device_config[i]);
  }

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateDevicesConfigPanel()
{
  return new DevicesConfigPanel();
}

