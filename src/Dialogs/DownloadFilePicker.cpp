// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilePicker.hpp"
#include "DownloadFilter.hpp"
#include "EmptyDownloadList.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Error.hpp"
#include "WidgetDialog.hpp"
#include "DownloadFileModal.hpp"
#include "Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/String.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "ui/control/List.hpp"
#include "ui/event/KeyCode.hpp"
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
#include <algorithm>
#include <string>
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
    DataFieldListener, Net::DownloadListener {

  enum Controls { AREAS, SEARCH };

  WidgetDialog &dialog;

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  const FileType file_type;

  /** countries/search only where they make sense - not for firmware
      images and the like */
  const bool filtered;

  Button *download_button = nullptr;
  Button *select_button = nullptr;

  ListControl *list = nullptr;

  std::vector<AvailableFile> items;

  /** parallel to #items: a ticked row */
  std::vector<uint8_t> selected;

  /** is the repository index itself empty/missing (as opposed to
      the filter leaving nothing)? */
  bool repository_empty = true;

  TwoTextRowsRenderer row_renderer;

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

  std::vector<AllocatedPath> paths;

public:
  DownloadFilePickerWidget(WidgetDialog &_dialog, FileType _file_type)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     dialog(_dialog), file_type(_file_type),
     filtered(DownloadFilter::AppliesTo(_file_type)) {}

  std::vector<AllocatedPath> &&GetPaths() noexcept {
    return std::move(paths);
  }

  void CreateButtons();

protected:
  void RefreshList();
  void RefreshRepository() noexcept;

  void UpdateButtons() noexcept;

  [[gnu::pure]]
  unsigned GetSelectedCount() const noexcept;

  void SetAllSelected(bool value) noexcept;
  void ToggleSelection(unsigned idx) noexcept;

  void Download();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept
    override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    if (items.empty()) {
      if (repository_empty)
        RefreshRepository();
    } else
      ToggleSelection(index);
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
    std::max(row_renderer.CalculateLayout(*look.list.font,
                                          look.small_font),
             LayoutEmptyDownloadRow(row_renderer));

  WindowStyle style;
  style.TabStop();
  auto l = std::make_unique<ListControl>((ContainerWindow &)GetWindow(), look,
                                         rc, style, row_height);
  l->SetItemRenderer(this);
  l->SetCursorHandler(this);
  /* a tap ticks the row; a drag still scrolls */
  l->SetActivateOnFirstClick(true);
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

bool
DownloadFilePickerWidget::KeyPress(unsigned key_code) noexcept
{
  /* Return reaches the list and toggles the row.  Space would
     otherwise fire a button, so tick the focused row here. */
  if (key_code != KEY_SPACE || list == nullptr || !list->HasFocus())
    return false;

  const unsigned index = list->GetCursorIndex();
  if (index < items.size())
    ToggleSelection(index);
  return true;
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

  std::vector<std::string> kept;
  for (unsigned i = 0; i < items.size() && i < selected.size(); ++i)
    if (selected[i])
      kept.emplace_back(items[i].name);

  items.clear();
  for (auto &i : repository)
    if (i.type == file_type &&
        (!filtered ||
         (DownloadFilter::MatchesArea(i) &&
          DownloadFilter::MatchesSearch(i))))
      items.emplace_back(std::move(i));

  selected.assign(items.size(), 0);
  for (unsigned i = 0; i < items.size(); ++i)
    if (std::find(kept.begin(), kept.end(), items[i].name) != kept.end())
      selected[i] = 1;

  list->SetLength(std::max(items.size(), size_t{1}));
  list->Invalidate();

  UpdateButtons();
}

void
DownloadFilePickerWidget::RefreshRepository() noexcept
{
  EnqueueRepositoryDownload(true);
}

unsigned
DownloadFilePickerWidget::GetSelectedCount() const noexcept
{
  unsigned n = 0;
  for (uint8_t value : selected)
    if (value)
      ++n;
  return n;
}

void
DownloadFilePickerWidget::SetAllSelected(bool value) noexcept
{
  if (selected.empty())
    return;

  std::fill(selected.begin(), selected.end(), value ? 1 : 0);
  if (list != nullptr)
    list->Invalidate();
  UpdateButtons();
}

void
DownloadFilePickerWidget::ToggleSelection(unsigned idx) noexcept
{
  if (idx >= selected.size())
    return;

  selected[idx] = !selected[idx];
  if (list != nullptr)
    list->Invalidate();
  UpdateButtons();
}

void
DownloadFilePickerWidget::UpdateButtons() noexcept
{
  if (download_button != nullptr)
    download_button->SetEnabled(!items.empty() || repository_empty);

  if (select_button == nullptr)
    return;

  const bool any = GetSelectedCount() > 0;
  select_button->SetCaption(any
                             ? C_("Button", "Select none")
                             : C_("Button", "Select all"));
  select_button->SetEnabled(!items.empty());
}

void
DownloadFilePickerWidget::CreateButtons()
{
  download_button = dialog.AddButton(_("Download"), [this](){ Download(); });
  select_button = dialog.AddButton(C_("Button", "Select all"), [this](){
    SetAllSelected(GetSelectedCount() == 0);
  });

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
      row_renderer.DrawFirstRow(canvas, rc,
                                _("No file matches the filter."));
    return;
  }

  const auto &file = items[i];
  const bool is_selected = i < selected.size() && selected[i];

  const unsigned padding = Layout::GetTextPadding();
  PixelRect text_rc = rc;
  const unsigned box_size = rc.GetHeight() > 2 * padding
    ? rc.GetHeight() - 2 * padding : 0;
  PixelRect box_rc;
  box_rc.left = rc.left + (int)padding;
  box_rc.top = rc.top + (int)padding;
  box_rc.right = box_rc.left + (int)box_size;
  box_rc.bottom = box_rc.top + (int)box_size;

  const bool focused = !HasCursorKeys() ||
                       (list != nullptr && list->HasFocus());
  DrawCheckBox(canvas, UIGlobals::GetDialogLook(), box_rc,
               is_selected, focused, false, true);
  text_rc.left = box_rc.right + 2 * (int)padding;

  row_renderer.DrawFirstRow(canvas, text_rc, file.GetName());
  const char *description = file.GetDescription();
  if (description != nullptr && description[0] != '\0')
    row_renderer.DrawSecondRow(canvas, text_rc, description);
}

