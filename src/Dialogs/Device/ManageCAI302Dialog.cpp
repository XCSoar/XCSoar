/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "ManageCAI302Dialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/FilePicker.hpp"
#include "Dialogs/JobDialog.hpp"
#include "UIGlobals.hpp"
#include "CAI302/UnitsEditor.hpp"
#include "CAI302/WaypointUploader.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/CAI302/Internal.hpp"
#include "Device/Driver/CAI302/Protocol.hpp"
#include "Waypoint/Patterns.hpp"

class ManageCAI302Widget final
  : public RowFormWidget, private ActionListener {
  enum Controls {
    Units,
    Waypoints,
    StartLogger,
    StopLogger,
    DeleteAllFlights,
    Reboot,
  };

  CAI302Device &device;

public:
  ManageCAI302Widget(const DialogLook &look, CAI302Device &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;
};

void
ManageCAI302Widget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddButton(_("Units"), *this, Units);
  AddButton(_("Waypoints"), *this, Waypoints);
  AddButton(_("Start Logger"), *this, StartLogger);
  AddButton(_("Stop Logger"), *this, StopLogger);
  AddButton(_("Delete all flights"), *this, DeleteAllFlights);
  AddButton(_("Reboot"), *this, Reboot);
}

static void
EditUnits(const DialogLook &look, CAI302Device &device)
{
  CAI302::Pilot data;

  MessageOperationEnvironment env;
  if (!device.ReadActivePilot(data, env))
    return;

  CAI302UnitsEditor widget(look, data);
  if (!DefaultWidgetDialog(UIGlobals::GetMainWindow(),
                           UIGlobals::GetDialogLook(),
                           _("Units"), widget))
    return;

  device.WriteActivePilot(widget.GetData(), env);
}

static void
UploadWaypoints(const DialogLook &look, CAI302Device &device)
{
  const auto path = FilePicker(_("Waypoints"), WAYPOINT_FILE_PATTERNS);
  if (path == nullptr)
    return;

  CAI302WaypointUploader job(path, device);
  JobDialog(UIGlobals::GetMainWindow(), look, _("Waypoints"), job, true);
}

void
ManageCAI302Widget::OnAction(int id)
{
  switch (id) {
  case Units:
    EditUnits(GetLook(), device);
    break;

  case Waypoints:
    UploadWaypoints(GetLook(), device);
    break;

  case StartLogger:
    {
      MessageOperationEnvironment env;
      device.StartLogging(env);
    }
    break;

  case StopLogger:
    {
      MessageOperationEnvironment env;
      device.StopLogging(env);
    }
    break;

  case DeleteAllFlights:
    {
      if (ShowMessageBox(_("Do you really want to delete all flights from the device?"),
                      _T("CAI 302"), MB_YESNO) != IDYES)
        return;

      MessageOperationEnvironment env;
      device.ClearLog(env);
    }
    break;

  case Reboot:
    {
      MessageOperationEnvironment env;
      device.Reboot(env);
    }
    break;
  }
}

void
ManageCAI302Dialog(SingleWindow &parent, const DialogLook &look,
                   Device &device)
{
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _T("CAI 302"),
                    new ManageCAI302Widget(look, (CAI302Device &)device));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
