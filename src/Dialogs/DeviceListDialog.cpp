/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/DeviceListDialog.hpp"
#include "Dialogs/Vega/VegaDialogs.hpp"
#include "Dialogs/ManageCAI302Dialog.hpp"
#include "Dialogs/ManageFlarmDialog.hpp"
#include "Dialogs/PortMonitor.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Widgets/DeviceEditWidget.hpp"
#include "UIGlobals.hpp"
#include "Util/TrivialArray.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Look/DialogLook.hpp"
#include "Form/List.hpp"
#include "Form/ListWidget.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"

class DeviceListWidget : public ListWidget, private ActionListener {
  enum Buttons {
    REFRESH, RECONNECT, FLIGHT, EDIT, MANAGE, MONITOR,
  };

  const DialogLook &look;
  const TerminalLook &terminal_look;

  UPixelScalar font_height;

  TrivialArray<unsigned, NUMDEV> indices;

  WndButton *reconnect_button, *flight_button;
  WndButton *edit_button;
  WndButton *manage_button, *monitor_button;

public:
  DeviceListWidget(const DialogLook &_look, const TerminalLook &_terminal_look)
    :look(_look), terminal_look(_terminal_look) {}

  void CreateButtons(WidgetDialog &dialog);

protected:
  gcc_pure
  bool IsFlarm(unsigned index) const {
    return device_blackboard->RealState(indices[index]).flarm.IsDetected();
  }

  void RefreshList();

  void UpdateButtons();

  void ReconnectCurrent();
  void DownloadFlightFromCurrent();
  void EditCurrent();
  void ManageCurrent();
  void MonitorCurrent();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare() {
    DeleteWindow();
  }

  /* virtual methods from class List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx);
  virtual void OnCursorMoved(unsigned index);

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id);
};

void
DeviceListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  UPixelScalar margin = Layout::Scale(2);
  font_height = look.list.font->GetHeight();
  CreateList(parent, look, rc, 3 * margin + 2 * font_height);
  RefreshList();
  UpdateButtons();
}

void
DeviceListWidget::RefreshList()
{
  indices.clear();
  for (unsigned i = 0; i < NUMDEV; ++i)
    if (device_list[i]->IsConfigured())
      indices.append(i);

  ListControl &list = GetList();
  list.SetLength(indices.size());
  list.Invalidate();
}

void
DeviceListWidget::CreateButtons(WidgetDialog &dialog)
{
  dialog.AddButton(_("Refresh"), this, REFRESH);
  reconnect_button = dialog.AddButton(_("Reconnect"), this, RECONNECT);
  flight_button = dialog.AddButton(_("Flight download"), this, FLIGHT);
  edit_button = dialog.AddButton(_("Edit"), this, EDIT);
  manage_button = dialog.AddButton(_("Manage"), this, MANAGE);
  monitor_button = dialog.AddButton(_("Monitor"), this, MONITOR);
}

void
DeviceListWidget::UpdateButtons()
{
  const unsigned current = GetList().GetCursorIndex();

  if (is_simulator() || current >= indices.size()) {
    reconnect_button->SetEnabled(false);
    flight_button->SetEnabled(false);
    manage_button->SetEnabled(false);
    monitor_button->SetEnabled(false);
  } else {
    const DeviceDescriptor &device = *device_list[indices[current]];

    reconnect_button->SetEnabled(device.IsConfigured());
    flight_button->SetEnabled(device.IsLogger());
    manage_button->SetEnabled(device.IsManageable());
    monitor_button->SetEnabled(device.GetConfig().UsesPort());
  }

  edit_button->SetEnabled(current < indices.size());
}

void
DeviceListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  const DeviceDescriptor &device = *device_list[indices[i]];

  const UPixelScalar margin = Layout::Scale(2);

  TCHAR buffer1[256], buffer2[256];
  const DeviceConfig &config = device.GetConfig();
  const TCHAR *name = config.GetPortName(buffer1, 128);

  if (config.UsesDriver()) {
    _sntprintf(buffer2, 128, _("%s on %s"), config.driver_name.c_str(), name);
    name = buffer2;
  }

  canvas.text(rc.left + margin, rc.top + margin, name);

  /* show a list of features that are available in the second row */

  const NMEAInfo &basic = device_blackboard->RealState(indices[i]);

