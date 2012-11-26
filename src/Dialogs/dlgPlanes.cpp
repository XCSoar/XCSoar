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

#include "Dialogs/Planes.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Plane/Plane.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Profile/Profile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <vector>
#include <assert.h>
#include <windef.h> /* for MAX_PATH */

struct ListItem
{
  StaticString<32> name;
  StaticString<MAX_PATH> path;

  bool operator<(const ListItem &i2) const {
    return _tcscmp(name, i2.name) < 0;
  }
};

class PlaneFileVisitor: public File::Visitor
{
  std::vector<ListItem> &list;

public:
  PlaneFileVisitor(std::vector<ListItem> &_list):list(_list) {}

  void Visit(const TCHAR* path, const TCHAR* filename) {
    ListItem item;
    item.name = filename;
    item.path = path;
    list.push_back(item);
  }
};

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class PlaneListWidget : public ListWidget, private ActionListener {
  enum Buttons {
    NEW,
    EDIT,
    DELETE,
    LOAD,
  };

  WndForm *form;
  WndButton *edit_button, *delete_button, *load_button;

  std::vector<ListItem> list;

public:
  void CreateButtons(WidgetDialog &dialog);

private:
  void UpdateList();
  bool Load(unsigned i);
  bool LoadWithDialog(unsigned i);

  void LoadClicked();
  void NewClicked();
  void EditClicked();
  void DeleteClicked();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
  virtual void Unprepare() gcc_override;

protected:
  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) gcc_override;

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const gcc_override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) gcc_override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id);
};

gcc_pure
static UPixelScalar
GetRowHeight(const DialogLook &look)
{
  return look.list.font->GetHeight() + Layout::Scale(6)
    + look.small_font->GetHeight();
}

void
PlaneListWidget::UpdateList()
{
  list.clear();

  PlaneFileVisitor pfv(list);
  VisitDataFiles(_T("*.xcp"), pfv);

  unsigned len = list.size();

  if (len > 0)
    std::sort(list.begin(), list.end());

  ListControl &list_control = GetList();
  list_control.SetLength(len);
  list_control.Invalidate();

  const bool empty = list.empty();
  load_button->SetEnabled(!empty);
  edit_button->SetEnabled(!empty);
  delete_button->SetEnabled(!empty);
}

void
PlaneListWidget::CreateButtons(WidgetDialog &dialog)
{
  form = &dialog;

  dialog.AddButton(_("New"), *this, NEW);
  edit_button = dialog.AddButton(_("Edit"), *this, EDIT);
  delete_button = dialog.AddButton(_("Delete"), *this, DELETE);
  load_button = dialog.AddButton(_("Load"), *this, LOAD);
}

void
PlaneListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, GetRowHeight(look));
  UpdateList();
}

void
PlaneListWidget::Unprepare()
{
  DeleteWindow();
}

void
PlaneListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < list.size());

  const DialogLook &look = UIGlobals::GetDialogLook();
  const Font &name_font = *look.list.font;
  const Font &details_font = *look.small_font;

  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(name_font);

  if (Profile::GetPathIsEqual(_T("PlanePath"), list[i].path)) {
    StaticString<256> buffer;
    buffer.Format(_T("%s - %s"), list[i].name.c_str(), _("Active"));
    canvas.DrawClippedText(rc.left + Layout::FastScale(2),
                           rc.top + Layout::FastScale(2), rc, buffer);
  } else
    canvas.DrawClippedText(rc.left + Layout::FastScale(2),
                           rc.top + Layout::FastScale(2), rc, list[i].name);

  canvas.Select(details_font);

  canvas.DrawClippedText(rc.left + Layout::FastScale(2),
                         rc.top + name_font.GetHeight() + Layout::FastScale(4),
                         rc, list[i].path);
}

static bool
LoadFile(const TCHAR *path)
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  if (!PlaneGlue::ReadFile(settings.plane, path))
    return false;

  Profile::SetPath(_T("PlanePath"), path);
  PlaneGlue::Synchronize(settings.plane, settings,
                         settings.polar.glide_polar_task);
  if (protected_task_manager != NULL)
    protected_task_manager->SetGlidePolar(settings.polar.glide_polar_task);

  return true;
}

