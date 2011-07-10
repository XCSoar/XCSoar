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

#include "Logger/ExternalLogger.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "DataField/ComboList.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "Device/device.hpp"
#include "Device/Declaration.hpp"
#include "Device/Descriptor.hpp"
#include "Device/List.hpp"
#include "Device/Driver.hpp"
#include "Profile/DeclarationConfig.hpp"
#include "LocalPath.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "Operation.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Thread/JobThread.hpp"
#include "Job.hpp"

static bool DeclaredToDevice = false;

bool
ExternalLogger::IsDeclared()
{
  return DeclaredToDevice;
}

class DeclareJob : public Job {
  DeviceDescriptor &device;
  const struct Declaration &declaration;

  bool result;

public:
  DeclareJob(DeviceDescriptor &_device, const struct Declaration &_declaration)
    :device(_device), declaration(_declaration) {}

  bool GetResult() const {
    return result;
  }

  virtual void Run(OperationEnvironment &env) {
    result = device.Declare(declaration, env);
  }
};

static bool
DoDeviceDeclare(DeviceDescriptor &device, const Declaration &declaration)
{
  DeclareJob job(device, declaration);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.look->dialog,
            _T(""), job, true);
  return job.GetResult();
}

static bool
DeviceDeclare(DeviceDescriptor *dev, const Declaration &declaration)
{
  if (!dev->CanDeclare())
    return false;

  if (MessageBoxX(_("Declare task?"),
                  dev->GetDisplayName(), MB_YESNO| MB_ICONQUESTION) == IDYES) {
    if (DoDeviceDeclare(*dev, declaration)) {
      MessageBoxX(_("Task declared!"),
                  dev->GetDisplayName(), MB_OK| MB_ICONINFORMATION);
      DeclaredToDevice = true;
    } else {
      MessageBoxX(_("Error occured,\nTask NOT declared!"),
                  dev->GetDisplayName(), MB_OK| MB_ICONERROR);
    }
  }

  return true;
}

void
ExternalLogger::Declare(const OrderedTask& task)
{
  DeclaredToDevice = false;
  bool found_logger = false;

  // don't do anything if task is not valid
  if (!task.check_task())
    return;

  Declaration decl(&task);
  Profile::GetDeclarationConfig(decl, CommonInterface::SettingsComputer().plane);

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceDeclare(&DeviceList[i], decl))
      found_logger = true;

  if (!found_logger)
    MessageBoxX(_("No logger connected"),
                _("Declare task"), MB_OK | MB_ICONINFORMATION);
}

/**
 * Checks whether a Task is declared to the Logger.
 * If so, asks whether to invalidate the declaration.
 * @return True if a Task is NOT declared to the Logger, False otherwise
 */
bool
ExternalLogger::CheckDeclaration(void)
{
  // if (Task is not declared) -> return true;
  if (!IsDeclared())
    return true;

  if (MessageBoxX(_("OK to invalidate declaration?"),
                  _("Task declared"),
     MB_YESNO| MB_ICONQUESTION) == IDYES){
    DeclaredToDevice = false;
    return true;
  }

  return false;
}

class ReadFlightListJob : public Job {
  DeviceDescriptor &device;
  RecordedFlightList &flight_list;

  bool result;

public:
  ReadFlightListJob(DeviceDescriptor &_device,
                    RecordedFlightList &_flight_list)
    :device(_device), flight_list(_flight_list) {}

  bool GetResult() const {
    return result;
  }

  virtual void Run(OperationEnvironment &env) {
    result = device.ReadFlightList(flight_list, env);
  }
};

static bool
DoReadFlightList(DeviceDescriptor &device, RecordedFlightList &flight_list)
{
  device.SetBusy(true);
  ReadFlightListJob job(device, flight_list);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.look->dialog,
            _T(""), job, true);
  device.SetBusy(false);
  return job.GetResult();
}

class DownloadFlightJob : public Job {
  DeviceDescriptor &device;
  const RecordedFlightInfo &flight;
  const TCHAR *path;

  bool result;

public:
  DownloadFlightJob(DeviceDescriptor &_device,
                    const RecordedFlightInfo &_flight, const TCHAR *_path)
    :device(_device), flight(_flight), path(_path) {}

  bool GetResult() const {
    return result;
  }

  virtual void Run(OperationEnvironment &env) {
    result = device.DownloadFlight(flight, path, env);
  }
};

static bool
DoDownloadFlight(DeviceDescriptor &device,
                 const RecordedFlightInfo &flight, const TCHAR *path)
{
  device.SetBusy(true);
  DownloadFlightJob job(device, flight, path);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.look->dialog,
            _T(""), job, true);
  device.SetBusy(false);
  return job.GetResult();
}

static void
DownloadFlightFrom(DeviceDescriptor &device)
{
  RecordedFlightList flight_list;
  if (!DoReadFlightList(device, flight_list)) {
    MessageBoxX(_("Failed to download flight list."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  if (flight_list.empty()) {
    MessageBoxX(_("Logger is empty."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  ComboList combo;
  for (unsigned i = 0; i < flight_list.size(); ++i) {
    const RecordedFlightInfo &flight = flight_list[i];

    TCHAR buffer[64];
    _sntprintf(buffer, 64, _T("%04u/%02u/%02u %02u:%02u-%02u:%02u"),
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);

    combo.Append(i, buffer);
  }

  int i = ComboPicker(CommonInterface::main_window, _T("Choose a flight"),
                      combo, NULL, false);
  if (i < 0)
    return;

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("external.igc")); // XXX better file name

  if (!DoDownloadFlight(device, flight_list[i], path)) {
    MessageBoxX(_("Failed to download flight."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }
}

void
ExternalLogger::DownloadFlight()
{
  StaticArray<DeviceDescriptor*, NUMDEV> loggers;

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].IsLogger())
      loggers.append(&DeviceList[i]);

  if (loggers.empty()) {
    MessageBoxX(_("No logger connected"),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  if (loggers.size() == 1)
    DownloadFlightFrom(*loggers[0]);
  else {
    ComboList combo;
    for (unsigned i = 0; i < loggers.size(); ++i)
      combo.Append(i, loggers[i]->GetDisplayName());

    int i = ComboPicker(CommonInterface::main_window, _T("Choose a logger"),
                        combo, NULL, false);
    if (i < 0)
      return;

    DownloadFlightFrom(*loggers[i]);
  }
}
