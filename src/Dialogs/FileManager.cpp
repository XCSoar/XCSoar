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
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "DateTime.hpp"
#include "Net/DownloadManager.hpp"
#include "Util/ConvertString.hpp"

#include <set>

#include <assert.h> /* for MAX_PATH */
#include <windef.h> /* for MAX_PATH */

struct RemoteFile {
  const char *uri;

  gcc_pure
  const char *GetName() const {
    assert(uri != NULL);
    const char *slash = strrchr(uri, '/');
    assert(slash != NULL);
    assert(slash[1] != 0);
    return slash + 1;
  }

  bool LocalPath(TCHAR *buffer) const {
    const char *base_narrow = GetName();
    ACPToWideConverter base(base_narrow);
    if (!base.IsValid())
      return false;

    ::LocalPath(buffer, base);
    return true;
  }
};

/**
 * List of downloadable files.
 *
 * TODO: make the list dynamic, not hard-coded.
 */
static gcc_constexpr_data RemoteFile remote_files[] = {
  { "http://www.flarmnet.org/files/data.fln",
  },
  { "http://download.xcsoar.org/waypoints/France.cup",
  },
  { "http://download.xcsoar.org/waypoints/Germany.cup",
  },
  { "http://download.xcsoar.org/waypoints/Netherlands.cup",
  },
  { "http://download.xcsoar.org/waypoints/Poland.cup",
  },
  { "http://www.daec.de/fileadmin/user_upload/files/2012/service/luftraumdaten/120312OpenAir.txt",
  },
  { "http://download.xcsoar.org/maps/ALPS.xcm",
  },
  { "http://download.xcsoar.org/maps/FRA_FULL.xcm",
  },
  { "http://download.xcsoar.org/maps/GER.xcm",
  },
  { "http://download.xcsoar.org/maps/POL.xcm",
  },
  { NULL }
};

static gcc_constexpr_data size_t N_REMOTE_FILES = ARRAY_SIZE(remote_files) - 1;

gcc_pure
static int
FindRemoteFile(const char *name)
{
  for (unsigned i = 0; remote_files[i].uri != NULL; ++i) {
    const char *base = remote_files[i].GetName();
    if (StringIsEqual(base, name))
      return i;
  }

  return -1;
}

#ifdef _UNICODE
gcc_pure
static int
FindRemoteFile(const TCHAR *name)
{
  WideToACPConverter name2(name);
  if (!name2.IsValid())
    return -1;

  return FindRemoteFile(name2);
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

  /**
   * This mutex protects the attribute "downloads".
   */
  mutable Mutex mutex;

  /**
   * The list of file names (base names) that are currently being
   * downloaded.
   */
  std::set<std::string> downloads;

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
  int FindItem(const TCHAR *name) const;

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
  RefreshList();
  UpdateButtons();

#ifdef HAVE_DOWNLOAD_MANAGER
  Net::DownloadManager::AddListener(*this);
#endif
}

void
ManagedFileListWidget::Unprepare()
{
#ifdef HAVE_DOWNLOAD_MANAGER
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
ManagedFileListWidget::RefreshList()
{
  items.clear();

  for (unsigned i = 0; remote_files[i].uri != NULL; ++i) {
    const bool is_downloading = IsDownloading(remote_files[i].GetName());

    TCHAR path[MAX_PATH];
    if (remote_files[i].LocalPath(path) &&
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
                              FindRemoteFile(items[current].name) >= 0);
  add_button->SetEnabled(Net::DownloadManager::IsAvailable() &&
                         items.size() < N_REMOTE_FILES);
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
  const int remote_index = FindRemoteFile(item.name);
  if (remote_index < 0)
    return;

  const RemoteFile &remote_file = remote_files[remote_index];
  ACPToWideConverter base(remote_file.GetName());
  if (!base.IsValid())
    return;

  Net::DownloadManager::Enqueue(remote_file.uri, base);

  mutex.Lock();
  downloads.insert(remote_file.GetName());
  mutex.Unlock();

  RefreshList();
#endif
}

#ifdef HAVE_DOWNLOAD_MANAGER

static TrivialArray<unsigned, N_REMOTE_FILES> add_list;

static void
OnPaintAddItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < add_list.size());
  assert(add_list[i] < N_REMOTE_FILES);

  const RemoteFile &file = remote_files[add_list[i]];

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

  add_list.clear();
  for (unsigned i = 0; remote_files[i].uri != NULL; ++i) {
    if (IsDownloading(remote_files[i].GetName()))
      /* already downloading this file */
      continue;

    ACPToWideConverter name(remote_files[i].GetName());
    if (!name.IsValid())
      continue;

    if (FindItem(name) < 0)
      add_list.append(i);
  }

  if (add_list.empty())
    return;

  int i = ListPicker(UIGlobals::GetMainWindow(), _("Select a file"),
                     add_list.size(), 0, Layout::FastScale(18),
                     OnPaintAddItem);
  if (i < 0)
    return;

  const unsigned remote_index = add_list[i];

  assert((unsigned)i < add_list.size());
  assert(remote_index < N_REMOTE_FILES);

  const RemoteFile &remote_file = remote_files[remote_index];
  ACPToWideConverter base(remote_file.GetName());
  if (!base.IsValid())
    return;

  Net::DownloadManager::Enqueue(remote_file.uri, base);

  mutex.Lock();
  downloads.insert(remote_file.GetName());
  mutex.Unlock();

  RefreshList();
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
ManagedFileListWidget::OnDownloadComplete(const TCHAR *path_relative,
                                          bool success)
{
  const TCHAR *name = BaseName(path_relative);
  if (name != NULL) {
    WideToACPConverter name2(name);
    if (name2.IsValid()) {
      ScopeLock protect(mutex);
      downloads.erase((const char *)name2);
    }
  }

  SendNotification();
}

void
ManagedFileListWidget::OnNotification()
{
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
