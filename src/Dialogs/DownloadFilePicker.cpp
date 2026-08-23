// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilePicker.hpp"
#include "EmptyDownloadList.hpp"
#include "Error.hpp"
#include "Message.hpp"
#include "WidgetDialog.hpp"
#include "DownloadFileModal.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Language/Language.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "system/Path.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/FileArea.hpp"
#include "Repository/CountryName.hpp"
#include "Repository/Glue.hpp"
#include "util/StringAPI.hxx"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "ui/event/Notify.hpp"
#include "ui/canvas/Canvas.hpp"
#include "thread/Mutex.hxx"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>


class RepositoryFilePickerWidget final
  : public MultiSelectListWidget,
    Net::DownloadListener {

  WidgetDialog &dialog;

  UI::Notify download_complete_notify{[this]{
    OnDownloadCompleteNotification();
  }};

  const FileType file_type;
  const bool watch_repository;
  const bool download_on_confirm;

  TwoTextRowsRenderer row_renderer;

  Button *primary_button = nullptr;
  Button *select_all_button = nullptr;
  Button *back_button = nullptr;

  std::vector<AvailableFile> all_files;
  std::vector<std::string> areas;
  std::vector<AvailableFile> visible_files;

  std::string selected_area;
  bool showing_areas = false;
  bool allow_area_step = false;

  std::vector<AvailableFile> chosen_files;
  std::vector<AllocatedPath> downloaded_paths;

  /**
   * This mutex protects the attribute "repository_modified".
   */
  mutable Mutex mutex;

  bool repository_modified = false;
  bool repository_failed = false;
  std::exception_ptr repository_error;

public:
  RepositoryFilePickerWidget(WidgetDialog &_dialog, FileType _file_type)
    :dialog(_dialog), file_type(_file_type),
     watch_repository(true), download_on_confirm(true) {}

  RepositoryFilePickerWidget(WidgetDialog &_dialog,
                             std::vector<AvailableFile> &&files)
    :dialog(_dialog), file_type(FileType::UNKNOWN),
     watch_repository(false), download_on_confirm(false),
     all_files(std::move(files)) {}

  std::vector<AllocatedPath> &&TakeDownloadedPaths() {
    return std::move(downloaded_paths);
  }

  std::vector<AvailableFile> &&TakeChosenFiles() {
    return std::move(chosen_files);
  }

  void CreateButtons();

protected:
  void RefreshList();
  void RefreshRepository() noexcept;
  void UpdateButtons();
  void ShowAreaList();
  void ShowFileList();
  void OpenArea(unsigned index);
  void SortAreas();
  [[gnu::pure]]
  const char *GetAreaCaption(const std::string &area) const noexcept;
  void OnPrimary();
  void OnSelectAll();
  void ConfirmSelection();
  AllocatedPath DownloadOne(const AvailableFile &file);
  void PaintAreaItem(Canvas &canvas, const PixelRect rc,
                     unsigned idx) noexcept;
  void PaintFileItem(Canvas &canvas, const PixelRect rc,
                     unsigned idx) noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;
  unsigned OnListResized() noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const noexcept override;
  void OnActivateItem(unsigned index) noexcept override;

  /* virtual methods from class MultiSelectListWidget */
  void OnSelectionChanged() noexcept override;

  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path path_relative,
                       int64_t size, int64_t position) noexcept override;
  void OnDownloadComplete(Path path_relative) noexcept override;
  void OnDownloadError(Path path_relative,
                       std::exception_ptr error) noexcept override;

  void OnDownloadCompleteNotification() noexcept;
};

void
RepositoryFilePickerWidget::Prepare(ContainerWindow &parent,
                                    const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  unsigned row_height =
    row_renderer.CalculateLayout(*look.list.font, look.small_font);
  if (all_files.empty() && watch_repository)
    row_height = LayoutEmptyDownloadRow(row_renderer);

  CreateList(parent, look, rc, row_height);
  MultiSelectListWidget::Prepare(parent, rc);
  RefreshList();

  if (watch_repository) {
    Net::DownloadManager::AddListener(*this);
    Net::DownloadManager::Enumerate(*this);
    EnqueueRepositoryDownload();
  }
}

