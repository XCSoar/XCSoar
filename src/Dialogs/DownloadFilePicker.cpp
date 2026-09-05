// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilePicker.hpp"
#include "DownloadFilter.hpp"
#include "EmptyDownloadList.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Error.hpp"
#include "WidgetDialog.hpp"
#include "DownloadFileModal.hpp"
#include "Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/String.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "ui/control/List.hpp"
#include "Language/Language.hpp"
#include "system/Path.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Glue.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "thread/Mutex.hxx"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include <vector>

#include <cassert>


/**
 * A row for the country filter: shows "All" or the ticked countries,
 * and opens the checkbox list (DownloadFilter::EditAreas()) when
 * edited.
 */
class DownloadAreasDataField final : public DataFieldString {
public:
  explicit DownloadAreasDataField(DataFieldListener *listener) noexcept
    :DataFieldString("", listener)
  {
    char buffer[256];
    SetValue(DownloadFilter::FormatAreas(buffer));
  }

  /** the selection changed: refresh the text and tell the listener */
  void Update() noexcept {
    char buffer[256];
    ModifyValue(DownloadFilter::FormatAreas(buffer));
  }
};

static bool
EditDownloadAreas([[maybe_unused]] const char *caption, DataField &df,
                  [[maybe_unused]] const char *help_text) noexcept
{
  if (!DownloadFilter::EditAreas())
    return false;

  static_cast<DownloadAreasDataField &>(df).Update();
  return true;
}

class DownloadFilePickerWidget final
  : public RowFormWidget, ListItemRenderer, ListCursorHandler,
    DataFieldListener,
    Net::DownloadListener {

  enum Controls { AREAS, SEARCH };

  WidgetDialog &dialog;

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  const FileType file_type;

  /** countries/search only where they make sense - not for firmware
      images and the like */
  const bool filtered;

  Button *download_button = nullptr;

  ListControl *list = nullptr;

  std::vector<AvailableFile> items;

  /** is the repository index itself empty/missing (as opposed to
      the filter leaving nothing)? */
  bool repository_empty = true;

  TextRowRenderer row_renderer;

  /**
   * This mutex protects the attribute "repository_modified".
   */
  mutable Mutex mutex;

  /**
   * Was the repository file modified, and needs to be reloaded by
   * RefreshList()?
   */
  bool repository_modified;

  /**
   * Has the repository file download failed?
   */
  bool repository_failed;

  std::exception_ptr repository_error;

  AllocatedPath path;

public:
  DownloadFilePickerWidget(WidgetDialog &_dialog, FileType _file_type)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dialog(_dialog), file_type(_file_type),
     filtered(DownloadFilter::AppliesTo(_file_type)) {}

  AllocatedPath &&GetPath() {
    return std::move(path);
  }

  void CreateButtons();

protected:
  void RefreshList();
  void RefreshRepository() noexcept;

  void UpdateButtons() {
    if (download_button != nullptr)
      download_button->SetEnabled(!items.empty() || repository_empty);
  }

  void Download();
  void Cancel();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    if (items.empty()) {
      if (repository_empty)
        RefreshRepository();
    } else
      Download();
  }

  /* virtual methods from class DataFieldListener */
  void OnModified(DataField &df) noexcept override {
    if (IsDataField(SEARCH, df))
      DownloadFilter::SetSearchText(df.GetAsString());

    RefreshList();
  }

  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path path_relative,
                       int64_t size, int64_t position) noexcept override;
  void OnDownloadComplete(Path path_relative) noexcept override;
  void OnDownloadError(Path path_relative,
                       std::exception_ptr error) noexcept override;

  void OnDownloadCompleteNotification() noexcept;
};

void
DownloadFilePickerWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  if (filtered) {
  DownloadFilter::LoadFromProfile();

  Add(_("Countries"),
      _("Show only the files of these countries - the same selection "
        "for maps, waypoints and airspaces, kept in the profile.  "
        "Files that concern every country stay listed."),
      new DownloadAreasDataField(this));
  GetControl(AREAS).SetEditCallback(EditDownloadAreas);

  Add(_("Search"),
      _("Show only the files whose name or description contains this "
        "text."),
      new DataFieldString(DownloadFilter::GetSearchText(), this));
  }

  const DialogLook &look = UIGlobals::GetDialogLook();

  const unsigned row_height =
    std::max(row_renderer.CalculateLayout(*look.list.font),
             LayoutEmptyDownloadRow(row_renderer));

  WindowStyle style;
  style.TabStop();
  auto l = std::make_unique<ListControl>((ContainerWindow &)GetWindow(), look,
                                         rc, style, row_height);
  l->SetItemRenderer(this);
  l->SetCursorHandler(this);
  list = l.get();
  AddRemaining(std::move(l));

  RefreshList();

  Net::DownloadManager::AddListener(*this);
  Net::DownloadManager::Enumerate(*this);

  EnqueueRepositoryDownload();
}

void
DownloadFilePickerWidget::Unprepare() noexcept
{
  Net::DownloadManager::RemoveListener(*this);
}