  const TCHAR *text;
  if (basic.alive) {
    if (basic.location_available) {
      _tcscpy(buffer1, _("GPS fix"));
    } else if (basic.gps.fix_quality_available) {
      /* device sends GPGGA, but no valid location */
      _tcscpy(buffer1, _("Bad GPS"));
    } else {
      _tcscpy(buffer1, _("Connected"));
    }

    if (basic.baro_altitude_available) {
      _tcscat(buffer1, _T("; "));
      _tcscat(buffer1, _("Baro"));
    }

    if (basic.airspeed_available) {
      _tcscat(buffer1, _T("; "));
      _tcscat(buffer1, _("Airspeed"));
    }

    if (basic.total_energy_vario_available) {
      _tcscat(buffer1, _T("; "));
      _tcscat(buffer1, _("Vario"));
    }

    if (IsFlarm(i))
      _tcscat(buffer1, _T("; FLARM"));

    text = buffer1;
  } else if (is_simulator()) {
    text = _("N/A");
  } else if (device.IsOpen()) {
    text = _("No data");
  } else {
    text = _("Not connected");
  }

  canvas.text(rc.left + margin, rc.top + 2 * margin + font_height,
              text);
}

void
DeviceListWidget::OnCursorMoved(unsigned index)
{
  UpdateButtons();
}

void
DeviceListWidget::ReconnectCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = *device_list[indices[current]];
  if (device.IsBorrowed()) {
    MessageBoxX(_("Device is occupied"), _("Reconnect"), MB_OK | MB_ICONERROR);
    return;
  }

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  device.Reopen(env);
}

void
DeviceListWidget::DownloadFlightFromCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = *device_list[indices[current]];
  if (!device.Borrow()) {
    MessageBoxX(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  ExternalLogger::DownloadFlightFrom(device);
  device.Return();
}

void
DeviceListWidget::EditCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= indices.size())
    return;

  const unsigned index = indices[current];
  DeviceConfig &config = CommonInterface::SetSystemSettings().devices[index];
  DeviceEditWidget widget(config);

  if (!DefaultWidgetDialog(_("Edit device"), widget))
    /* not modified */
    return;

  /* save new config to profile .. */

  config = widget.GetConfig();
  Profile::SetDeviceConfig(index, config);
  Profile::Save();

  /* .. and reopen the device */

  DeviceDescriptor &descriptor = *device_list[index];
  descriptor.SetConfig(widget.GetConfig());

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  descriptor.Reopen(env);
}

void
DeviceListWidget::ManageCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = *device_list[indices[current]];
  if (!descriptor.IsManageable())
    return;

  if (!descriptor.Borrow()) {
    MessageBoxX(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  Device *device = descriptor.GetDevice();
  if (device == NULL) {
    descriptor.Return();
    return;
  }

  if (descriptor.IsDriver(_T("CAI 302")))
    ManageCAI302Dialog(UIGlobals::GetMainWindow(), look, *device);
  else if (descriptor.IsDriver(_T("FLARM")))
    ManageFlarmDialog(*device);
  else if (descriptor.IsDriver(_T("Vega")))
    dlgConfigurationVarioShowModal(*device);

  MessageOperationEnvironment env;
  descriptor.EnableNMEA(env);
  descriptor.Return();
}

void
DeviceListWidget::MonitorCurrent()
{
  const unsigned current = GetList().GetCursorIndex();
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = *device_list[indices[current]];
  ShowPortMonitor(UIGlobals::GetMainWindow(), look, terminal_look,
                  descriptor);
}

void
DeviceListWidget::OnAction(int id)
{
  switch (id) {
  case REFRESH:
    RefreshList();
    UpdateButtons();
    break;

  case RECONNECT:
    ReconnectCurrent();
    break;

  case FLIGHT:
    DownloadFlightFromCurrent();
    break;

  case EDIT:
    EditCurrent();
    break;

  case MANAGE:
    ManageCurrent();
    break;

  case MONITOR:
    MonitorCurrent();
    break;
  }
}

void
ShowDeviceList(SingleWindow &parent, const DialogLook &look,
               const TerminalLook &terminal_look)
{
  DeviceListWidget widget(look, terminal_look);

  WidgetDialog dialog(_("Devices"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}
