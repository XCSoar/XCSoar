// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "StorageLocationPickerDialog.hpp"
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
#include "Language/FormatText.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Frame.hpp"
#include "Screen/Layout.hpp"
#include "IGC/IgcMetaCache.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"
#include "util/StaticString.hxx"
#include "net/client/WeGlide/UploadIGCFile.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "Interface.hpp"
#include "net/http/Features.hpp"
#ifdef HAVE_HTTP
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "io/FileMapping.hpp"
#include "net/client/SoaringScoring/Account.hpp"
#include "net/client/SoaringScoring/Client.hpp"
#include "net/http/Init.hpp"
#include "ui/canvas/Canvas.hpp"
#include <span>
#endif
#include "ui/event/Notify.hpp"
#include "ui/event/DelayedNotify.hpp"
#include "LogFile.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "java/Global.hxx"
#endif

#include <chrono>
#include <memory>
#include <string>
#include <vector>

static constexpr char EXPORT_FLIGHTS_SUBFOLDER[] = "xcsoar_flights";

static IgcMetaCache igc_cache;

void
ShutdownExportFlightsPanel() noexcept
{
  igc_cache.Shutdown();
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

static std::unique_ptr<MultiFileDataField>
ScanLogs(bool igc_only)
{
  auto df = std::make_unique<MultiFileDataField>();
  auto logs_path =
    MakeLocalPath(GetFileTypeDefaultDir(FileType::IGC).c_str());
  if (logs_path == nullptr || !Directory::Exists(logs_path))
    return df;

  if (igc_only)
    df->Scan(logs_path, {FileType::IGC}, true);
  else
    df->Scan(logs_path, {FileType::IGC, FileType::NMEA}, true);
  return df;
}

static void
ShowNeedFlightSelection() noexcept
{
  ShowMessageBox(_("Select at least one flight."), "",
                 MB_OK | MB_ICONINFORMATION);
}

static void
UseHighlightedFlight(FileMultiSelectWidget &files) noexcept
{
  if (!files.ActivateCursor())
    ShowNeedFlightSelection();
}
static const char *
GetIgcMetadata(const FileMultiSelectWidget::FileItem &it) noexcept
{
  return igc_cache.GetCompactInfoPtr(it.path);
}

static void
PerformExport(FileMultiSelectWidget *file_widget)
{
  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowNeedFlightSelection();
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
    StaticString<64> message;
    FormatDeviceNotFound(message, N_("Target"));
    ShowMessageBox(message,
                   C_("Menu", "Export flights"), MB_OK | MB_ICONERROR);
    return;
  }

  unsigned completed = 0, skipped = 0, failed = 0;
  ExportJob job(selected, std::move(device), completed, skipped, failed);

  if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                 C_("Menu", "Export flights"), job, true)) {
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

  ShowMessageBox(msg, C_("Menu", "Export flights"),
                 MB_OK | (failed ? MB_ICONERROR : MB_ICONINFORMATION));
}

#ifdef HAVE_HTTP

static int
PickSoaringScoringEntry(const std::vector<SoaringScoring::Entry> &entries)
{
  if (entries.size() == 1)
    return 0;

  TextRowRenderer row_renderer;
  row_renderer.CalculateLayout(*UIGlobals::GetDialogLook().list.font);

  class EntryRenderer final : public ListItemRenderer {
    const std::vector<SoaringScoring::Entry> &entries;
    TextRowRenderer &row_renderer;

  public:
    EntryRenderer(const std::vector<SoaringScoring::Entry> &_entries,
                  TextRowRenderer &_row_renderer) noexcept
      :entries(_entries), row_renderer(_row_renderer) {}

    void OnPaintItem(Canvas &canvas, const PixelRect rc,
                     unsigned idx) noexcept override {
      const char *name = entries[idx].contest_name.c_str();
      if (name[0] == '\0')
        name = entries[idx].class_name.c_str();
      row_renderer.DrawTextRow(canvas, rc, name);
    }
  } renderer{entries, row_renderer};

  return ListPicker(_("SoaringScoring"), entries.size(), 0,
                    Layout::GetMinimumControlHeight(), renderer);
}