void
RepositoryFilePickerWidget::Unprepare() noexcept
{
  if (watch_repository)
    Net::DownloadManager::RemoveListener(*this);
}

void
RepositoryFilePickerWidget::RefreshList()
{
  {
    const std::lock_guard lock{mutex};
    repository_modified = false;
    repository_failed = false;
  }

  if (watch_repository) {
    FileRepository repository;
    LoadAllRepositories(repository);

    all_files.clear();
    for (auto &i : repository)
      if (i.type == file_type)
        all_files.emplace_back(std::move(i));
  }

  areas = CollectUniqueFileAreas(all_files);
  SortAreas();
  allow_area_step = areas.size() > 1;

  if (all_files.empty()) {
    showing_areas = false;
    visible_files.clear();
    GetList().SetLength(1);
    GetList().Invalidate();
    UpdateButtons();
    return;
  }

  if (allow_area_step && showing_areas) {
    ShowAreaList();
    return;
  }

  if (allow_area_step && !selected_area.empty() &&
      std::find(areas.begin(), areas.end(),
                selected_area) != areas.end()) {
    ShowFileList();
    return;
  }

  if (allow_area_step) {
    ShowAreaList();
    return;
  }

  selected_area.clear();
  ShowFileList();
}

void
RepositoryFilePickerWidget::RefreshRepository() noexcept
{
  if (watch_repository)
    EnqueueRepositoryDownload(true);
}

void
RepositoryFilePickerWidget::ShowAreaList()
{
  showing_areas = true;
  visible_files.clear();

  unsigned cursor = 0;
  if (!selected_area.empty()) {
    for (unsigned i = 0; i < areas.size(); ++i) {
      if (areas[i] == selected_area) {
        cursor = i;
        break;
      }
    }
  }

  SetLengthWithSelection(areas.size());
  GetList().SetCursorIndex(cursor);
  GetList().Invalidate();
  UpdateButtons();
}

void
RepositoryFilePickerWidget::ShowFileList()
{
  showing_areas = false;
  visible_files.clear();
  if (allow_area_step)
    AppendFilesInArea(all_files, selected_area, visible_files);
  else
    visible_files = all_files;

  SetLengthWithSelection(visible_files.size());
  GetList().SetCursorIndex(0);
  GetList().Invalidate();
  UpdateButtons();
}

void
RepositoryFilePickerWidget::SortAreas()
{
  std::sort(areas.begin(), areas.end(),
            [this](const std::string &a, const std::string &b) {
              if (a.empty() != b.empty())
                return a.empty();
              return StringCollate(GetAreaCaption(a),
                                   GetAreaCaption(b)) < 0;
            });
}

const char *
RepositoryFilePickerWidget::GetAreaCaption(const std::string &area)
  const noexcept
{
  if (area.empty())
    return _("Regions");
  if (const char *name = GetCountryName(area))
    return gettext(name);
  return area.c_str();
}

void
RepositoryFilePickerWidget::OpenArea(unsigned index)
{
  assert(index < areas.size());
  selected_area = areas[index];
  ShowFileList();
}

void
RepositoryFilePickerWidget::CreateButtons()
{
  primary_button = dialog.AddButton(_("Download"), [this](){
    OnPrimary();
  });
  select_all_button = dialog.AddButton(C_("Button", "Select all"),
                                       [this](){ OnSelectAll(); });
  back_button = dialog.AddButton(_("Back"), [this](){
    ShowAreaList();
  });

  UpdateButtons();
}