/**
 * Local path the repository file is stored under, relative to the
 * data directory.
 */
static AllocatedPath
RelativeDownloadPath(FileType file_type, const AvailableFile &file)
{
  const Path file_path(file.GetName());
  if (!file_path.IsValidFilename())
    throw std::runtime_error("Invalid download filename");

  AllocatedPath dest_dir = GetFileTypeDefaultDir(file_type);
  if (dest_dir == nullptr)
    return file_path;

  const auto dest_path = LocalPath(dest_dir);
  Directory::CreateRecursive(dest_path);
  if (!Directory::Exists(dest_path))
    throw std::runtime_error("Directory does not exist and could not be "
                             "created.");

  return AllocatedPath::Build(Path(dest_dir), file_path);
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

  /* copy first: a download modal can refresh the repository and
     replace #items */
  struct Pending {
    std::string uri;
    AllocatedPath relative;
  };

  std::vector<Pending> pending;
  const bool any_selected = GetSelectedCount() > 0;
  const unsigned cursor = list != nullptr ? list->GetCursorIndex() : 0;
  for (unsigned i = 0; i < items.size(); ++i) {
    if (any_selected && (i >= selected.size() || !selected[i]))
      continue;
    if (!any_selected && i != cursor)
      continue;

    try {
      pending.push_back({items[i].uri,
                         RelativeDownloadPath(file_type, items[i])});
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  }

  std::vector<AllocatedPath> downloaded;
  for (const auto &file : pending) {
    try {
      auto path = DownloadFileModal(_("Download"), file.uri.c_str(),
                                    file.relative.c_str());
      if (path == nullptr)
        break;

      downloaded.push_back(std::move(path));
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  }

  if (downloaded.empty())
    return;

  paths = std::move(downloaded);
  dialog.SetModalResult(mrOK);
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

std::vector<AllocatedPath>
DownloadFilePicker(FileType file_type)
{
  if (!Net::DownloadManager::IsAvailable()) {
    const char *message =
      _("The file manager is not available on this device.");
    ShowMessageBox(message, _("File Manager"), MB_OK);
    return {};
  }

  TWidgetDialog<DownloadFilePickerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Download"));
  dialog.SetWidget(dialog, file_type);
  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Close"), mrCancel);
  /* No EnableCursorSelection: Left/Right page the list (ListControl).
     Up/Down walk list ↔ buttons. Enter or a tap ticks the row.
     Download fetches the ticked files, or the highlighted file when
     none are ticked. */
  dialog.ShowModal();

  return dialog.GetWidget().GetPaths();
}
