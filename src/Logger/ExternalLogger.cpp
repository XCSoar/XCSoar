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

#include "Logger/ExternalLogger.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "Device/Descriptor.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/RecordedFlight.hpp"
#include "Components.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "UIGlobals.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Job/TriStateJob.hpp"
#include "OS/Path.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/FileTransaction.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCHeader.hpp"
#include "Formatter/IGCFilenameFormatter.hpp"
#include "Time/BrokenDate.hpp"

class DeclareJob {
  DeviceDescriptor &device;
  const struct Declaration &declaration;
  const Waypoint *home;

public:
  DeclareJob(DeviceDescriptor &_device, const struct Declaration &_declaration,
             const Waypoint *_home)
    :device(_device), declaration(_declaration), home(_home) {}

  bool Run(OperationEnvironment &env) {
    bool result = device.Declare(declaration, home, env);
    device.EnableNMEA(env);
    return result;
  }
};

static TriStateJobResult
DoDeviceDeclare(DeviceDescriptor &device, const Declaration &declaration,
                const Waypoint *home)
try {
  TriStateJob<DeclareJob> job(device, declaration, home);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            _T(""), job, true);
  return job.GetResult();
} catch (...) {
  LogError(std::current_exception());
  return TriStateJobResult::ERROR;
}

static bool
DeviceDeclare(DeviceDescriptor &dev, const Declaration &declaration,
              const Waypoint *home)
{
  if (dev.IsOccupied())
    return false;

  if (ShowMessageBox(_("Declare task?"), dev.GetDisplayName(),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  if (!dev.Borrow())
    return false;

  const TCHAR *caption = dev.GetDisplayName();
  if (caption == nullptr)
    caption = _("Declare task");

  auto result = DoDeviceDeclare(dev, declaration, home);
  dev.Return();

  switch (result) {
  case TriStateJobResult::SUCCESS:
    ShowMessageBox(_("Task declared!"),
                   caption, MB_OK | MB_ICONINFORMATION);
    return true;

  case TriStateJobResult::ERROR:
    ShowMessageBox(_("Error occured,\nTask NOT declared!"),
                   caption, MB_OK | MB_ICONERROR);
    return false;

  case TriStateJobResult::CANCELLED:
    return false;
  }

  gcc_unreachable();
}

void
ExternalLogger::Declare(const Declaration &decl, const Waypoint *home)
{
  bool found_logger = false;

  for (DeviceDescriptor *i : *devices) {
    DeviceDescriptor &device = *i;

    if (device.CanDeclare() && device.GetState() == PortState::READY) {
      found_logger = true;
      DeviceDeclare(device, decl, home);
    }
  }

  if (!found_logger)
    ShowMessageBox(_("No logger connected"),
                _("Declare task"), MB_OK | MB_ICONINFORMATION);
}

class ReadFlightListJob {
  DeviceDescriptor &device;
  RecordedFlightList &flight_list;

public:
  ReadFlightListJob(DeviceDescriptor &_device,
                    RecordedFlightList &_flight_list)
    :device(_device), flight_list(_flight_list) {}

  bool Run(OperationEnvironment &env) {
    return device.ReadFlightList(flight_list, env);
  }
};

static TriStateJobResult
DoReadFlightList(DeviceDescriptor &device, RecordedFlightList &flight_list)
try {
  TriStateJob<ReadFlightListJob> job(device, flight_list);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            _T(""), job, true);
  return job.GetResult();
} catch (...) {
  LogError(std::current_exception());
  return TriStateJobResult::ERROR;
}

class DownloadFlightJob {
  DeviceDescriptor &device;
  const RecordedFlightInfo &flight;
  const Path path;

public:
  DownloadFlightJob(DeviceDescriptor &_device,
                    const RecordedFlightInfo &_flight, const Path _path)
    :device(_device), flight(_flight), path(_path) {}

  bool Run(OperationEnvironment &env) {
    return device.DownloadFlight(flight, path, env);
  }
};

static TriStateJobResult
DoDownloadFlight(DeviceDescriptor &device,
                 const RecordedFlightInfo &flight, Path path)
try {
  TriStateJob<DownloadFlightJob> job(device, flight, path);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            _T(""), job, true);
  return job.GetResult();
} catch (...) {
  LogError(std::current_exception());
  return TriStateJobResult::ERROR;
}