void
RepositoryFilePickerWidget::UpdateButtons()
{
  if (primary_button == nullptr)
    return;

  const bool empty = all_files.empty();
  const bool files_view = !empty && !showing_areas;

  if (empty)
    primary_button->SetCaption(_("Download"));
  else if (showing_areas)
    primary_button->SetCaption(_("Select"));
  else if (download_on_confirm)
    primary_button->SetCaption(_("Download"));
  else
    primary_button->SetCaption(C_("Button", "Add"));

  primary_button->SetEnabled(empty ||
                             (showing_areas
                              ? !areas.empty()
                              : !visible_files.empty()));

  if (select_all_button != nullptr) {
    select_all_button->SetEnabled(files_view && !visible_files.empty());
    if (files_view && !visible_files.empty() &&
        GetSelectedCount() == visible_files.size())
      select_all_button->SetCaption(C_("Button", "Select none"));
    else
      select_all_button->SetCaption(C_("Button", "Select all"));
  }

  if (back_button != nullptr)
    back_button->SetEnabled(files_view && allow_area_step);

  dialog.ResyncButtonPanelSelection();
}

void
RepositoryFilePickerWidget::OnSelectAll()
{
  if (showing_areas || visible_files.empty())
    return;

  if (GetSelectedCount() == visible_files.size())
    ClearSelection();
  else
    SelectAll();
}

void
RepositoryFilePickerWidget::OnPrimary()
{
  if (all_files.empty()) {
    RefreshRepository();
    return;
  }

  if (showing_areas) {
    const unsigned current = GetList().GetCursorIndex();
    if (current < areas.size())
      OpenArea(current);
    return;
  }

  ConfirmSelection();
}

AllocatedPath
RepositoryFilePickerWidget::DownloadOne(const AvailableFile &file)
{
  const auto relative_path = GetFileDownloadRelativePath(file);
  if (relative_path == nullptr)
    throw std::runtime_error("Invalid download filename");

  const AllocatedPath dest_dir = GetFileTypeDefaultDir(file.type);
  if (dest_dir != nullptr) {
    const auto dest_path = LocalPath(dest_dir);
    Directory::CreateRecursive(dest_path);
    if (!Directory::Exists(dest_path))
      throw std::runtime_error("Directory does not exist and "
                               "could not be created.");
  }

  return DownloadFileModal(_("Download"), file.GetURI(),
                           relative_path.c_str());
}

void
RepositoryFilePickerWidget::ConfirmSelection()
{
  assert(!showing_areas);

  auto indices = GetSelectedIndices();
  if (indices.empty()) {
    if (visible_files.empty())
      return;
    indices.push_back(GetList().GetCursorIndex());
  }

  chosen_files.clear();
  for (unsigned i : indices) {
    assert(i < visible_files.size());
    chosen_files.push_back(visible_files[i]);
  }

  if (!download_on_confirm) {
    dialog.SetModalResult(mrOK);
    return;
  }

  downloaded_paths.clear();
  for (const auto &file : chosen_files) {
    try {
      auto path = DownloadOne(file);
      if (path == nullptr) {
        if (!downloaded_paths.empty())
          dialog.SetModalResult(mrOK);
        return;
      }
      downloaded_paths.push_back(std::move(path));
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
      return;
    }
  }

  dialog.SetModalResult(mrOK);
}

unsigned
RepositoryFilePickerWidget::OnListResized() noexcept
{
  if (all_files.empty())
    return LayoutEmptyDownloadRow(row_renderer);

  const DialogLook &look = UIGlobals::GetDialogLook();
  return row_renderer.CalculateLayout(*look.list.font, look.small_font);
}

bool
RepositoryFilePickerWidget::CanActivateItem(unsigned index) const noexcept
{
  if (all_files.empty())
    return index == 0;
  if (showing_areas)
    return index < areas.size();
  return index < visible_files.size();
}

void
RepositoryFilePickerWidget::OnActivateItem(unsigned index) noexcept
{
  if (all_files.empty()) {
    assert(index == 0);
    RefreshRepository();
    return;
  }

  if (showing_areas)
    OpenArea(index);
  else
    ToggleSelection(index);
}

void
RepositoryFilePickerWidget::OnSelectionChanged() noexcept
{
  UpdateButtons();
}