void
DownloadFilePickerWidget::RefreshList()
{
  {
    const std::lock_guard lock{mutex};
    repository_modified = false;
    repository_failed = false;
  }

  FileRepository repository;
  LoadAllRepositories(repository);

  repository_empty = repository.begin() == repository.end();

  items.clear();
  for (auto &i : repository)
    if (i.type == file_type &&
        (!filtered ||
         (DownloadFilter::MatchesArea(i) &&
          DownloadFilter::MatchesSearch(i))))
      items.emplace_back(std::move(i));

  list->SetLength(std::max(items.size(), size_t{1}));
  list->Invalidate();

  UpdateButtons();
}

void
DownloadFilePickerWidget::RefreshRepository() noexcept
{
  EnqueueRepositoryDownload(true);
}

void
DownloadFilePickerWidget::CreateButtons()
{
  download_button = dialog.AddButton(_("Download"), [this](){ Download(); });

  UpdateButtons();
}

void
DownloadFilePickerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                      unsigned i) noexcept
{
  if (items.empty()) {
    assert(i == 0);

    if (repository_empty)
      DrawEmptyDownloadHint(row_renderer, canvas, rc);
    else
      row_renderer.DrawTextRow(canvas, rc,
                               _("No file matches the filter."));
    return;
  }

  const auto &file = items[i];

  row_renderer.DrawTextRow(canvas, rc, file.GetName());
}

void
DownloadFilePickerWidget::Download()
{
  assert(Net::DownloadManager::IsAvailable());

  if (items.empty()) {
    if (repository_empty)
      RefreshRepository();
    return;
  }

  const unsigned current = list->GetCursorIndex();
  assert(current < items.size());

  const auto &file = items[current];

  try {
    AllocatedPath dest_dir = GetFileTypeDefaultDir(file_type);

    const Path file_path(file.GetName()); //AllocatedPath cannot take nullptr

    if (!file_path.IsValidFilename())
      throw std::runtime_error("Invalid download filename");

    AllocatedPath relative_path(file_path);
    if (dest_dir != nullptr) {
      const auto dest_path = LocalPath(dest_dir);
      Directory::CreateRecursive(dest_path);
      if (!Directory::Exists(dest_path))
        throw std::runtime_error("Directory does not exist and could not be created.");

      relative_path = AllocatedPath::Build(Path(dest_dir), file_path);
    }
    path = DownloadFileModal(_("Download"), file.GetURI(), relative_path.c_str());
    if (path != nullptr)
      dialog.SetModalResult(mrOK);
  } catch (...) {
    ShowError(std::current_exception(), _("Error"));
  }
}

void
DownloadFilePickerWidget::OnDownloadAdded([[maybe_unused]] Path path_relative,
                                          [[maybe_unused]] int64_t size,
                                          [[maybe_unused]] int64_t position) noexcept
{
}

void
DownloadFilePickerWidget::OnDownloadComplete(Path path_relative) noexcept
{
  const auto name = path_relative.GetBase();
  if (name == nullptr)
    return;

  const bool is_main = name == Path("repository");
  const bool is_user = IsUserRepositoryFile(name.c_str());

  if (is_main || is_user) {
    const std::lock_guard lock{mutex};
    if (is_main)
      repository_failed = false;
    repository_modified = true;
  }

  download_complete_notify.SendNotification();
}

void
DownloadFilePickerWidget::OnDownloadError(Path path_relative,
                                          std::exception_ptr error) noexcept
{
  const auto name = path_relative.GetBase();
  if (name == nullptr)
    return;

  if (name == Path("repository")) {
    const std::lock_guard lock{mutex};
    repository_failed = true;
    repository_error = std::move(error);
  }

  /* user repository download errors are silently ignored 
     one warning is enough on network loss */

  download_complete_notify.SendNotification();
}

void
DownloadFilePickerWidget::OnDownloadCompleteNotification() noexcept
{
  bool repository_modified2, repository_failed2;
  std::exception_ptr repository_error2;

  {
    const std::lock_guard lock{mutex};
    repository_modified2 = std::exchange(repository_modified, false);
    repository_failed2 = std::exchange(repository_failed, false);
    repository_error2 = std::move(repository_error);
  }

  if (repository_error2)
    ShowError(std::move(repository_error2),
              _("Failed to download the repository index."));
  else if (repository_failed2)
    ShowMessageBox(_("Failed to download the repository index."),
                   _("Error"), MB_OK);

  if (repository_modified2)
    RefreshList();
}

AllocatedPath
DownloadFilePicker(FileType file_type)
{
  if (!Net::DownloadManager::IsAvailable()) {
    const char *message =
      _("The file manager is not available on this device.");
    ShowMessageBox(message, _("File Manager"), MB_OK);
    return nullptr;
  }

  TWidgetDialog<DownloadFilePickerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Download"));
  dialog.SetWidget(dialog, file_type);
  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Close"), mrCancel);
  /* No EnableCursorSelection: Left/Right page the list (ListControl).
     Up/Down walk list ↔ Download/Close; Enter downloads the cursor row. */
  dialog.ShowModal();

  return dialog.GetWidget().GetPath();
}
