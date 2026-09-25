// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/ExternalLogger.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "Device/Config.hpp"
#include "Device/Descriptor.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/RecordedFlight.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "UIGlobals.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Job/TriStateJob.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileTransaction.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCHeader.hpp"
#include "Formatter/IGCFilenameFormatter.hpp"
#include "time/BrokenDate.hpp"
#include "Interface.hpp"
#include "net/client/WeGlide/UploadIGCFile.hpp"

#ifdef HAVE_HTTP
#include "Dialogs/CoFunctionDialog.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Operation/ProgressListener.hpp"
#include "co/InvokeTask.hxx"
#include "net/client/FlarmHub/Client.hpp"
#include "net/http/Init.hpp"
#endif

#include <optional>


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
{
  TriStateJob<DeclareJob> job(device, declaration, home);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            "", job, true);
  return job.GetResult();
}

static bool
DeviceDeclare(DeviceDescriptor &dev, const Declaration &declaration,
              const Waypoint *home)
try {
  if (dev.IsOccupied())
    return false;

  if (ShowMessageBox(_("Declare task?"), dev.GetDisplayName(),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  if (!dev.Borrow())
    return false;

  MessageOperationEnvironment env;
  const ScopeReturnDevice return_device{dev, env};

  const char *caption = dev.GetDisplayName();
  if (caption == nullptr)
    caption = _("Declare task");

  auto result = DoDeviceDeclare(dev, declaration, home);

  switch (result) {
  case TriStateJobResult::SUCCESS:
    ShowMessageBox(_("Task declared!"),
                   caption, MB_OK | MB_ICONINFORMATION);
    return true;

  case TriStateJobResult::ERROR:
    ShowMessageBox(_("Error occurred,\nTask NOT declared!"),
                   caption, MB_OK | MB_ICONERROR);
    return false;

  case TriStateJobResult::CANCELLED:
    return false;
  }

  gcc_unreachable();
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(_("Error occurred,\nTask NOT declared!"),
            std::current_exception(),
            dev.GetDisplayName());
  return false;
}

void
ExternalLogger::Declare(const Declaration &decl, const Waypoint *home)
{
  bool found_logger = false;

  for (DeviceDescriptor *i : *backend_components->devices) {
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

/**
 * Determine the host which may serve the FLARM Hub REST API for this
 * device.  Newer PowerFLARM devices do not implement the binary
 * protocol and hand out their flights only over HTTP.
 *
 * @return the host or nullptr
 */
[[gnu::pure]]
static const char *
GetFlarmHubHost([[maybe_unused]] const DeviceConfig &config) noexcept
{
#ifdef HAVE_HTTP
  if (config.port_type == DeviceConfig::PortType::TCP_CLIENT &&
      config.IsDriver("FLARM") && !config.ip_address.empty())
    return config.ip_address.c_str();
#endif

  return nullptr;
}

/**
 * Closes the port of a borrowed device and schedules reopening it
 * when the caller leaves the current scope.
 */
class ScopeCloseBorrowedDevice {
  DeviceDescriptor &device;

public:
  explicit ScopeCloseBorrowedDevice(DeviceDescriptor &_device) noexcept
    :device(_device) {
    device.CloseBorrowed();
  }

  ~ScopeCloseBorrowedDevice() noexcept {
    device.ScheduleReopenBorrowed();
  }

  ScopeCloseBorrowedDevice(const ScopeCloseBorrowedDevice &) = delete;

  ScopeCloseBorrowedDevice &
  operator=(const ScopeCloseBorrowedDevice &) = delete;
};

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
{
  TriStateJob<ReadFlightListJob> job(device, flight_list);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            "", job, true);
  return job.GetResult();
}

/**
 * Read the list of flights, preferring the FLARM Hub REST API over
 * the FLARM binary protocol.
 *
 * @param flarm_hub_host is cleared if this host has no Hub REST API
 */
static TriStateJobResult
ReadFlightList(DeviceDescriptor &device, RecordedFlightList &flight_list,
               [[maybe_unused]] const char *&flarm_hub_host)
{
#ifdef HAVE_HTTP
  if (flarm_hub_host != nullptr) {
    PluggableOperationEnvironment env;
    const auto available =
      ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                           UIGlobals::GetDialogLook(),
                           _("Download flight"),
                           FlarmHub::CoReadFlightList(*Net::curl,
                                                      flarm_hub_host,
                                                      flight_list, env),
                           &env);
    if (!available)
      return TriStateJobResult::CANCELLED;

    if (*available)
      return TriStateJobResult::SUCCESS;

    flarm_hub_host = nullptr;
  }
#endif

  return DoReadFlightList(device, flight_list);
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
{
  TriStateJob<DownloadFlightJob> job(device, flight, path);
  JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
            "", job, true);
  return job.GetResult();
}

#ifdef HAVE_HTTP

static Co::InvokeTask
DownloadFlarmHubFlight(const char *host, unsigned index, Path path,
                       ProgressListener &progress)
{
  co_await FlarmHub::CoDownloadFlight(*Net::curl, host, index, path,
                                      progress);
}

#endif

static TriStateJobResult
DownloadFlight(DeviceDescriptor &device, const RecordedFlightInfo &flight,
               Path path, [[maybe_unused]] const char *flarm_hub_host)
{
#ifdef HAVE_HTTP
  if (flarm_hub_host != nullptr) {
    PluggableOperationEnvironment env;
    return ShowCoDialog(UIGlobals::GetMainWindow(),
                        UIGlobals::GetDialogLook(),
                        _("Download flight"),
                        DownloadFlarmHubFlight(flarm_hub_host,
                                               flight.internal.flarm_hub,
                                               path, env),
                        &env)
      ? TriStateJobResult::SUCCESS
      : TriStateJobResult::CANCELLED;
  }
#endif

  return DoDownloadFlight(device, flight, path);
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
        flight.start_time > _flight.start_time)
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
    if (flight.date.IsPlausible())
      buffer.UnsafeFormat("%04u/%02u/%02u %02u:%02u-%02u:%02u",
                          flight.date.year, flight.date.month, flight.date.day,
                          flight.start_time.hour, flight.start_time.minute,
                          flight.end_time.hour, flight.end_time.minute);
    else
      buffer.UnsafeFormat("----/--/-- %02u:%02u-%02u:%02u",
                          flight.start_time.hour, flight.start_time.minute,
                          flight.end_time.hour, flight.end_time.minute);

    combo.Append(i, buffer);
  }

  // Show list of the flights
  int i = ComboPicker("Choose a flight",
                      combo, nullptr, false);

  return i < 0 ? nullptr : &flight_list[i];
}

