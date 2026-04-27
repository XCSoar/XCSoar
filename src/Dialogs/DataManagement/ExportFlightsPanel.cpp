// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "StorageLocationPickerDialog.hpp"
#include "Dialogs/DataManagement/FileTransferUtil.hpp"
#include "Formatter/FileMetadataFormatter.hpp"
#include "Storage/StorageUtil.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Widget/PropertyWidgetContainer.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "LocalPath.hpp"
#include "Storage/StorageDevice.hpp"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Form/CheckBox.hpp"
#include "Screen/Layout.hpp"
#include "IGC/IgcMetaCache.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"
#include "net/client/WeGlide/UploadIGCFile.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "Interface.hpp"
#include "util/StaticString.hxx"
#include "ui/event/Notify.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cerrno>
#include <cstring>

static constexpr char EXPORT_FLIGHTS_SUBFOLDER[] = "xcsoar_flights";

static IgcMetaCache igc_cache;

/**
 * Check if WeGlide is properly configured for uploads.
 * Requires pilot ID and birthdate to be set.
 */
static bool
IsWeGlideConfigured() noexcept
{
  const WeGlideSettings &settings = CommonInterface::GetComputerSettings().weglide;
  return settings.enabled && 
         settings.pilot_id != 0 && 
         settings.pilot_birthdate.IsPlausible();
}

// Export job that runs in background thread
struct ExportJob final : public Job {
  std::vector<Path> selected_files;
  std::shared_ptr<StorageDevice> device;
  unsigned total_files;
  unsigned &completed;
  unsigned &skipped;
  unsigned &failed;
  std::vector<std::string> failed_names;

  ExportJob(const std::vector<Path> &files,
            std::shared_ptr<StorageDevice> dev,
            unsigned &c, unsigned &s, unsigned &f) noexcept
    : selected_files(files),
      device(std::move(dev)),
      total_files((unsigned)files.size()),
      completed(c), skipped(s), failed(f) {}

  void Run(OperationEnvironment &env) override {
    completed = 0;
    skipped = 0;
    failed = 0;

    env.SetProgressRange(total_files);

    unsigned processed = 0;
    for (const auto &p : selected_files) {
      if (env.IsCancelled())
        break;

      {
        const std::string dev_name = device->Name();
        const Path dev_path{dev_name.c_str()};
        if (!IsContentUri(dev_path) && !Directory::Exists(dev_path)) {
          failed += total_files - processed;
          break;
        }
      }

      auto filename = p.GetBase();

      /* Build device-relative destination path.  For SAF devices the
         subdirectory is created automatically by resolveOrCreateDocument;
         for local filesystems LocalFilesystemDevice prepends the root. */
      std::string rel = EXPORT_FLIGHTS_SUBFOLDER;
      rel.push_back('/');
      rel.append(filename.c_str());
      AllocatedPath remote_path{rel.c_str()};

      StaticString<128> status_msg;
      status_msg.Format(_("Exporting: %s"), filename.c_str());
      env.SetText(status_msg);
      env.SetProgressPosition(processed);

      try {
        device->CopyFromLocal(p, remote_path, nullptr);
        ++completed;
      } catch (...) {
        ++failed;
        if (failed_names.size() < 5)
          failed_names.emplace_back(filename.c_str());
      }

      ++processed;
    }

    env.SetProgressPosition(total_files);
  }
};

static const char*
GetIgcMetadata(const FileMultiSelectWidget::FileItem &it) noexcept
{
  return igc_cache.GetCompactInfoPtr(it.path);
}

