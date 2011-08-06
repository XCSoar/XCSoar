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
#include "Dialogs/ManageCAI302Dialog.hpp"
#include "Util/StaticArray.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "DeviceBlackboard.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Language/Language.hpp"
#include "Operation.hpp"
#include "Simulator.hpp"
#include "Logger/ExternalLogger.hpp"

static WndForm *dialog;
static WndListFrame *list;
static unsigned current;
static unsigned font_height;

static WndButton *restart_button, *flight_button, *manage_button;

static StaticArray<unsigned, NUMDEV> indices;

static void
UpdateButtons()
{
  if (is_simulator() || current >= indices.size()) {
    restart_button->set_enabled(false);
    flight_button->set_enabled(false);
    manage_button->set_enabled(false);
  } else {
    const DeviceDescriptor &device = DeviceList[indices[current]];

    restart_button->set_enabled(device.IsConfigured());
    flight_button->set_enabled(device.IsLogger());
    manage_button->set_enabled(device.IsDriver(_T("CAI 302")));
  }
}

static void
RefreshList()
{
  indices.clear();
  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].IsConfigured())
      indices.append(i);

  list->SetLength(indices.size());
  list->invalidate();
}

static void
PaintDevice(Canvas &canvas, const PixelRect rc, unsigned i)
{
  const DeviceDescriptor &device = DeviceList[indices[i]];

  const unsigned margin = Layout::Scale(1);

  TCHAR buffer1[256], buffer2[256];
  const DeviceConfig &config = device.GetConfig();
  const TCHAR *name = config.GetPortName(buffer1, 128);

  if (config.UsesDriver()) {
    _sntprintf(buffer2, 128, _("%s on %s"), config.driver_name.c_str(), name);
    name = buffer2;
  }

  canvas.text(rc.left + margin, rc.top + margin, name);

  /* show a list of features that are available in the second row */

  const NMEAInfo &basic = device_blackboard.RealState(indices[i]);
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

    if (basic.flarm.IsDetected())
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
OnRestartClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = DeviceList[indices[current]];
  MessageOperationEnvironment env;
  device.Reopen(env);
}

static void
OnFlightDownloadClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &device = DeviceList[indices[current]];
  ExternalLogger::DownloadFlightFrom(device);
}

static void
OnManageClicked(gcc_unused WndButton &button)
{
  if (current >= indices.size())
    return;

  DeviceDescriptor &descriptor = DeviceList[indices[current]];
  if (!descriptor.IsDriver(_T("CAI 302")) || descriptor.IsBusy())
    return;

  Device *device = descriptor.GetDevice();
  if (device == NULL)
    return;

  descriptor.SetBusy(true);
  ManageCAI302Dialog(dialog->GetMainWindow(), dialog->GetLook(), *device);
  descriptor.SetBusy(false);
}

void
ShowDeviceList(SingleWindow &parent, const DialogLook &look)
{
  unsigned margin = Layout::Scale(2);

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.hide();
  dialog_style.control_parent();

  PixelSize size = parent.get_size();
  dialog = new WndForm(parent, look, 0, 0, size.cx, size.cy,
                       _("Devices"), dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, look);
  const PixelRect rc = buttons.GetRemainingRect();

  /* create the list */

  current = 0;

  font_height = look.text_font->get_height();

  WindowStyle list_style;
  list_style.tab_stop();
  list_style.border();

  list = new WndListFrame(client_area, look,
                          rc.left + margin, rc.top + margin,
                          rc.right - rc.left - 2 * margin,
                          rc.bottom - rc.top - 2 * margin,
                          list_style, 3 * margin + 2 * font_height);
  list->SetPaintItemCallback(PaintDevice);
  list->SetCursorCallback(OnCursor);

  RefreshList();

  /* create buttons */

  buttons.Add(_("Close"), OnCloseClicked);
  buttons.Add(_("Refresh"), OnRefreshClicked);
  restart_button = buttons.Add(_("Restart"), OnRestartClicked);
  flight_button = buttons.Add(_("Flight download"), OnFlightDownloadClicked);
  manage_button = buttons.Add(_("Manage"), OnManageClicked);

  UpdateButtons();

  /* run it */

  dialog->ShowModal();

  delete list;
  delete dialog;
}