static void
PerformSoaringScoringUpload(FileMultiSelectWidget *file_widget)
{
  if (!SoaringScoring::AccountConnected() || Net::curl == nullptr) {
    ShowMessageBox(_("Sign in from Services, SoaringScoring."),
                   _("SoaringScoring"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowMessageBox(_("Select at least one flight."), "",
                   MB_OK | MB_ICONINFORMATION);
    return;
  }

  for (const auto &path : selected) {
    if (!path.EndsWithIgnoreCase(".igc")) {
      ShowMessageBox(_("Only .igc files can be uploaded."),
                     _("SoaringScoring"), MB_OK | MB_ICONERROR);
      return;
    }
  }

  const auto token = SoaringScoring::LoadAccountToken().access_token;
  PluggableOperationEnvironment env;
  std::optional<std::vector<SoaringScoring::Entry>> entries;
  try {
    entries = ShowCoFunctionDialog(
      UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
      _("Download"),
      SoaringScoring::LoadEntries(*Net::curl, token, env), &env);
  } catch (...) {
    ShowError(std::current_exception(), _("SoaringScoring"));
    return;
  }
  if (!entries)
    return;
  if (entries->empty()) {
    ShowMessageBox(_("No current contest entry."), _("SoaringScoring"),
                   MB_OK | MB_ICONINFORMATION);
    return;
  }

  const int which = PickSoaringScoringEntry(*entries);
  if (which < 0)
    return;
  const auto &entry = (*entries)[which];

  std::string problems;
  for (const auto &path : selected) {
    try {
      FileMapping mapping{path};
      const std::span<const std::byte> bytes = mapping;
      std::string igc{reinterpret_cast<const char *>(bytes.data()),
                      bytes.size()};
      const auto base = path.GetBase();
      auto report = ShowCoFunctionDialog(
        UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
        _("SoaringScoring"),
        SoaringScoring::UploadIgc(*Net::curl, token, entry.upload_url,
                                  base.c_str(), std::move(igc), env),
        &env);
      if (!report)
        return;
      if (!report->validation_ok) {
        if (!problems.empty())
          problems.push_back('\n');
        problems += base.c_str();
        for (const auto &issue : report->issues) {
          problems.push_back('\n');
          problems += issue;
        }
      }
    } catch (...) {
      ShowError(std::current_exception(), _("SoaringScoring"));
      return;
    }
  }

  if (problems.empty())
    ShowMessageBox(_("OK"), _("SoaringScoring"),
                   MB_OK | MB_ICONINFORMATION);
  else
    ShowMessageBox(problems.c_str(), _("SoaringScoring"),
                   MB_OK | MB_ICONEXCLAMATION);
}

#endif

static bool
EnsureWeGlideConfigured() noexcept
{
  if (CommonInterface::GetComputerSettings().weglide.IsConfigured())
    return true;

  ShowMessageBox(_("WeGlide is not configured. Please set your pilot ID and birthdate in the settings."),
                 _("WeGlide Upload"), MB_OK | MB_ICONERROR);
  return false;
}

static void
UploadOneIGCFile(Path path) noexcept
{
  if (!EnsureWeGlideConfigured())
    return;

  if (path == nullptr || path.empty())
    return;

  if (!path.EndsWithIgnoreCase(".igc")) {
    ShowMessageBox(_("Only .igc files can be uploaded to WeGlide."),
                   _("WeGlide Upload"), MB_OK | MB_ICONERROR);
    return;
  }

  /* UploadIGCFile() shows its own progress dialog and result. */
  WeGlide::UploadIGCFile(path);
}

static void
PerformWeGlideUpload(FileMultiSelectWidget *file_widget)
{
  if (!EnsureWeGlideConfigured())
    return;

  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowNeedFlightSelection();
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

#ifdef ANDROID
static void
PerformShare(FileMultiSelectWidget *file_widget)
{
  const auto selected = file_widget->GetSelectedPaths();
  if (selected.empty()) {
    ShowNeedFlightSelection();
    return;
  }

  if (native_view == nullptr) {
    ShowMessageBox(_("Sharing is not available."), C_("Button", "Share"),
                   MB_OK | MB_ICONERROR);
    return;
  }

  native_view->ShareFiles(Java::GetEnv(), selected);
}
#endif

struct FlightContainer : public PropertyWidgetContainer {
  std::unique_ptr<FileMultiSelectWidget> file_list;
  UI::DelayedNotify igc_progress_notify{std::chrono::milliseconds{100},
                                        [this]() { RefreshIgcRows(); }};
  UI::Notify igc_completion_notify{[this]() {
    igc_cache.PollBackgroundFill();
    RefreshIgcRows();
  }};
  PixelRect checkbox_rect;
  PixelRect total_rect;
  std::unique_ptr<WndFrame> total_line;
  AllocatedPath target_device_path;
  std::unique_ptr<CheckBoxControl> nmea_checkbox;
  bool show_nmea_files{false};
  /** No export target and no NMEA checkbox. The list fills the dialog. */
  const bool list_only;
  Path initial_path{nullptr};
  FileMetadataFormatter file_metadata;

  explicit FlightContainer(MultiFileDataField &df, bool _list_only)
    : PropertyWidgetContainer(_("Target")),
      file_list(std::make_unique<FileMultiSelectWidget>(
          df, nullptr, C_("Setting", "Flights"), nullptr)),
      list_only(_list_only)
  {
    file_metadata.Build(df.GetAllPaths());
    file_list->SetSecondRightProvider([this](const FileMultiSelectWidget::FileItem &it) noexcept {
      return file_metadata.GetSizeText(it.path);
    });
    file_list->SetSecondLeftProvider(GetIgcMetadata);
  }

  void RefreshIgcRows() {
    if (file_list)
      file_list->InvalidateRows();
    UpdateTotal();
  }

  static unsigned TotalLineHeight() noexcept {
    const DialogLook &look = UIGlobals::GetDialogLook();
    return 2 * Layout::GetTextPadding() + look.text_font.GetHeight();
  }

  void UpdateTotal() {
    if (!total_line || !file_list)
      return;

    std::chrono::seconds total{0};
    unsigned flights = 0;
    bool complete = true;
    for (const auto &path : file_list->GetAllPaths()) {
      if (!path.EndsWithIgnoreCase(".igc"))
        continue;

      const auto flight = igc_cache.GetFlight(path);
      if (!flight) {
        complete = false;
        continue;
      }

      if (!flight->detected)
        continue;

      ++flights;
      total += flight->duration;
    }

    const auto seconds = total.count();
    const auto hours = static_cast<unsigned>(seconds / 3600);
    const auto minutes = static_cast<unsigned>((seconds % 3600) / 60);

    StaticString<16> flight_n, span;
    flight_n.Format("%u", flights);
    /* Hours are not wrapped at 24. FormatSignedTimeHHMM() would turn
       106 h 47 min into 10:47. */
    span.Format("%u:%02u", hours, minutes);

    StaticString<48> flight_label;
    flight_label.Format(_("%s: %s"), C_("Setting", "Flights"),
                        flight_n.c_str());

    StaticString<80> text;
    text.Format("%s  %s", flight_label.c_str(), span.c_str());
    if (!complete)
      text.append("...");

    total_line->SetText(text);
  }

  void CreateTotalLine(ContainerWindow &parent) {
    if (total_line)
      return;

    const DialogLook &look = UIGlobals::GetDialogLook();
    WindowStyle style;
    style.Hide();
    total_line = std::make_unique<WndFrame>(parent, look, total_rect, style);
    total_line->SetTopSeparator();
    total_line->SetVAlignCenter();
  }

  void SetInitialPath(Path path) noexcept {
    initial_path = path;
  }

  void SelectInitialPath() noexcept {
    if (initial_path == nullptr || initial_path.empty())
      return;

    const auto paths = file_list->GetAllPaths();
    for (unsigned i = 0; i < paths.size(); ++i) {
      if (paths[i] == initial_path) {
        file_list->SetCursorIndex(i);
        return;
      }
    }
  }

  Widget &GetContentWidget() noexcept override { return *file_list; }
  const Widget &GetContentWidget() const noexcept override { return *file_list; }

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
    try {
      std::vector<AllocatedPath> paths;
      const auto all_paths = file_list->GetAllPaths();
      paths.reserve(all_paths.size());
      for (const auto &path : all_paths)
        if (path.EndsWithIgnoreCase(".igc"))
          paths.emplace_back(path);

      igc_cache.StartBackgroundFill(std::move(paths),
                                    &igc_progress_notify,
                                    &igc_completion_notify);
    } catch (...) {
      LogError(std::current_exception(), "Failed to start IGC metadata worker");
      igc_cache.CancelBackgroundFill();
    }
  }

  void CalculateLayout(const PixelRect &rc) noexcept override {
    const unsigned spacing = Layout::GetTextPadding();
    const unsigned total_height = TotalLineHeight();

    if (list_only) {
      content_rect = rc;
      content_rect.bottom = rc.bottom
        - static_cast<int>(total_height + spacing);
      total_rect = rc;
      total_rect.top = content_rect.bottom + static_cast<int>(spacing);
      return;
    }

    PropertyWidgetContainer::CalculateLayout(rc);
    const unsigned prop_height = Layout::GetMinimumControlHeight();
    content_rect.bottom = rc.bottom
      - static_cast<int>(prop_height + total_height + 2 * spacing);
    total_rect = rc;
    total_rect.top = content_rect.bottom + static_cast<int>(spacing);
    total_rect.bottom = total_rect.top + static_cast<int>(total_height);
    checkbox_rect = rc;
    checkbox_rect.top = total_rect.bottom + static_cast<int>(spacing);
    checkbox_rect.bottom = checkbox_rect.top + static_cast<int>(prop_height);
  }

  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override {
    if (list_only)
      return;

    PropertyWidgetContainer::Initialise(parent, rc);
    if (!nmea_checkbox)
      nmea_checkbox = std::make_unique<CheckBoxControl>();
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    if (list_only) {
      CalculateLayout(rc);
      GetContentWidget().Prepare(parent, content_rect);
    } else {
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
        UpdateTotal();
      });
      nmea_checkbox->SetState(show_nmea_files);

      ApplyNmeaFilter();
      UpdateTargetCaption();
    }

    CreateTotalLine(parent);
    file_list->Refresh();
    file_list->ClearSelection();
    SelectInitialPath();
    StartIgcCacheFill();
    UpdateTotal();
  }

  void Unprepare() noexcept override {
    igc_cache.CancelBackgroundFill();
    igc_progress_notify.ClearNotification();
    igc_completion_notify.ClearNotification();
    PropertyWidgetContainer::Unprepare();
    total_line.reset();
    nmea_checkbox.reset();
  }

  void Show(const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Show(rc);
    if (total_line)
      total_line->MoveAndShow(total_rect);
    if (nmea_checkbox)
      nmea_checkbox->MoveAndShow(checkbox_rect);
  }

  void Hide() noexcept override {
    PropertyWidgetContainer::Hide();
    if (total_line)
      total_line->Hide();
    if (nmea_checkbox)
      nmea_checkbox->Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    PropertyWidgetContainer::Move(rc);
    if (total_line && total_line->IsDefined())
      total_line->Move(total_rect);
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
ShowExportFlightsDialog(ExportFlightsMode mode)
{
  const bool weglide = mode == ExportFlightsMode::WEGLIDE;
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, weglide
                      ? _("WeGlide Upload")
                      : C_("Menu", "Export flights"));

  auto df = ScanLogs(weglide);
  auto container = std::make_unique<FlightContainer>(*df, weglide);
  FlightContainer &flight_container = *container;
  FileMultiSelectWidget &file_list = *flight_container.file_list;

  if (!weglide) {
    const AllocatedPath &last = GetLastStorageTarget();
    if (last != nullptr)
      flight_container.SetTargetDevice(Path(last));
  }

  if (weglide) {
    file_list.SetShowCheckmarks(false);
    file_list.SetFileActivateCallback([](Path picked) {
      UploadOneIGCFile(picked);
    });
    dialog.AddButton(_("Upload Flight"), [&file_list]() {
      UseHighlightedFlight(file_list);
    });
    dialog.AddButton(_("Cancel"), mrCancel);
  } else {
    dialog.AddButton(C_("Button", "Choose location"), [&flight_container]() {
      PickStorageLocationAndApply([&flight_container](AllocatedPath chosen) {
        flight_container.SetTargetDevice(std::move(chosen));
      });
    });

    dialog.AddButton(_("WeGlide Upload"), [&file_list]() {
      PerformWeGlideUpload(&file_list);
    });
#ifdef HAVE_HTTP
    dialog.AddButton(_("SoaringScoring"), [&file_list]() {
      PerformSoaringScoringUpload(&file_list);
    });
#endif
#ifdef ANDROID
    dialog.AddSymbolButton("s", [&file_list]() {
      PerformShare(&file_list);
    });
#endif
    dialog.AddButton(C_("Button", "Export"), [&file_list]() {
      PerformExport(&file_list);
    });
    dialog.AddButton(C_("Button", "Select all"), [&file_list]() {
      file_list.SelectAll();
    });
    dialog.AddButton(C_("Button", "Select none"), [&file_list]() {
      file_list.ClearSelection();
    });
    dialog.AddButton(C_("Button", "Back"), mrCancel);
  }

  dialog.FinishPreliminary(std::move(container));
  dialog.ShowModal();
}

ReplayFlightChoice
PickReplayFlight(const char *caption, AllocatedPath &path)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look,
                      caption != nullptr ? caption : _("Replay"));

  auto df = ScanLogs(false);
  auto container = std::make_unique<FlightContainer>(*df, true);
  FlightContainer &flight_container = *container;
  flight_container.SetInitialPath(path);
  FileMultiSelectWidget &file_list = *flight_container.file_list;
  file_list.SetShowCheckmarks(false);

  AllocatedPath chosen;
  bool demo = false;

  file_list.SetFileActivateCallback([&](Path picked) {
    if (picked == nullptr || picked.empty())
      return;

    chosen = AllocatedPath(picked);
    demo = false;
    dialog.SetModalResult(mrOK);
  });

  dialog.AddButton(_("Select"), [&file_list]() {
    UseHighlightedFlight(file_list);
  });
  dialog.AddButton(_("Demo"), [&]() {
    demo = true;
    dialog.SetModalResult(mrOK);
  });
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.FinishPreliminary(std::move(container));
  if (dialog.ShowModal() != mrOK)
    return ReplayFlightChoice::CANCEL;
  if (demo)
    return ReplayFlightChoice::DEMO;

  path = std::move(chosen);
  return ReplayFlightChoice::FILE;
}
