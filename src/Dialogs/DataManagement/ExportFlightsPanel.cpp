// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/DataManagement/TargetPickerDialog.hpp"
#include "Dialogs/DataManagement/ExportUtil.hpp"
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
#include "util/StaticString.hxx"
#include "util/ConvertString.hpp"
#include "Formatter/ByteSizeFormatter.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cerrno>

static constexpr char EXPORT_FLIGHTS_SUBFOLDER[] = "xcsoar_flights";

// Export job that runs in background thread
struct ExportJob final : public Job {
  std::vector<Path> selected_files;
  const AllocatedPath &target_device;
  unsigned total_files;
  unsigned &completed;
  unsigned &skipped;
  unsigned &failed;

  bool aborted{false};
  std::string error_message;

  ExportJob(const std::vector<Path> &files,
            const AllocatedPath &target,
            unsigned &c, unsigned &s, unsigned &f) noexcept
    : selected_files(files),
      target_device(target),
      total_files((unsigned)files.size()),
      completed(c), skipped(s), failed(f) {}

  void Run(OperationEnvironment &env) noexcept override {
    // Try to create the export subfolder on the target device
    AllocatedPath exports_subfolder =
      BuildExportDirectory(target_device, EXPORT_FLIGHTS_SUBFOLDER);
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
          int err = errno;
          std::string msg = MapErrnoToMessage(err);
          if (msg.empty())
            msg = e.what();
          std::string fn = ToUtf8String(filename.c_str());
          if (!fn.empty()) {
            msg.reserve(msg.size() + fn.size() + 3);
            msg += " (";
            msg += fn;
            msg += ')';
          }
          error_message = std::move(msg);
          break;
        } catch (...) {
          ++failed;
          aborted = true;
          std::string msg = ToUtf8String(_("Unknown error"));
          std::string fn = ToUtf8String(filename.c_str());
          if (!fn.empty()) {
            msg.reserve(msg.size() + fn.size() + 3);
            msg += " (";
            msg += fn;
            msg += ')';
          }
          error_message = std::move(msg);
          break;
        }
      }

      ++processed;
    }

    env.SetProgressPosition(total_files);
  }
};

static const char*
GetFileName(const FileMultiSelectWidget::FileItem &it) noexcept
{
  return it.path.GetBase().c_str();
}

static const char*
GetFileSizeText(const FileMultiSelectWidget::FileItem &it) noexcept
{
  static StaticString<32> buf;
  const uint64_t sz = File::GetSize(it.path);
  FormatByteSize(buf.data(), buf.capacity(), sz, false);
  return buf.c_str();
}

static const char*
GetIgcMetadata(const FileMultiSelectWidget::FileItem &it) noexcept
{
  static IgcMetaCache igc_cache;
  return igc_cache.GetCompactInfo(it.path);
}

