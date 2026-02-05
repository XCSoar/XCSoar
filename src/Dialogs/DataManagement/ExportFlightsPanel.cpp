// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/DataManagement/TargetPickerDialog.hpp"
#include "Dialogs/DataManagement/FileTransferUtil.hpp"
#include "Dialogs/DataManagement/FileMetadataHelper.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Form/Edit.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "LocalPath.hpp"
#include "io/CopyFile.hxx"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"
#include "Form/CheckBox.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/DataManagement/IgcMetaCache.hpp"
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

// Forward declaration of helper used by ExportJob
static std::string AppendFilename(std::string msg, Path filename);

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
  AllocatedPath target_device;
  unsigned total_files;
  unsigned &completed;
  unsigned &skipped;
  unsigned &failed;

  bool aborted{false};
  std::string error_message;

  ExportJob(const std::vector<Path> &files,
            Path target,
            unsigned &c, unsigned &s, unsigned &f) noexcept
    : selected_files(files),
      target_device(target),
      total_files((unsigned)files.size()),
      completed(c), skipped(s), failed(f) {}

  void Run(OperationEnvironment &env) override {
    // Try to create the export subfolder on the target device
    AllocatedPath exports_subfolder =
      BuildTargetDirectory(target_device, EXPORT_FLIGHTS_SUBFOLDER);
    bool use_subfolder = Directory::Exists(exports_subfolder);
    if (!use_subfolder) {
      try {
        Directory::Create(exports_subfolder);
        use_subfolder = true;
      } catch (...) {
        // Fall back to target root on creation failure
        use_subfolder = false;
      }
    }

    completed = 0;
    skipped = 0;
    failed = 0;

    env.SetProgressRange(total_files);

    unsigned processed = 0;
    for (const auto &p : selected_files) {
      if (env.IsCancelled())
        break;

      auto filename = p.GetBase();

      const AllocatedPath &export_dir = use_subfolder ? exports_subfolder : target_device;
      AllocatedPath dest = AllocatedPath::Build(export_dir, filename);

      StaticString<128> status_msg;
      status_msg.Format(_("Exporting: %s"), filename.c_str());
      env.SetText(status_msg);
      env.SetProgressPosition(processed);

      if (File::Exists(dest)) {
        ++skipped;
      } else {
        try {
          CopyFile(p, dest);
          ++completed;
        } catch (const std::exception &e) {
          ++failed;
          aborted = true;
          error_message = AppendFilename(e.what(), filename);
          break;
        } catch (...) {
          ++failed;
          aborted = true;
          std::string msg = _("Unknown error");
          error_message = AppendFilename(std::move(msg), filename);
          break;
        }
      }

      ++processed;
    }

    env.SetProgressPosition(total_files);
  }
};

class UploadJob final : public Job {
  const std::vector<AllocatedPath> files;
  unsigned &successful;
  unsigned &failed;

public:
  UploadJob(const std::vector<Path> &_files, 
            unsigned &_successful, unsigned &_failed) noexcept
    : files(_files.begin(), _files.end()),
      successful(_successful), failed(_failed) {}

  void Run(OperationEnvironment &env) override {
    env.SetProgressRange(files.size());
    
    successful = 0;
    failed = 0;
    
    for (unsigned i = 0; i < files.size(); ++i) {
      if (env.IsCancelled())
        break;
        
      const auto &path = files[i];
      
      StaticString<256> msg;
      msg.Format(_("Uploading %s (%u/%u)"), 
                 path.GetBase().c_str(), i + 1, (unsigned)files.size());
      env.SetText(msg);
      
      try {
        if (WeGlide::UploadIGCFile(path))
          ++successful;
        else
          ++failed;
      } catch (const std::exception &e) {
        ++failed;
        StaticString<256> error_msg;
        error_msg.Format(_("Error uploading %s: %s"), path.GetBase().c_str(), e.what());
        env.SetText(error_msg);
      } catch (...) {
        ++failed;
        StaticString<256> error_msg;
        error_msg.Format(_("Unknown error uploading %s"), path.GetBase().c_str());
        env.SetText(error_msg);
      }
      
      env.SetProgressPosition(i + 1);
    }
  }
};

static const char*
GetFileName(const FileMultiSelectWidget::FileItem &it) noexcept
{
  static std::string cached_result;
  cached_result = it.path.GetBase().c_str();
  return cached_result.c_str();
}

static const char*
GetIgcMetadata(const FileMultiSelectWidget::FileItem &it) noexcept
{
  // Cache the result in a static string to ensure the returned pointer remains valid.
  // GetCompactInfo returns an owning std::string (safe copy of text from cache entry).
  static std::string cached_result;
  cached_result = igc_cache.GetCompactInfo(it.path);
  return cached_result.c_str();
}