void
RepositoryFilePickerWidget::PaintAreaItem(Canvas &canvas, const PixelRect rc,
                                          unsigned idx) noexcept
{
  assert(idx < areas.size());

  const auto &area = areas[idx];
  row_renderer.DrawFirstRow(canvas, rc, GetAreaCaption(area));

  const unsigned count = CountFilesInArea(all_files, area);
  StaticString<64> count_text;
  if (count == 1)
    count_text = _("1 file");
  else
    count_text.Format(_("%u files"), count);
  row_renderer.DrawSecondRow(canvas, rc, count_text);
}

void
RepositoryFilePickerWidget::PaintFileItem(Canvas &canvas, const PixelRect rc,
                                          unsigned idx) noexcept
{
  assert(idx < visible_files.size());

  const auto &file = visible_files[idx];
  const DialogLook &look = UIGlobals::GetDialogLook();
  const bool focused = !HasCursorKeys() || GetList().HasFocus();
  const unsigned padding = Layout::GetTextPadding();
  const unsigned box_size = rc.GetHeight() > 2 * padding
    ? rc.GetHeight() - 2 * padding
    : 0;

  PixelRect box_rc;
  box_rc.left = rc.left + (int)padding;
  box_rc.top = rc.top + (int)padding;
  box_rc.right = box_rc.left + (int)box_size;
  box_rc.bottom = box_rc.top + (int)box_size;

  DrawCheckBox(canvas, look, box_rc, IsSelected(idx), focused, false, true);

  PixelRect text_rc = rc;
  text_rc.left = box_rc.right + 2 * (int)padding;

  if (file.GetName())
    row_renderer.DrawFirstRow(canvas, text_rc, file.GetName());

  if (file.GetDescription() && *file.GetDescription() != '\0')
    row_renderer.DrawSecondRow(canvas, text_rc, file.GetDescription());

  if (file.update_date.IsPlausible()) {
    char string_buffer[21];
    FormatISO8601(string_buffer, file.update_date);
    row_renderer.DrawRightSecondRow(canvas, text_rc, string_buffer);
  }
}

void
RepositoryFilePickerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                        unsigned i) noexcept
{
  if (all_files.empty()) {
    assert(i == 0);
    DrawEmptyDownloadHint(row_renderer, canvas, rc);
    return;
  }

  if (showing_areas)
    PaintAreaItem(canvas, rc, i);
  else
    PaintFileItem(canvas, rc, i);
}

void
RepositoryFilePickerWidget::OnDownloadAdded([[maybe_unused]] Path path_relative,
                                            [[maybe_unused]] int64_t size,
                                            [[maybe_unused]] int64_t position) noexcept
{
}

void
RepositoryFilePickerWidget::OnDownloadComplete(Path path_relative) noexcept
{
  if (!watch_repository)
    return;

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
RepositoryFilePickerWidget::OnDownloadError(Path path_relative,
                                            std::exception_ptr error) noexcept
{
  if (!watch_repository)
    return;

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
RepositoryFilePickerWidget::OnDownloadCompleteNotification() noexcept
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

std::vector<AllocatedPath>
DownloadFilePicker(FileType file_type)
{
  if (!Net::DownloadManager::IsAvailable()) {
    const char *message =
      _("The file manager is not available on this device.");
    ShowMessageBox(message, _("File Manager"), MB_OK);
    return {};
  }

  TWidgetDialog<RepositoryFilePickerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Download"));
  dialog.SetWidget(dialog, file_type);
  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);
  /* No EnableCursorSelection: Left/Right page the list (ListControl).
     Up/Down walk list ↔ buttons; Enter on a country opens it, Enter
     on a file toggles the checkbox. */
  if (dialog.ShowModal() != mrOK)
    return {};

  return dialog.GetWidget().TakeDownloadedPaths();
}

std::vector<AvailableFile>
SelectAvailableFiles(std::vector<AvailableFile> files)
{
  if (files.empty() || !Net::DownloadManager::IsAvailable())
    return {};

  TWidgetDialog<RepositoryFilePickerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Select a file"));
  dialog.SetWidget(dialog, std::move(files));
  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);
  if (dialog.ShowModal() != mrOK)
    return {};

  return dialog.GetWidget().TakeChosenFiles();
}