static void
ReadIGCMetaData(Path path, IGCHeader &header, BrokenDate &date)
try {
  strcpy(header.manufacturer, "XXX");
  strcpy(header.id, "000");
  header.flight = 0;

  FileLineReaderA reader(path);

  char *line = reader.ReadLine();
  if (line != nullptr)
    IGCParseHeader(line, header);

  line = reader.ReadLine();
  if (line == nullptr || !IGCParseDateRecord(line, date))
    date = BrokenDate::TodayUTC();
} catch (...) {
  date = BrokenDate::TodayUTC();
}

/**
 *
 * @param list list of flights from the logger
 * @param flight the flight
 * @return 1-99 Flight number of the day per section 2.5 of the
 * FAI IGC tech gnss spec Appendix 1
 * (spec says 35 flights - this handles up to 99 flights per day)
 */
static unsigned
GetFlightNumber(const RecordedFlightList &flight_list,
                const RecordedFlightInfo &flight)
{
  unsigned flight_number = 1;
  for (auto it = flight_list.begin(), end = flight_list.end(); it != end; ++it) {
    const RecordedFlightInfo &_flight = *it;
    if (flight.date == _flight.date &&
        (flight.start_time.GetSecondOfDay() >
         _flight.start_time.GetSecondOfDay()))
      flight_number++;
  }
  return flight_number;
}

static const RecordedFlightInfo *
ShowFlightList(const RecordedFlightList &flight_list)
{
  // Prepare list of the flights for displaying
  ComboList combo;
  for (unsigned i = 0; i < flight_list.size(); ++i) {
    const RecordedFlightInfo &flight = flight_list[i];

    StaticString<64> buffer;
    buffer.UnsafeFormat(_T("%04u/%02u/%02u %02u:%02u-%02u:%02u"),
                        flight.date.year, flight.date.month, flight.date.day,
                        flight.start_time.hour, flight.start_time.minute,
                        flight.end_time.hour, flight.end_time.minute);

    combo.Append(i, buffer);
  }

  // Show list of the flights
  int i = ComboPicker(_T("Choose a flight"),
                      combo, nullptr, false);

  return i < 0 ? nullptr : &flight_list[i];
}

void
ExternalLogger::DownloadFlightFrom(DeviceDescriptor &device)
{
  MessageOperationEnvironment env;

  // Download the list of flights that the logger contains
  RecordedFlightList flight_list;
  switch (DoReadFlightList(device, flight_list)) {
  case TriStateJobResult::SUCCESS:
    break;

  case TriStateJobResult::ERROR:
    device.EnableNMEA(env);
    ShowMessageBox(_("Failed to download flight list."),
                _("Download flight"), MB_OK | MB_ICONERROR);
    return;

  case TriStateJobResult::CANCELLED:
    return;
  }

  // The logger seems to be empty -> cancel
  if (flight_list.empty()) {
    device.EnableNMEA(env);
    ShowMessageBox(_("Logger is empty."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const auto logs_path = MakeLocalPath(_T("logs"));

  while (true) {
    // Show list of the flights
    const RecordedFlightInfo *flight = ShowFlightList(flight_list);
    if (!flight)
      break;

    // Download chosen IGC file into temporary file
    FileTransaction transaction(AllocatedPath::Build(logs_path,
                                                     _T("temp.igc")));
    switch (DoDownloadFlight(device, *flight, transaction.GetTemporaryPath())) {
    case TriStateJobResult::SUCCESS:
      break;

    case TriStateJobResult::ERROR:
      ShowMessageBox(_("Failed to download flight."),
                  _("Download flight"), MB_OK | MB_ICONERROR);
      continue;

    case TriStateJobResult::CANCELLED:
      continue;
    }

    /* read the IGC header and build the final IGC file name with it */

    IGCHeader header;
    BrokenDate date;
    ReadIGCMetaData(transaction.GetTemporaryPath(), header, date);
    if (header.flight == 0)
      header.flight = GetFlightNumber(flight_list, *flight);

    TCHAR name[64];
    FormatIGCFilenameLong(name, date, header.manufacturer, header.id,
                          header.flight);

    transaction.SetPath(AllocatedPath::Build(logs_path, name));
    transaction.Commit();

    if (ShowMessageBox(_("Do you want to download another flight?"),
                    _("Download flight"), MB_YESNO | MB_ICONQUESTION) != IDYES)
      break;
  }

  device.EnableNMEA(env);
}