// Helper to append filename context to an error message
static std::string
AppendFilename(std::string msg, Path filename) {
  const char *fn = filename.c_str();
  if (fn && fn[0] != '\0') {
    msg += " (";
    msg += fn;
    msg += ')';
  }
  return msg;
}

static void
PerformExport(FileMultiSelectWidget *file_widget)
{
  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one flight."), "", MB_OK | MB_ICONINFORMATION);
    return;
  }

  const AllocatedPath &target = TargetPickerDialog::GetLastTarget();
  if (target == nullptr) {
    TargetPickerDialog dlg(UIGlobals::GetDialogLook());
    AllocatedPath chosen = dlg.ShowModal();
    if (chosen == nullptr)
      return;
    TargetPickerDialog::SetLastTarget(chosen);
  }

  unsigned completed = 0, skipped = 0, failed = 0;
  ExportJob job(selected, Path(TargetPickerDialog::GetLastTarget()), completed, skipped, failed);

  if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                 _("Export flights"), job, true)) {
    return;
  }

  StaticString<256> msg;
  msg.Format(_("Exported %u file(s)"), completed);

  if (skipped > 0)
    msg.AppendFormat(_("\nSkipped %u existing."), skipped);
  if (failed > 0)
    msg.AppendFormat(_("\nFailed %u."), failed);

  if (job.aborted) {
    const char *err = !job.error_message.empty() ? job.error_message.c_str() : "(error)";
    StaticString<256> fullmsg;
    fullmsg.append(msg.c_str());
    fullmsg.append(_("\n\nError: "));
    fullmsg.append(err);
    ShowMessageBox(fullmsg, _("Export flights"), MB_OK | MB_ICONERROR);
  } else {
    ShowMessageBox(msg, _("Export flights"), MB_OK | (failed ? MB_ICONERROR : MB_ICONINFORMATION));
  }
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

  unsigned successful = 0, failed = 0;
  UploadJob job(selected, successful, failed);

  if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                 _("WeGlide Upload"), job, true)) {
    return;
  }

  StaticString<256> msg;
  msg.Format(_("Uploaded %u file(s)"), successful);
  if (failed > 0)
    msg.AppendFormat(_("\nFailed %u."), failed);

  ShowMessageBox(msg, _("WeGlide Upload"), 
                 MB_OK | (failed > 0 ? MB_ICONWARNING : MB_ICONINFORMATION));
}

/**
 * Container widget that composes a target property field and the
 * FileMultiSelectWidget. It forwards lifecycle methods to the
 * contained list widget.
 */
struct FlightContainer : public Widget {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  UI::Notify igc_notify{[this]() {
    igc_cache.PollBackgroundFill();
    if (file_list)
      file_list->Refresh();
  }};
  std::unique_ptr<WndProperty> target_property;
  PixelRect property_rect;
  PixelRect list_rect;
  PixelRect checkbox_rect;
  AllocatedPath target_device_path;
  std::unique_ptr<CheckBoxControl> nmea_checkbox;
  bool show_nmea_files{false};
  FileMetadataHelper file_metadata;