bool
PlaneListWidget::Load(unsigned i)
{
  assert(i < list.size());

  return LoadFile(list[i].path);
}

bool
PlaneListWidget::LoadWithDialog(unsigned i)
{
  const TCHAR *title;
  StaticString<256> text;

  bool result = Load(i);
  if (!result) {
    title = _("Error");
    text.Format(_("Loading of plane profile \"%s\" failed!"),
                list[i].name.c_str());
  } else {
    title = _("Load");
    text.Format(_("Plane profile \"%s\" activated."),
                list[i].name.c_str());
  }

  ShowMessageBox(text, title, MB_OK);

  return result;
}

inline void
PlaneListWidget::LoadClicked()
{
  if (LoadWithDialog(GetList().GetCursorIndex()))
    form->SetModalResult(mrOK);
}

inline void
PlaneListWidget::NewClicked()
{
  Plane plane = CommonInterface::GetComputerSettings().plane;

  while (dlgPlaneDetailsShowModal(UIGlobals::GetMainWindow(), plane)) {
    if (plane.registration.empty()) {
      ShowMessageBox(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    StaticString<MAX_PATH> path;
    LocalPath(path.buffer(), filename);

    if (File::Exists(path)) {
      StaticString<256> tmp;
      tmp.Format(_("A plane profile \"%s\" already exists. "
                   "Do you want to overwrite it?"),
                   filename.c_str());
      if (ShowMessageBox(tmp, _("Overwrite"), MB_YESNO) != IDYES)
        continue;
    }

    PlaneGlue::WriteFile(plane, path);
    UpdateList();
    break;
  }
}

inline void
PlaneListWidget::EditClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  const unsigned index = GetList().GetCursorIndex();
  const TCHAR *old_path = list[index].path;
  const TCHAR *old_filename = list[index].name;

  Plane plane;
  PlaneGlue::ReadFile(plane, old_path);

  while (dlgPlaneDetailsShowModal(UIGlobals::GetMainWindow(), plane)) {
    if (plane.registration.empty()) {
      ShowMessageBox(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    if (filename != old_filename) {
      StaticString<MAX_PATH> path;
      DirName(old_path, path.buffer());
      path += _T(DIR_SEPARATOR_S);
      path += filename;

      if (File::Exists(path)) {
        StaticString<256> tmp;
        tmp.Format(_("A plane profile \"%s\" already exists. "
                     "Do you want to overwrite it?"),
                     filename.c_str());
        if (ShowMessageBox(tmp, _("Overwrite"), MB_YESNO) != IDYES)
          continue;
      }

      File::Delete(old_path);
      PlaneGlue::WriteFile(plane, path);
      if (Profile::GetPathIsEqual(_T("PlanePath"), old_path)) {
        list[index].path = path;
        list[index].name = filename;
        Load(index);
      }
    } else {
      PlaneGlue::WriteFile(plane, old_path);
      if (Profile::GetPathIsEqual(_T("PlanePath"), old_path))
        Load(index);
    }

    UpdateList();
    break;
  }
}

inline void
PlaneListWidget::DeleteClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  StaticString<256> tmp;
  tmp.Format(_("Do you really want to delete plane profile \"%s\"?"),
             list[GetList().GetCursorIndex()].name.c_str());
  if (ShowMessageBox(tmp, _("Delete"), MB_YESNO) != IDYES)
    return;

  File::Delete(list[GetList().GetCursorIndex()].path);
  UpdateList();
}

void
PlaneListWidget::OnAction(int id)
{
  switch ((Buttons)id) {
  case NEW:
    NewClicked();
    break;

  case EDIT:
    EditClicked();
    break;

  case DELETE:
    DeleteClicked();
    break;

  case LOAD:
    LoadClicked();
    break;
  }
}

void
PlaneListWidget::OnActivateItem(unsigned i)
{
  assert(i < list.size());

  StaticString<256> tmp;
  tmp.Format(_("Do you want to load plane profile \"%s\"?"),
             list[i].name.c_str());

  if (ShowMessageBox(tmp, _("Load"), MB_YESNO) == IDYES)
    if (LoadWithDialog(i))
      form->SetModalResult(mrOK);
}

void
dlgPlanesShowModal()
{
  PlaneListWidget widget;
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Planes"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}