static void
PerformExport(FileMultiSelectWidget *file_widget) noexcept
{
  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one flight."), "", MB_OK | MB_ICONINFORMATION);
    return;
  }

  const AllocatedPath &target = TargetPickerDialog::GetLastTarget();
  if (target == nullptr) {
    TargetPickerDialog dlg(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook());
    AllocatedPath chosen = dlg.ShowModal();
    if (chosen == nullptr)
      return;
    TargetPickerDialog::SetLastTarget(chosen);
  }

  unsigned completed = 0, skipped = 0, failed = 0;
  ExportJob job(selected, TargetPickerDialog::GetLastTarget(), completed, skipped, failed);

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
    UTF8ToWideConverter conv(job.error_message.c_str());
    const char *err = conv.IsValid() ? conv.c_str() : "(error)";
    StaticString<256> fullmsg;
    fullmsg.append(msg.c_str());
    fullmsg.append(_("\n\nError: "));
    fullmsg.append(err);
    ShowMessageBox(fullmsg, _("Export flights"), MB_OK | MB_ICONERROR);
  } else {
    ShowMessageBox(msg, _("Export flights"), MB_OK | (failed ? MB_ICONERROR : MB_ICONINFORMATION));
  }
}

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

  // Ensure a sensible default export folder
  if (TargetPickerDialog::GetLastTarget() == nullptr) {
    AllocatedPath default_target = TargetPickerDialog::GetDefaultTarget();
    if (default_target != nullptr)
      TargetPickerDialog::SetLastTarget(default_target);
  }

  /**
   * Container widget that composes a target property field and the
   * FileMultiSelectWidget. It forwards lifecycle methods to the
   * contained list widget.
   */
  struct FlightContainer : public Widget {
    std::unique_ptr<FileMultiSelectWidget> file_list;
    std::unique_ptr<WndProperty> target_property;
    PixelRect property_rect;
    PixelRect list_rect;
    PixelRect checkbox_rect;
    AllocatedPath target_device_path;
    std::unique_ptr<CheckBoxControl> nmea_checkbox;
    bool show_nmea_files{false};

    explicit FlightContainer(MultiFileDataField &df, FileMultiSelectWidget::TextProvider tp)
      : file_list(std::make_unique<FileMultiSelectWidget>(df, std::move(tp), "Flights", nullptr)) {}

    PixelSize GetMinimumSize() const noexcept override {
      return file_list->GetMinimumSize();
    }
    PixelSize GetMaximumSize() const noexcept override {
      return file_list->GetMaximumSize();
    }

    void Initialise([[maybe_unused]] ContainerWindow &parent, 
                    [[maybe_unused]] const PixelRect &rc) noexcept override {}

    void ApplyNmeaFilter() noexcept
    {
      if (show_nmea_files)
        file_list->SetFilter(std::function<bool(const Path&)>());
      else
        file_list->SetFilter([](const Path &p){
          return !p.EndsWithIgnoreCase(".nmea");
        });
    }

    void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
      const unsigned prop_height = Layout::GetMinimumControlHeight();
      const unsigned spacing = Layout::GetTextPadding();

      property_rect = rc;
      property_rect.bottom = property_rect.top + prop_height;

      list_rect = rc;
      list_rect.top = property_rect.bottom + spacing;
      list_rect.bottom = rc.bottom - (int)(prop_height + spacing);

      const DialogLook &look = UIGlobals::GetDialogLook();
      target_property = std::make_unique<WndProperty>(look);
      target_property->Create(parent, property_rect, _("Target"), 0, WindowStyle());
      target_property->SetReadOnly(true);

      file_list->Prepare(parent, list_rect);

      checkbox_rect = rc;
      checkbox_rect.top = list_rect.bottom + spacing;
      checkbox_rect.bottom = checkbox_rect.top + prop_height;

      nmea_checkbox = std::make_unique<CheckBoxControl>();
      nmea_checkbox->Create(parent, look, _("Show .nmea files"), checkbox_rect,
                           WindowStyle(), [this](bool value) {
        show_nmea_files = value;
        ApplyNmeaFilter();
        if (file_list)
          file_list->Refresh();
      });
      nmea_checkbox->SetState(show_nmea_files);

      ApplyNmeaFilter();
      UpdateTargetCaption();
      file_list->Refresh();
      file_list->ClearSelection();
    }

    void Unprepare() noexcept override {
      if (file_list)
        file_list->Unprepare();
      target_property.reset();
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
      const unsigned prop_height = Layout::GetMinimumControlHeight();
      const unsigned spacing = Layout::GetTextPadding();

      property_rect = rc;
      property_rect.bottom = property_rect.top + prop_height;

      list_rect = rc;
      list_rect.top = property_rect.bottom + spacing;
      list_rect.bottom = rc.bottom - (int)(prop_height + spacing);

      checkbox_rect = rc;
      checkbox_rect.top = list_rect.bottom + spacing;
      checkbox_rect.bottom = checkbox_rect.top + prop_height;

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
        BuildExportDirectory(target_device_path, EXPORT_FLIGHTS_SUBFOLDER);
      target_property->SetText(result_dir != nullptr ? result_dir.c_str() : target_device_path.c_str());
    }
  };

  auto container = std::make_unique<FlightContainer>(*df, GetFileName);
  FlightContainer *container_ptr = container.get();
  FileMultiSelectWidget *file_widget = container_ptr->file_list.get();

  file_widget->SetFirstRightProvider(GetFileSizeText);
  file_widget->SetSecondLeftProvider(GetIgcMetadata);

  {
    const AllocatedPath &last = TargetPickerDialog::GetLastTarget();
    if (last != nullptr) {
      container_ptr->target_device_path = AllocatedPath(last.c_str());
      container_ptr->UpdateTargetCaption();
    }
  }

  dialog.AddButton(_("Target"), [container_ptr]() {
    TargetPickerDialog dlg(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook());
    AllocatedPath chosen = dlg.ShowModal();
    if (chosen == nullptr)
      return;
    container_ptr->target_device_path = std::move(chosen);
    container_ptr->UpdateTargetCaption();
  });

  dialog.AddButton(_("Export"), [file_widget]() { PerformExport(file_widget); });
  dialog.AddButton(_("Select all"), [file_widget]() { file_widget->SelectAll(); });
  dialog.AddButton(_("Select none"), [file_widget]() { file_widget->ClearSelection(); });
  dialog.AddButton(_("Back"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}
