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
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/List.hpp"
#include "Form/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Thread/Notify.hpp"
#include "OS/FileUtil.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "DateTime.hpp"
#include "Net/DownloadManager.hpp"

#include <windef.h> /* for MAX_PATH */

struct FileItem {
  StaticString<64u> name;
  StaticString<32u> size;
  StaticString<32u> last_modified;

  void Set(const TCHAR *_name) {
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
  }
};

class ManagedFileListWidget
  : public ListWidget, private ActionListener,
    private Net::DownloadListener, private Notify {
  enum Buttons {
    DOWNLOAD,
  };

  UPixelScalar font_height;

  WndButton *download_button;

  StaticArray<FileItem, 64u> items;

public:
  void CreateButtons(WidgetDialog &dialog);

protected:
  void RefreshList();
  void UpdateButtons();

  void Download();

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
  virtual void OnDownloadComplete(const char *path_relative, bool success);

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

void
ManagedFileListWidget::RefreshList()
{
  items.clear();
  items.append().Set(_T("data.fln"));
  items.append().Set(_T("GER.xcm"));
  items.append().Set(_T("Germany.cup"));

  ListControl &list = GetList();
  list.SetLength(items.size());
  list.Invalidate();
}

void
ManagedFileListWidget::CreateButtons(WidgetDialog &dialog)
{
  download_button = dialog.AddButton(_("Download"), this, DOWNLOAD);
}

void
ManagedFileListWidget::UpdateButtons()
{
  const unsigned current = GetList().GetCursorIndex();

  download_button->SetEnabled(Net::DownloadManager::IsAvailable() &&
                              current == 0);
}

void
ManagedFileListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                   unsigned i)
{
  const FileItem &file = items[i];

  const UPixelScalar margin = Layout::Scale(2);

  canvas.text(rc.left + margin, rc.top + margin, file.name.c_str());
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

  Net::DownloadManager::Enqueue("http://www.flarmnet.org/files/data.fln",
                                "data.fln");
#endif
}

void
ManagedFileListWidget::OnAction(int id)
{
  switch (id) {
  case DOWNLOAD:
    Download();
    break;
  }
}

void
ManagedFileListWidget::OnDownloadComplete(const char *path_relative,
                                          bool success)
{
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