static void
PerformExport(FileMultiSelectWidget *file_widget)
{
  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one flight."), "", MB_OK | MB_ICONINFORMATION);
    return;
  }

  AllocatedPath target{Path{GetLastStorageTarget()}};
  if (target == nullptr) {
    target = PickStorageLocation();
    if (target == nullptr)
      return;
  }

  auto device = FindDeviceByName(target);
  if (!device) {
    ShowMessageBox(_("Target device not found."),
                   _("Export flights"), MB_OK | MB_ICONERROR);
    return;
  }

  unsigned completed = 0, skipped = 0, failed = 0;
  ExportJob job(selected, std::move(device), completed, skipped, failed);

  if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                 _("Export flights"), job, true)) {
    return;
  }

  StaticString<256> msg;
  msg.Format(_("Exported %u file(s)"), completed);

  if (skipped > 0)
    msg.AppendFormat(_("\nSkipped %u existing."), skipped);
  if (failed > 0) {
    msg.AppendFormat(_("\nFailed %u."), failed);
    for (const auto &name : job.failed_names)
      msg.AppendFormat("\n- %s", name.c_str());
  }

  ShowMessageBox(msg, _("Export flights"),
                 MB_OK | (failed ? MB_ICONERROR : MB_ICONINFORMATION));
}

static void
PerformWeGlideUpload(FileMultiSelectWidget *file_widget)
{
  if (!IsWeGlideConfigured()) {
    ShowMessageBox(_("WeGlide is not configured. Please set your pilot ID and birthdate in the settings."),
                   _("WeGlide Upload"), MB_OK | MB_ICONERROR);
    return;
  }

  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one flight."), "", MB_OK | MB_ICONINFORMATION);
    return;
  }

  // Validate that only .igc files are selected
  for (const auto &path : selected) {
    if (!path.EndsWithIgnoreCase(".igc")) {
      ShowMessageBox(_("Only .igc files can be uploaded to WeGlide."), 
                     _("WeGlide Upload"), MB_OK | MB_ICONERROR);
      return;
    }
  }

  /* Call UploadIGCFile() directly on the UI thread.  Each call shows
     its own modal progress dialog (ShowCoFunctionDialog) with cancel
     support.  Running this via JobDialog would crash because
     UploadIGCFile() performs UI operations internally. */
  unsigned successful = 0, failed = 0;
  for (const auto &path : selected) {
    if (WeGlide::UploadIGCFile(path))
      ++successful;
    else
      ++failed;
  }

  StaticString<256> msg;
  msg.Format(_("Uploaded %u file(s)"), successful);
  if (failed > 0)
    msg.AppendFormat(_("\nFailed %u."), failed);

  ShowMessageBox(msg, _("WeGlide Upload"), 
                 MB_OK | (failed > 0 ? MB_ICONWARNING : MB_ICONINFORMATION));
}

struct FlightContainer : public PropertyWidgetContainer {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  UI::Notify igc_notify{[this]() {
    igc_cache.PollBackgroundFill();
    if (file_list)
      file_list->Refresh();
  }};
  PixelRect checkbox_rect;
  AllocatedPath target_device_path;
  std::unique_ptr<CheckBoxControl> nmea_checkbox;
  bool show_nmea_files{false};
  FileMetadataFormatter file_metadata;