void
ExternalLogger::DownloadFlightFrom(DeviceDescriptor &device)
{
  class ScopeEnableSecondDeviceNMEA {
    DeviceDescriptor &device;
    OperationEnvironment &env;

  public:
    ScopeEnableSecondDeviceNMEA(DeviceDescriptor &_device,
                                OperationEnvironment &_env) noexcept
      :device(_device), env(_env) {}

    ~ScopeEnableSecondDeviceNMEA() noexcept {
      (void)device.EnableSecondDeviceNMEA(env);
    }
  };

  MessageOperationEnvironment env;
  std::optional<ScopeEnableSecondDeviceNMEA> enable_second_device_nmea;
  enable_second_device_nmea.emplace(device, env);

  // Download the list of flights that the logger contains
  RecordedFlightList flight_list;

  /* the host serving the FLARM Hub REST API, or nullptr if the
     flights are read with the FLARM binary protocol */
  const char *flarm_hub_host = GetFlarmHubHost(device.GetConfig());

  try {
    switch (ReadFlightList(device, flight_list, flarm_hub_host)) {
    case TriStateJobResult::SUCCESS:
      break;

    case TriStateJobResult::ERROR:
      ShowMessageBox(_("Failed to download flight list."),
                     _("Download flight"), MB_OK | MB_ICONERROR);
      return;

    case TriStateJobResult::CANCELLED:
      return;
    }
  } catch (OperationCancelled) {
    return;
  } catch (...) {
    ShowError(_("Failed to download flight list."),
              std::current_exception(),
              _("Download flight"));
    return;
  }

  // The logger seems to be empty -> cancel
  if (flight_list.empty()) {
    ShowMessageBox(_("Logger is empty."),
                _("Download flight"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const auto logs_path = LocalPath(GetFileTypeDefaultDir(FileType::IGC));
  Directory::CreateRecursive(logs_path);

  /* the FLARM Hub cannot obtain its own connection to the FLARM while
     we occupy the NMEA port, and answers with HTTP status 500 */
  std::optional<ScopeCloseBorrowedDevice> close_device;
  if (flarm_hub_host != nullptr) {
    /* the device is not used at all, and it must not be talked to
       after its port has been closed */
    enable_second_device_nmea.reset();
    close_device.emplace(device);
  }

  while (true) {
    // Show list of the flights
    const RecordedFlightInfo *flight = ShowFlightList(flight_list);
    if (!flight)
      break;

    // Download chosen IGC file into temporary file
    FileTransaction transaction(AllocatedPath::Build(logs_path,
                                                     "temp.igc"));

    try {
      switch (DownloadFlight(device, *flight,
                             transaction.GetTemporaryPath(),
                             flarm_hub_host)) {
      case TriStateJobResult::SUCCESS:
        break;

      case TriStateJobResult::ERROR:
        ShowMessageBox(_("Failed to download flight."),
                       _("Download flight"), MB_OK | MB_ICONERROR);
        return;

      case TriStateJobResult::CANCELLED:
        continue;
      }
    } catch (OperationCancelled) {
      continue;
    } catch (...) {
      ShowError(_("Failed to download flight."),
                std::current_exception(),
                _("Download flight"));
      return;
    }

    /* read the IGC header and build the final IGC file name with it */

    IGCHeader header;
    BrokenDate date;
    ReadIGCMetaData(transaction.GetTemporaryPath(), header, date);
    if (header.flight == 0)
      header.flight = GetFlightNumber(flight_list, *flight);

    char name[64];
    FormatIGCFilenameLong(name, date, header.manufacturer, header.id,
                          header.flight);

    const auto igc_path = AllocatedPath::Build(logs_path, name);
    transaction.SetPath((Path)igc_path);
    
    try {
      transaction.Commit();
    } catch (...) {
      ShowError(std::current_exception(), _("Download flight"));
    }

    WeGlideSettings weglide_settings =
      CommonInterface::GetComputerSettings().weglide;
    if (weglide_settings.enabled && weglide_settings.automatic_upload &&
      weglide_settings.pilot_id > 0) {
      // ask whether this IGC should be uploaded to WeGlide
      if (ShowMessageBox(_("Do you want to upload this flight to WeGlide?"),
        _("Upload Flight"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
        WeGlide::UploadIGCFile(igc_path);
      }
    }

    if (ShowMessageBox(_("Do you want to download another flight?"),
                    _("Download flight"), MB_YESNO | MB_ICONQUESTION) != IDYES)
      break;
  }
}
