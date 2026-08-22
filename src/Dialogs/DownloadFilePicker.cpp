// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilePicker.hpp"
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
#include "Widget/ListWidget.hpp"
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
#include "util/StaticString.hxx"
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <cassert>

static const char *
GetCountryName(std::string_view area) noexcept
{
  if (area == "ar") return _("Argentina");
  if (area == "au") return _("Australia");
  if (area == "br") return _("Brazil");
  if (area == "bg") return _("Bulgaria");
  if (area == "ca") return _("Canada");
  if (area == "cl") return _("Chile");
  if (area == "co") return _("Colombia");
  if (area == "cz") return _("Czechia");
  if (area == "dk") return _("Denmark");
  if (area == "fi") return _("Finland");
  if (area == "fr") return _("France");
  if (area == "de") return _("Germany");
  if (area == "hu") return _("Hungary");
  if (area == "ie") return _("Ireland");
  if (area == "il") return _("Israel");
  if (area == "it") return _("Italy");
  if (area == "jp") return _("Japan");
  if (area == "mw") return _("Malawi");
  if (area == "mx") return _("Mexico");
  if (area == "na") return _("Namibia");
  if (area == "nz") return _("New Zealand");
  if (area == "no") return _("Norway");
  if (area == "pl") return _("Poland");
  if (area == "pt") return _("Portugal");
  if (area == "za") return _("South Africa");
  if (area == "es") return _("Spain");
  if (area == "se") return _("Sweden");
  if (area == "tr") return _("Turkey");
  if (area == "ua") return _("Ukraine");
  if (area == "gb") return _("United Kingdom");
  if (area == "us") return _("United States");

  return nullptr;
}

class DownloadFilePickerWidget final
  : public ListWidget,
    Net::DownloadListener {

  WidgetDialog &dialog;

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  const FileType file_type;

  unsigned font_height;

  Button *download_button;

  std::vector<AvailableFile> items;

  struct VisibleItem {
    enum class Type {
      GROUP,
      FILE,
    } type;

    std::string group;
    std::string label;
    std::size_t file_index;
  };

  std::vector<VisibleItem> visible_items;
  std::set<std::string> expanded_groups;

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
    :dialog(_dialog), file_type(_file_type) {}

  AllocatedPath &&GetPath() {
    return std::move(path);
  }

  void CreateButtons();

protected:
  void RefreshList();
  void RebuildVisibleItems();
  void RefreshRepository() noexcept;

  void UpdateButtons();

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

  void OnActivateItem(unsigned index) noexcept override {
    if (items.empty()) {
      assert(index == 0);
      RefreshRepository();
    } else if (visible_items[index].type == VisibleItem::Type::GROUP) {
      const auto &group = visible_items[index].group;
      if (!expanded_groups.erase(group))
        expanded_groups.insert(group);

      RebuildVisibleItems();
    } else
      Download();
  }

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
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
DownloadFilePickerWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  unsigned row_height = row_renderer.CalculateLayout(*look.list.font);
  if (items.empty())
    row_height = LayoutEmptyDownloadRow(row_renderer);

  CreateList(parent, look, rc, row_height);
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

  items.clear();
  for (auto &i : repository)
    if (i.type == file_type)
      items.emplace_back(std::move(i));

  RebuildVisibleItems();
}

void
DownloadFilePickerWidget::RebuildVisibleItems()
{
  visible_items.clear();

  if (file_type == FileType::MAP) {
    std::map<std::string, std::vector<std::size_t>> groups;
    std::vector<std::size_t> regions;

    for (std::size_t i = 0; i < items.size(); ++i) {
      const char *area = items[i].GetArea();
      if (area != nullptr && *area != '\0')
        groups[area].push_back(i);
      else
        regions.push_back(i);
    }

    for (const auto &[group, files] : groups) {
      const char *name = GetCountryName(group);
      visible_items.push_back({VisibleItem::Type::GROUP, group,
                               name != nullptr ? name : group, 0});
      if (expanded_groups.contains(group))
        for (const auto file_index : files)
          visible_items.push_back({VisibleItem::Type::FILE, {}, {}, file_index});
    }

    if (!regions.empty()) {
      const std::string group = _("Regions");
      visible_items.push_back({VisibleItem::Type::GROUP, group, group, 0});
      if (expanded_groups.contains(group))
        for (const auto file_index : regions)
          visible_items.push_back({VisibleItem::Type::FILE, {}, {}, file_index});
    }
  } else {
    for (std::size_t i = 0; i < items.size(); ++i)
      visible_items.push_back({VisibleItem::Type::FILE, {}, {}, i});
  }

  ListControl &list = GetList();
  list.SetLength(std::max(visible_items.size(), size_t{1}));
  list.Invalidate();

  UpdateButtons();
}

void
DownloadFilePickerWidget::UpdateButtons()
{
  bool enabled = items.empty();
  if (IsDefined() && !items.empty() && !visible_items.empty()) {
    const unsigned index = GetList().GetCursorIndex();
    enabled = index < visible_items.size() &&
      visible_items[index].type == VisibleItem::Type::FILE;
  }

  download_button->SetEnabled(enabled);
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
    DrawEmptyDownloadHint(row_renderer, canvas, rc);
    return;
  }

  const auto &item = visible_items[i];
  if (item.type == VisibleItem::Type::GROUP) {
    StaticString<64> text;
    text.Format("%s %s", expanded_groups.contains(item.group) ? "▼" : "▶",
                item.label.c_str());
    row_renderer.DrawTextRow(canvas, rc, text);
  } else {
    StaticString<256> text;
    text.Format("    %s", items[item.file_index].GetName());
    row_renderer.DrawTextRow(canvas, rc, text);
  }
}

void
DownloadFilePickerWidget::Download()
{
  assert(Net::DownloadManager::IsAvailable());

  if (items.empty()) {
    RefreshRepository();
    return;
  }

  const unsigned current = GetList().GetCursorIndex();
  assert(current < visible_items.size());

  const auto &item = visible_items[current];
  if (item.type != VisibleItem::Type::FILE)
    return;

  const auto &file = items[item.file_index];

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
  dialog.AddButton(_("Cancel"), mrCancel);
  /* No EnableCursorSelection: Left/Right page the list (ListControl).
     Up/Down walk list ↔ Download/Cancel; Enter downloads the cursor row. */
  dialog.ShowModal();

  return dialog.GetWidget().GetPath();
}
