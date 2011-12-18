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

#include "Dialogs/DeviceListDialog.hpp"
#include "Dialogs/Vega.hpp"
#include "Dialogs/ManageCAI302Dialog.hpp"
#include "Dialogs/PortMonitor.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widgets/DeviceEditWidget.hpp"
#include "UIGlobals.hpp"
#include "Util/TrivialArray.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Profile/Profile.hpp"

static const TerminalLook *terminal_look;
static WndForm *dialog;
static WndListFrame *list;
static unsigned current;
static UPixelScalar font_height;

static WndButton *reconnect_button, *flight_button, *manage_button;
static WndButton *edit_button;
static WndButton *monitor_button;

static TrivialArray<unsigned, NUMDEV> indices;

static void
UpdateButtons()
{
  if (is_simulator() || current >= indices.size()) {
    reconnect_button->set_enabled(false);
    flight_button->set_enabled(false);
    manage_button->set_enabled(false);
    monitor_button->set_enabled(false);
  } else {
    const DeviceDescriptor &device = device_list[indices[current]];

    reconnect_button->set_enabled(device.IsConfigured());
    flight_button->set_enabled(device.IsLogger());
    manage_button->set_enabled(device.IsManageable());
    monitor_button->set_enabled(device.GetConfig().UsesPort());
  }

  edit_button->set_enabled(current < indices.size());
}

static void
RefreshList()
{
  indices.clear();
  for (unsigned i = 0; i < NUMDEV; ++i)
    if (device_list[i].IsConfigured())
      indices.append(i);

  list->SetLength(indices.size());
  list->invalidate();
}

static bool
IsFlarm(unsigned index)
{
  return device_blackboard->RealState(indices[index]).flarm.IsDetected();
}

static void
PaintDevice(Canvas &canvas, const PixelRect rc, unsigned i)
{
  const DeviceDescriptor &device = device_list[indices[i]];

  const UPixelScalar margin = Layout::Scale(1);

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
  if (basic.connected) {
    _tcscpy(buffer1, _("Connected"));

    if (basic.location_available) {
      _tcscat(buffer1, _T("; "));
      _tcscat(buffer1, _("GPS fix"));
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

    canvas.text(rc.left + margin, rc.top + margin + font_height,
                buffer1);
  } else {
    canvas.text(rc.left + margin, rc.top + margin + font_height,
                _("Not connected"));
  }
}

static void
OnCursor(unsigned idx)
{
  current = idx;
  UpdateButtons();
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
OnRefreshClicked(gcc_unused WndButton &button)
{
  RefreshList();
  UpdateButtons();
}

static void
OnReconnectClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = device_list[indices[current]];
  MessageOperationEnvironment env;
  device.Reopen(env);
}

static void
OnFlightDownloadClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = device_list[indices[current]];
  ExternalLogger::DownloadFlightFrom(device);
}

static void
OnEditClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = device_list[indices[current]];
  DeviceEditWidget widget(descriptor.GetConfig());

  if (!WidgetDialog(_("Edit device"),
                    UIGlobals::GetMainWindow().get_client_rect(),
                    widget))
    /* not modified */
    return;

  /* save new config to profile .. */

  Profile::SetDeviceConfig(descriptor.GetIndex(),
                           widget.GetConfig());
  Profile::Save();

  /* .. and reopen the device */

  descriptor.SetConfig() = widget.GetConfig();
  MessageOperationEnvironment env;
  descriptor.Reopen(env);
}

static void
OnManageClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = device_list[indices[current]];
  if (!descriptor.IsManageable() || descriptor.IsBusy())
    return;

  Device *device = descriptor.GetDevice();
  if (device == NULL)
    return;

  descriptor.SetBusy(true);
  if (descriptor.IsDriver(_T("CAI 302")))
    ManageCAI302Dialog(dialog->GetMainWindow(), dialog->GetLook(), *device);
  if (descriptor.IsDriver(_T("Vega")))
    dlgConfigurationVarioShowModal();
  descriptor.SetBusy(false);
}

static void
OnMonitorClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = device_list[indices[current]];
  ShowPortMonitor(dialog->GetMainWindow(), dialog->GetLook(), *terminal_look,
                  descriptor);
}

void
ShowDeviceList(SingleWindow &parent, const DialogLook &look,
               const TerminalLook &_terminal_look)
{
  terminal_look = &_terminal_look;

  UPixelScalar margin = Layout::Scale(2);

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.hide();
  dialog_style.control_parent();

  dialog = new WndForm(parent, look, parent.get_client_rect(),
                       _("Devices"), dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, look);
  buttons.Add(_("Close"), OnCloseClicked);
  buttons.Add(_("Refresh"), OnRefreshClicked);
  reconnect_button = buttons.Add(_("Reconnect"), OnReconnectClicked);
  flight_button = buttons.Add(_("Flight download"), OnFlightDownloadClicked);
  edit_button = buttons.Add(_("Edit"), OnEditClicked);
  manage_button = buttons.Add(_("Manage"), OnManageClicked);
  monitor_button = buttons.Add(_("Monitor"), OnMonitorClicked);

  const PixelRect rc = buttons.GetRemainingRect();

  /* create the list */

  current = 0;

  font_height = look.list.font->GetHeight();

  WindowStyle list_style;
  list_style.tab_stop();
  list_style.border();

  list = new WndListFrame(client_area, look, rc.left, rc.top,
                          rc.right - rc.left, rc.bottom - rc.top,
                          list_style, 3 * margin + 2 * font_height);
  list->SetPaintItemCallback(PaintDevice);
  list->SetCursorCallback(OnCursor);

  RefreshList();

  /* update buttons */

  UpdateButtons();

  /* run it */

  dialog->ShowModal();

  delete list;
  delete dialog;
}