  explicit FlightContainer(MultiFileDataField &df)
    : PropertyWidgetContainer(_("Target")),
      file_list(std::make_unique<FileMultiSelectWidget>(df, nullptr, _("Flights"), nullptr))
  {
    file_metadata.Build(df.GetAllPaths());
    file_list->SetSecondRightProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      return GetFileSizeText(it);
    });
    file_list->SetSecondLeftProvider(GetIgcMetadata);
  }

  Widget &GetContentWidget() noexcept override { return *file_list; }
  const Widget &GetContentWidget() const noexcept override { return *file_list; }

  const char *GetFileSizeText(const FileMultiSelectWidget::FileItem &it) const noexcept
  {
    return file_metadata.GetSizeText(it.path);
  }

  void SetTargetDevice(AllocatedPath path) noexcept
  {
    target_device_path = std::move(path);
    UpdateTargetCaption();
  }

  void ApplyNmeaFilter() noexcept
  {
    if (show_nmea_files)
      file_list->SetFilter(std::function<bool(const Path&)>());
    else
      file_list->SetFilter([](const Path &p){
        return !p.EndsWithIgnoreCase(".nmea");
      });
  }

  void StartIgcCacheFill() noexcept
  {
    std::vector<AllocatedPath> paths;
    const auto all_paths = file_list->GetAllPaths();
    paths.reserve(all_paths.size());
    for (const auto &path : all_paths)
      paths.emplace_back(path);

    igc_cache.StartBackgroundFill(std::move(paths), &igc_notify);
  }

  void CalculateLayout(const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::CalculateLayout(rc);
    const unsigned prop_height = Layout::GetMinimumControlHeight();
    const unsigned spacing = Layout::GetTextPadding();
    content_rect.bottom = rc.bottom - static_cast<int>(prop_height + spacing);
    checkbox_rect = rc;
    checkbox_rect.top = content_rect.bottom + spacing;
    checkbox_rect.bottom = checkbox_rect.top + prop_height;
  }

  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Initialise(parent, rc);
    if (!nmea_checkbox)
      nmea_checkbox = std::make_unique<CheckBoxControl>();
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Prepare(parent, rc);

    const DialogLook &look = UIGlobals::GetDialogLook();
    if (!nmea_checkbox)
      nmea_checkbox = std::make_unique<CheckBoxControl>();
    WindowStyle style;
    style.TabStop();
    nmea_checkbox->Create(parent, look, _("Show .nmea files"), checkbox_rect,
                         style, [this](bool value) {
      show_nmea_files = value;
      ApplyNmeaFilter();
      if (file_list)
        file_list->Refresh();
      StartIgcCacheFill();
    });
    nmea_checkbox->SetState(show_nmea_files);

    ApplyNmeaFilter();
    UpdateTargetCaption();
    file_list->Refresh();
    file_list->ClearSelection();
    StartIgcCacheFill();
  }

  void Unprepare() noexcept override {
    igc_cache.CancelBackgroundFill();
    igc_notify.ClearNotification();
    PropertyWidgetContainer::Unprepare();
    nmea_checkbox.reset();
  }

  void Show(const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Show(rc);
    if (nmea_checkbox)
      nmea_checkbox->MoveAndShow(checkbox_rect);
  }

  void Hide() noexcept override {
    PropertyWidgetContainer::Hide();
    if (nmea_checkbox)
      nmea_checkbox->Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Move(rc);
    if (nmea_checkbox && nmea_checkbox->IsDefined() && nmea_checkbox->IsVisible())
      nmea_checkbox->Move(checkbox_rect);
  }

  void UpdateTargetCaption() noexcept {
    if (target_device_path == nullptr) {
      UpdatePropertyText(_("(none)"));
      return;
    }
    std::string caption = FormatStorageCaption(target_device_path);
    if (EXPORT_FLIGHTS_SUBFOLDER[0] != '\0') {
      caption.push_back('/');
      caption.append(EXPORT_FLIGHTS_SUBFOLDER);
    }
    UpdatePropertyText(caption.c_str());
  }
};

void
ShowExportFlightsDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Export flights"));

  /**
   * Prepare MultiFileDataField with available log files from the XCSoar
   * logs folder only (collect, sort externally, then populate)
   */
  auto df = std::make_unique<MultiFileDataField>();
  auto logs_path = MakeLocalPath("logs");
  if (logs_path != nullptr && Directory::Exists(logs_path)) {
    ScanFilesIntoDataField(logs_path, *df,
                           {"*.igc", "*.nmea"}, true);
  }

  auto container = std::make_unique<FlightContainer>(*df);
  FlightContainer &flight_container = *container;
  FileMultiSelectWidget &file_list = *flight_container.file_list;

  {
    const AllocatedPath &last = GetLastStorageTarget();
    if (last != nullptr) {
      flight_container.SetTargetDevice(Path(last));
    }
  }

  dialog.AddButton(_("Choose location"), [&flight_container]() {
    PickStorageLocationAndApply([&flight_container](AllocatedPath chosen) {
      flight_container.SetTargetDevice(std::move(chosen));
    });
  });

  dialog.AddButton(_("WeGlide Upload"), [&file_list]() { PerformWeGlideUpload(&file_list); });
  dialog.AddButton(_("Export"), [&file_list]() { PerformExport(&file_list); });
  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAll(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
