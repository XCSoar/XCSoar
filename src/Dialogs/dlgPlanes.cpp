/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/Planes.hpp"
#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Plane/Plane.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "LocalPath.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "Profile/Profile.hpp"
#include "Task/ProtectedTaskManager.hpp"

#include <vector>
#include <assert.h>

static WndForm *dialog = NULL;
static WndListFrame *plane_list = NULL;

struct ListItem
{
  StaticString<32> name;
  StaticString<MAX_PATH> path;

  bool operator<(const ListItem &i2) const {
    return _tcscmp(name, i2.name) == -1;
  }
};

static std::vector<ListItem> list;

class PlaneFileVisitor: public File::Visitor
{
  void Visit(const TCHAR* path, const TCHAR* filename) {
    ListItem item;
    item.name = filename;
    item.path = path;
    list.push_back(item);
  }
};

static void
UpdateList()
{
  list.clear();

  PlaneFileVisitor pfv;
  VisitDataFiles(_T("*.xcp"), pfv);

  unsigned len = list.size();

  if (len > 0)
    std::sort(list.begin(), list.end());

  plane_list->SetLength(len);
  plane_list->invalidate();

  WndButton* b = (WndButton*)dialog->FindByName(_T("LoadButton"));
  assert(b != NULL);
  b->set_enabled(len > 0);

  b = (WndButton*)dialog->FindByName(_T("EditButton"));
  assert(b != NULL);
  b->set_enabled(len > 0);

  b = (WndButton*)dialog->FindByName(_T("DeleteButton"));
  assert(b != NULL);
  b->set_enabled(len > 0);
}

static void
OnPlaneListPaint(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < list.size());

  const Font &name_font = Fonts::MapBold;
  const Font &details_font = Fonts::MapLabel;

  canvas.select(name_font);

  if (Profile::GetPathIsEqual(_T("PlanePath"), list[i].path)) {
    TCHAR buffer[256];
    _stprintf(buffer, _T("%s - %s"), list[i].name.c_str(), _("Active"));
    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2), rc, buffer);
  } else
    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2), rc, list[i].name);

  canvas.select(details_font);

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, list[i].path);
}

static void
Load(unsigned i)
{
  assert(i < list.size());

  SETTINGS_COMPUTER &settings = CommonInterface::SetSettingsComputer();

  if (!PlaneGlue::ReadFile(settings.plane, list[i].path)) {
    TCHAR tmp[256];
    _stprintf(tmp, _("Loading of plane profile \"%s\" failed!"),
              list[i].name.c_str());
    MessageBoxX(tmp, _("Error"), MB_OK);
    return;
  }

  Profile::SetPath(_T("PlanePath"), list[i].path);
  PlaneGlue::Synchronize(settings.plane, settings, settings.glide_polar_task);
  if (protected_task_manager != NULL)
    protected_task_manager->set_glide_polar(settings.glide_polar_task);

  TCHAR tmp[256];
  _stprintf(tmp, _("Plane profile \"%s\" activated."),
            list[i].name.c_str());
  MessageBoxX(tmp, _("Load"), MB_OK);

  dialog->SetModalResult(mrOK);
}

static void
LoadClicked(gcc_unused WndButton &button)
{
  Load(plane_list->GetCursorIndex());
}

static void
CloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

static void
NewClicked(gcc_unused WndButton &button)
{
  Plane plane = CommonInterface::SettingsComputer().plane;

  while (dlgPlaneDetailsShowModal(*(SingleWindow*)dialog->get_root_owner(), plane)) {
    if (plane.registration.empty()) {
      MessageBoxX(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    StaticString<MAX_PATH> path;
    LocalPath(path.buffer(), filename);

    if (File::Exists(path)) {
      TCHAR tmp[256];
      _stprintf(tmp, _("A plane profile \"%s\" already exists. Do you want to overwrite it?"),
                filename.c_str());
      if (MessageBoxX(tmp, _("Overwrite"), MB_YESNO) != IDYES)
        continue;
    }

    PlaneGlue::WriteFile(plane, path);
    UpdateList();
    break;
  }
}

static void
EditClicked(gcc_unused WndButton &button)
{
  assert(plane_list->GetCursorIndex() < list.size());

  const TCHAR *old_path = list[plane_list->GetCursorIndex()].path;
  const TCHAR *old_filename = list[plane_list->GetCursorIndex()].name;

  Plane plane;
  PlaneGlue::ReadFile(plane, old_path);

  while (dlgPlaneDetailsShowModal(*(SingleWindow*)dialog->get_root_owner(), plane)) {
    if (plane.registration.empty()) {
      MessageBoxX(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    if (filename != old_filename) {
      StaticString<MAX_PATH> path;
      DirName(old_path, path.buffer());
      path += _T("/");
      path += filename;

      if (File::Exists(path)) {
        TCHAR tmp[256];
        _stprintf(tmp, _("A plane profile \"%s\" already exists. Do you want to overwrite it?"),
                  filename.c_str());
        if (MessageBoxX(tmp, _("Overwrite"), MB_YESNO) != IDYES)
          continue;
      }

      File::Delete(old_path);
      PlaneGlue::WriteFile(plane, path);
    } else {
      PlaneGlue::WriteFile(plane, old_path);
    }

    UpdateList();
    break;
  }
}

static void
DeleteClicked(gcc_unused WndButton &button)
{
  assert(plane_list->GetCursorIndex() < list.size());

  TCHAR tmp[256];
  _stprintf(tmp, _("Do you really want to delete plane profile \"%s\"?"),
            list[plane_list->GetCursorIndex()].name.c_str());
  if (MessageBoxX(tmp, _("Delete"), MB_YESNO) != IDYES)
    return;

  File::Delete(list[plane_list->GetCursorIndex()].path);
  UpdateList();
}

static void
ListItemSelected(unsigned i)
{
  assert(i < list.size());

  TCHAR tmp[256];
  _stprintf(tmp, _("Do you want to load plane profile \"%s\"?"),
            list[i].name.c_str());

  if (MessageBoxX(tmp, _("Load"), MB_YESNO) == IDYES)
    Load(i);
}

static CallBackTableEntry CallBackTable[] = {
   DeclareCallBackEntry(LoadClicked),
   DeclareCallBackEntry(CloseClicked),
   DeclareCallBackEntry(NewClicked),
   DeclareCallBackEntry(EditClicked),
   DeclareCallBackEntry(DeleteClicked),
   DeclareCallBackEntry(NULL)
};

void
dlgPlanesShowModal(SingleWindow &parent)
{
  dialog = LoadDialog(CallBackTable, parent,
                      Layout::landscape ?
                      _T("IDR_XML_PLANES_L") : _T("IDR_XML_PLANES"));
  assert(dialog != NULL);

  unsigned item_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                         Fonts::MapLabel.get_height();

  plane_list = (WndListFrame*)dialog->FindByName(_T("List"));
  assert(plane_list != NULL);
  plane_list->SetItemHeight(item_height);
  plane_list->SetPaintItemCallback(OnPlaneListPaint);
  plane_list->SetActivateCallback(ListItemSelected);

  UpdateList();
  dialog->ShowModal();

  delete dialog;
}

