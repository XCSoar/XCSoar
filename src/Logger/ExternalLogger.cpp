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
#include "DataField/ComboList.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "Device/device.hpp"
#include "Device/Declaration.hpp"
#include "Device/Descriptor.hpp"
#include "Device/List.hpp"
#include "Device/Driver.hpp"
#include "LocalPath.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"
#include "Operation.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Thread/JobThread.hpp"
#include "Job.hpp"
#include "OS/FileUtil.hpp"
#include "IO/FileLineReader.hpp"
#include "Replay/IGCParser.hpp"

#include <windef.h> /* for MAX_PATH */

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
            CommonInterface::main_window.GetLook().dialog,
            _T(""), job, true);
  return job.GetResult();
}

static bool
DeviceDeclare(DeviceDescriptor &dev, const Declaration &declaration)
{
  if (MessageBoxX(_("Declare task?"), dev.GetDisplayName(),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  if (!DoDeviceDeclare(dev, declaration)) {
    MessageBoxX(_("Error occured,\nTask NOT declared!"),
                dev.GetDisplayName(), MB_OK | MB_ICONERROR);
    return false;
  }

  MessageBoxX(_("Task declared!"),
              dev.GetDisplayName(), MB_OK | MB_ICONINFORMATION);
  return true;
}

void
ExternalLogger::Declare(const Declaration &decl)
{
  bool found_logger = false;

  for (unsigned i = 0; i < NUMDEV; ++i) {
    if (device_list[i].CanDeclare()) {
      found_logger = true;
      DeviceDeclare(device_list[i], decl);
    }
  }

  if (!found_logger)
    MessageBoxX(_("No logger connected"),
                _("Declare task"), MB_OK | MB_ICONINFORMATION);
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
    result = device.EnableDownloadMode(env) &&
             device.ReadFlightList(flight_list, env);
  }
};

static bool
DoReadFlightList(DeviceDescriptor &device, RecordedFlightList &flight_list)
{
  device.SetBusy(true);
  ReadFlightListJob job(device, flight_list);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.GetLook().dialog,
            _T(""), job, true);
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
    if (!result) {
      device.DisableDownloadMode(env);
      device.SetBusy(false);
    }
  }
};

static bool
DoDownloadFlight(DeviceDescriptor &device,
                 const RecordedFlightInfo &flight, const TCHAR *path)
{
  DownloadFlightJob job(device, flight, path);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.GetLook().dialog,
            _T(""), job, true);
  return job.GetResult();
}

static void
ReadIGCMetaData(const TCHAR *path, IGCHeader &header, BrokenDate &date)
{
  strcpy(header.manufacturer, "XXX");
  strcpy(header.id, "000");
  header.flight = 0;

  FileLineReaderA reader(path);
  if (reader.error()) {
    date = BrokenDateTime::NowUTC();
    return;
  }

  char *line = reader.read();
  if (line != NULL)
    IGCParseHeader(line, header);

  line = reader.read();
  if (line == NULL || !IGCParseDate(line, date))
    date = BrokenDateTime::NowUTC();
}

static void
BuildIGCFileName(TCHAR *name, const IGCHeader &header, const BrokenDate &date)
{
  assert(strlen(header.manufacturer) == 3);
  assert(strlen(header.id) == 3);

  TCHAR manufacturer[4], id[4];
  /* poor man's char->TCHAR converted; this works because we know
     we're dealing with ASCII only */
  std::copy(header.manufacturer, header.manufacturer + 4, manufacturer);
  std::copy(header.id, header.id + 4, id);

  _stprintf(name, _T("%04u-%02u-%02u-%s-%s-%02u.igc"),
            date.year, date.month, date.day,
            manufacturer,id,
            header.flight);
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

    TCHAR buffer[64];
    _sntprintf(buffer, 64, _T("%04u/%02u/%02u %02u:%02u-%02u:%02u"),
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);

    combo.Append(i, buffer);
  }

  // Show list of the flights
  int i = ComboPicker(CommonInterface::main_window, _T("Choose a flight"),
                      combo, NULL, false);

  return (i < 0) ? NULL : &flight_list[i];
}

class DisableDownloadModeJob : public Job {
  DeviceDescriptor &device;
  bool result;

public:
  DisableDownloadModeJob(DeviceDescriptor &_device):device(_device) {}

  bool GetResult() const {
    return result;
  }

  virtual void Run(OperationEnvironment &env) {
    result = device.DisableDownloadMode(env);
  }
};

static bool
DisableDownloadMode(DeviceDescriptor &device)
{
  DisableDownloadModeJob job(device);
  JobDialog(CommonInterface::main_window,
            CommonInterface::main_window.GetLook().dialog,
            _T(""), job, true);
  device.SetBusy(false);
  return job.GetResult();
}

void
ExternalLogger::DownloadFlightFrom(DeviceDescriptor &device)
{
  // Download the list of flights that the logger contains
  RecordedFlightList flight_list;
  if (!DoReadFlightList(device, flight_list)) {
    DisableDownloadMode(device);
    MessageBoxX(_("Failed to download flight list."),
                _("Download flight"), MB_OK | MB_ICONERROR);
    return;
  }

  // The logger seems to be empty -> cancel
  if (flight_list.empty()) {
    DisableDownloadMode(device);
    MessageBoxX(_("Logger is empty."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  // Show list of the flights
  const RecordedFlightInfo *flight = ShowFlightList(flight_list);
  if (!flight) {
    DisableDownloadMode(device);
    return;
  }

  // Download chosen IGC file into temporary file
  TCHAR path[MAX_PATH];
  LocalPath(path, _T("logs"), _T("temp.igc"));
  if (!DoDownloadFlight(device, *flight, path)) {
    // Delete temporary file
    File::Delete(path);
    MessageBoxX(_("Failed to download flight."),
                _("Download flight"), MB_OK | MB_ICONERROR);
    return;
  }

  DisableDownloadMode(device);

  /* read the IGC header and build the final IGC file name with it */

  IGCHeader header;
  BrokenDate date;
  ReadIGCMetaData(path, header, date);
  if (header.flight == 0)
    header.flight = GetFlightNumber(flight_list, *flight);

  TCHAR name[64];
  TCHAR final_path[MAX_PATH];
  BuildIGCFileName(name, header, date);
  LocalPath(final_path, _T("logs"), name);

  // Remove a file with the same name if it exists
  if (File::Exists(final_path))
    File::Delete(final_path);

  // Rename the temporary file to the actual filename
  File::Rename(path, final_path);
}
