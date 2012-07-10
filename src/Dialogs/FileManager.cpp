/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FileManager.hpp"
#include "WidgetDialog.hpp"
#include "ListPicker.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/List.hpp"
#include "Form/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Thread/Notify.hpp"
#include "Thread/Mutex.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "IO/FileLineReader.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "DateTime.hpp"
#include "Net/DownloadManager.hpp"
#include "Util/ConvertString.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Parser.hpp"

#include <set>
#include <vector>

#include <assert.h>
#include <windef.h> /* for MAX_PATH */

#define REPOSITORY_URI "http://download.xcsoar.org/repository"

static bool
LocalPath(TCHAR *buffer, const AvailableFile &file)
{
  ACPToWideConverter base(file.GetName());
  if (!base.IsValid())
    return false;

  ::LocalPath(buffer, base);
  return true;
}

gcc_pure
static const AvailableFile *
FindRemoteFile(const FileRepository &repository, const char *name)
{
  return repository.FindByName(name);
}

#ifdef _UNICODE
gcc_pure
static const AvailableFile *
FindRemoteFile(const FileRepository &repository, const TCHAR *name)
{
  WideToACPConverter name2(name);
  if (!name2.IsValid())
    return NULL;

  return FindRemoteFile(repository, name2);
}
#endif

struct DownloadItem {
  StaticString<64u> name;
};

struct FileItem {
  StaticString<64u> name;
  StaticString<32u> size;
  StaticString<32u> last_modified;

  bool downloading;

  void Set(const TCHAR *_name, bool _downloading) {
    name = _name;

    TCHAR path[MAX_PATH];
    LocalPath(path, name);

    if (File::Exists(path)) {
      FormatByteSize(size.buffer(), size.MAX_SIZE,
                     File::GetSize(path));
#ifdef HAVE_POSIX
      FormatISO8601(last_modified.buffer(),
                    BrokenDateTime::FromUnixTimeUTC(File::GetLastModification(path)));
#else
      // XXX implement
      last_modified.clear();
#endif
    } else {
      size.clear();
      last_modified.clear();
    }

    downloading = _downloading;
  }
};

class ManagedFileListWidget
  : public ListWidget, private ActionListener,
    private Net::DownloadListener, private Notify {
  enum Buttons {
    DOWNLOAD,
    ADD,
  };

  UPixelScalar font_height;

  WndButton *download_button, *add_button;

  FileRepository repository;

  /**
   * This mutex protects the attributes "downloads" and
   * "repository_modified".
   */
  mutable Mutex mutex;

  /**
   * The list of file names (base names) that are currently being
   * downloaded.
   */
  std::set<std::string> downloads;

  /**
   * Was the repository file modified, and needs to be reloaded by
   * LoadRepositoryFile()?
   */
  bool repository_modified;

  TrivialArray<FileItem, 64u> items;

public:
  void CreateButtons(WidgetDialog &dialog);

protected:
  gcc_pure
  bool IsDownloading(const char *name) const {
    ScopeLock protect(mutex);
    return std::find(downloads.begin(), downloads.end(),
                     name) != downloads.end();
  }

  gcc_pure
  bool IsDownloading(const AvailableFile &file) const {
    return IsDownloading(file.GetName());
  }

  gcc_pure
  int FindItem(const TCHAR *name) const;

  void LoadRepositoryFile();
  void RefreshList();
  void UpdateButtons();

  void Download();
  void Add();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();

  /* virtual methods from class List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx);
  virtual void OnCursorMoved(unsigned index);

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id);

  /* virtual methods from class Net::DownloadListener */
  virtual void OnDownloadAdded(const TCHAR *path_relative);
  virtual void OnDownloadComplete(const TCHAR *path_relative, bool success);

  /* virtual methods from class Notify */
  virtual void OnNotification();
};

void
ManagedFileListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  UPixelScalar margin = Layout::Scale(2);
  font_height = look.list.font->GetHeight();

  UPixelScalar row_height = std::max(UPixelScalar(3 * margin + 2 * font_height),
                                     Layout::GetMaximumControlHeight());
  CreateList(parent, look, rc, row_height);
  LoadRepositoryFile();
  RefreshList();
  UpdateButtons();

#ifdef HAVE_DOWNLOAD_MANAGER
  if (Net::DownloadManager::IsAvailable()) {
    Net::DownloadManager::AddListener(*this);
    Net::DownloadManager::Enumerate(*this);

    Net::DownloadManager::Enqueue(REPOSITORY_URI, _T("repository"));
  }
#endif
}

void
ManagedFileListWidget::Unprepare()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  if (Net::DownloadManager::IsAvailable())
    Net::DownloadManager::RemoveListener(*this);

  ClearNotification();
#endif

  DeleteWindow();
}

int
ManagedFileListWidget::FindItem(const TCHAR *name) const
{
  for (auto i = items.begin(), end = items.end(); i != end; ++i)
    if (StringIsEqual(i->name, name))
      return std::distance(items.begin(), i);

  return -1;
}

void
ManagedFileListWidget::LoadRepositoryFile()
{
  mutex.Lock();
  repository_modified = false;
  mutex.Unlock();

  repository.Clear();

  TCHAR path[MAX_PATH];
  LocalPath(path, _T("repository"));
  FileLineReaderA reader(path);
  if (reader.error())
    return;

  ParseFileRepository(repository, reader);
}