  explicit FlightContainer(MultiFileDataField &df, FileMultiSelectWidget::TextProvider tp)
    : file_list(std::make_unique<FileMultiSelectWidget>(df, std::move(tp), _("Flights"), nullptr))
  {
    file_metadata.Build(df.GetAllPaths());
    file_list->SetFirstRightProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      return GetFileSizeText(it);
    });
    file_list->SetSecondLeftProvider(GetIgcMetadata);
  }

  const char *GetFileSizeText(const FileMultiSelectWidget::FileItem &it) const noexcept
  {
    return file_metadata.GetSizeText(it.path);
  }

  void SetTargetDevice(AllocatedPath path) noexcept
  {
    target_device_path = std::move(path);
    UpdateTargetCaption();
  }

  PixelSize GetMinimumSize() const noexcept override {
    return file_list->GetMinimumSize();
  }
  PixelSize GetMaximumSize() const noexcept override {
    return file_list->GetMaximumSize();
  }

  void Initialise([[maybe_unused]] ContainerWindow &parent,
                  [[maybe_unused]] const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    if (!target_property)
      target_property = std::make_unique<WndProperty>(look);
    if (!nmea_checkbox)
      nmea_checkbox = std::make_unique<CheckBoxControl>();
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

  void CalculateLayout(const PixelRect &rc) noexcept {
    const unsigned prop_height = Layout::GetMinimumControlHeight();
    const unsigned spacing = Layout::GetTextPadding();

    property_rect = rc;
    property_rect.bottom = property_rect.top + prop_height;

    list_rect = rc;
    list_rect.top = property_rect.bottom + spacing;
    list_rect.bottom = rc.bottom - static_cast<int>(prop_height + spacing);

    checkbox_rect = rc;
    checkbox_rect.top = list_rect.bottom + spacing;
    checkbox_rect.bottom = checkbox_rect.top + prop_height;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CalculateLayout(rc);

    const DialogLook &look = UIGlobals::GetDialogLook();
    // Create widgets with the actual parent/rect
    if (target_property)
      target_property->Create(parent, property_rect, _("Target"), 0, WindowStyle());
    else
      target_property = std::make_unique<WndProperty>(look);
    target_property->SetReadOnly(true);

    file_list->Prepare(parent, list_rect);

    if (nmea_checkbox) {
      nmea_checkbox->Create(parent, look, _("Show .nmea files"), checkbox_rect,
                           WindowStyle(), [this](bool value) {
        show_nmea_files = value;
        ApplyNmeaFilter();
        if (file_list)
          file_list->Refresh();
        StartIgcCacheFill();
      });
    } else {
      nmea_checkbox = std::make_unique<CheckBoxControl>();
      nmea_checkbox->Create(parent, look, _("Show .nmea files"), checkbox_rect,
                            WindowStyle(), [this](bool value) {
        show_nmea_files = value;
        ApplyNmeaFilter();
        if (file_list)
          file_list->Refresh();
        StartIgcCacheFill();
      });
    }
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
    if (file_list)
      file_list->Unprepare();
    target_property.reset();
    nmea_checkbox.reset();
  }

  bool Save(bool &changed) noexcept override { return file_list->Save(changed); }
  bool Click() noexcept override { return file_list->Click(); }
  void ReClick() noexcept override { file_list->ReClick(); }

  void Show([[maybe_unused]] const PixelRect &rc) noexcept override {
    if (target_property)
      target_property->MoveAndShow(property_rect);
    file_list->Show(list_rect);
    if (nmea_checkbox)
      nmea_checkbox->MoveAndShow(checkbox_rect);
  }

  bool Leave() noexcept override { return file_list->Leave(); }
  void Hide() noexcept override {
    if (target_property)
      target_property->Hide();
    file_list->Hide();
    if (nmea_checkbox)
      nmea_checkbox->Hide();
  }
  void Move(const PixelRect &rc) noexcept override {
    CalculateLayout(rc);

    if (target_property && target_property->IsDefined() && target_property->IsVisible())
      target_property->Move(property_rect);
    if (file_list)
      file_list->Move(list_rect);
    if (nmea_checkbox && nmea_checkbox->IsDefined() && nmea_checkbox->IsVisible())
      nmea_checkbox->Move(checkbox_rect);
  }

  bool SetFocus() noexcept override { return file_list->SetFocus(); }
  bool HasFocus() const noexcept override { return file_list->HasFocus(); }
  bool KeyPress(unsigned key_code) noexcept override { return file_list->KeyPress(key_code); }

  void UpdateTargetCaption() noexcept {
    if (target_property == nullptr)
      return;
    if (target_device_path == nullptr) {
      target_property->SetText(_("(none)"));
      return;
    }
    const AllocatedPath result_dir =
      BuildTargetDirectory(target_device_path, EXPORT_FLIGHTS_SUBFOLDER);
    target_property->SetText(result_dir != nullptr ? result_dir.c_str() : target_device_path.c_str());
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

  auto container = std::make_unique<FlightContainer>(*df, GetFileName);
  FlightContainer &flight_container = *container;
  FileMultiSelectWidget &file_list = *flight_container.file_list;

  {
    const AllocatedPath &last = TargetPickerDialog::GetLastTarget();
    if (last != nullptr) {
      flight_container.SetTargetDevice(Path(last));
    }
  }

  dialog.AddButton(_("Target"), [&flight_container]() {
    TargetPickerDialog dlg(UIGlobals::GetDialogLook());
    AllocatedPath chosen = dlg.ShowModal();
    if (chosen == nullptr)
      return;
    TargetPickerDialog::SetLastTarget(chosen);
    flight_container.SetTargetDevice(std::move(chosen));
  });

  dialog.AddButton(_("WeGlide Upload"), [&file_list]() { PerformWeGlideUpload(&file_list); });
  dialog.AddButton(_("Export"), [&file_list]() { PerformExport(&file_list); });
  dialog.AddButton(_("Select all"), [&file_list]() { file_list.SelectAll(); });
  dialog.AddButton(_("Select none"), [&file_list]() { file_list.ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