void
ManagedFileListWidget::RefreshList()
{
  items.clear();

  for (auto i = repository.begin(), end = repository.end(); i != end; ++i) {
    const auto &remote_file = *i;
    const bool is_downloading = IsDownloading(remote_file);

    TCHAR path[MAX_PATH];
    if (LocalPath(path, remote_file) &&
        (is_downloading || File::Exists(path)))
      items.append().Set(BaseName(path), is_downloading);
  }

  ListControl &list = GetList();
  list.SetLength(items.size());
  list.Invalidate();
}

void
ManagedFileListWidget::CreateButtons(WidgetDialog &dialog)
{
  download_button = dialog.AddButton(_("Download"), this, DOWNLOAD);
  add_button = dialog.AddButton(_("Add"), this, ADD);
}

void
ManagedFileListWidget::UpdateButtons()
{
  const unsigned current = GetList().GetCursorIndex();

  download_button->SetEnabled(Net::DownloadManager::IsAvailable() &&
                              !items.empty() &&
                              FindRemoteFile(repository,
                                             items[current].name) != NULL);
  add_button->SetEnabled(Net::DownloadManager::IsAvailable());
}

void
ManagedFileListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i)
{
  const FileItem &file = items[i];

  const UPixelScalar margin = Layout::Scale(2);

  canvas.text(rc.left + margin, rc.top + margin, file.name.c_str());

  if (file.downloading) {
    const TCHAR *text = _("Downloading");
    UPixelScalar width = canvas.CalcTextWidth(text);
    canvas.text(rc.right - width - margin, rc.top + margin, text);
  }

  canvas.text(rc.left + margin, rc.top + 2 * margin + font_height,
              file.size.c_str());

  canvas.text((rc.left + rc.right) / 2, rc.top + 2 * margin + font_height,
              file.last_modified.c_str());
}

void
ManagedFileListWidget::OnCursorMoved(unsigned index)
{
  UpdateButtons();
}

void
ManagedFileListWidget::Download()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  if (!Net::DownloadManager::IsAvailable())
    return;

  if (items.empty())
    return;

  const unsigned current = GetList().GetCursorIndex();
  assert(current < items.size());

  const FileItem &item = items[current];
  const AvailableFile *remote_file_p = FindRemoteFile(repository, item.name);
  if (remote_file_p == NULL)
    return;

  const AvailableFile &remote_file = *remote_file_p;
  ACPToWideConverter base(remote_file.GetName());
  if (!base.IsValid())
    return;

  Net::DownloadManager::Enqueue(remote_file.uri.c_str(), base);
#endif
}

#ifdef HAVE_DOWNLOAD_MANAGER

static const std::vector<AvailableFile> *add_list;

static void
OnPaintAddItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(add_list != NULL);
  assert(i < add_list->size());

  const AvailableFile &file = (*add_list)[i];

  ACPToWideConverter name(file.GetName());
  if (name.IsValid())
    canvas.text(rc.left + Layout::Scale(2), rc.top + Layout::Scale(2), name);
}

#endif

void
ManagedFileListWidget::Add()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  if (!Net::DownloadManager::IsAvailable())
    return;

  if (items.empty())
    return;

  std::vector<AvailableFile> list;
  for (auto i = repository.begin(), end = repository.end(); i != end; ++i) {
    const AvailableFile &remote_file = *i;

    if (IsDownloading(remote_file.GetName()))
      /* already downloading this file */
      continue;

    ACPToWideConverter name(remote_file.GetName());
    if (!name.IsValid())
      continue;

    if (FindItem(name) < 0)
      list.push_back(remote_file);
  }

  if (list.empty())
    return;

  add_list = &list;
  int i = ListPicker(UIGlobals::GetMainWindow(), _("Select a file"),
                     list.size(), 0, Layout::FastScale(18),
                     OnPaintAddItem);
  add_list = NULL;
  if (i < 0)
    return;

  assert((unsigned)i < list.size());

  const AvailableFile &remote_file = list[i];
  ACPToWideConverter base(remote_file.GetName());
  if (!base.IsValid())
    return;

  Net::DownloadManager::Enqueue(remote_file.GetURI(), base);
#endif
}

void
ManagedFileListWidget::OnAction(int id)
{
  switch (id) {
  case DOWNLOAD:
    Download();
    break;

  case ADD:
    Add();
    break;
  }
}

void
ManagedFileListWidget::OnDownloadAdded(const TCHAR *path_relative)
{
  const TCHAR *name = BaseName(path_relative);
  if (name == NULL)
    return;

  WideToACPConverter name2(name);
  if (!name2.IsValid())
    return;

  mutex.Lock();
  downloads.insert((const char *)name2);
  mutex.Unlock();

  SendNotification();
}

void
ManagedFileListWidget::OnDownloadComplete(const TCHAR *path_relative,
                                          bool success)
{
  const TCHAR *name = BaseName(path_relative);
  if (name == NULL)
    return;

  WideToACPConverter name2(name);
  if (!name2.IsValid())
    return;

  mutex.Lock();

  downloads.erase((const char *)name2);

  if (StringIsEqual(name2, "repository"))
    repository_modified = true;

  mutex.Unlock();

  SendNotification();
}

void
ManagedFileListWidget::OnNotification()
{
  mutex.Lock();
  bool repository_modified2 = repository_modified;
  mutex.Unlock();

  if (repository_modified2)
    LoadRepositoryFile();

  RefreshList();
}

void
ShowFileManager()
{
  ManagedFileListWidget widget;
  WidgetDialog dialog(_("File Manager"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}
