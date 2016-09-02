/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "PlaneDialogs.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Plane/Plane.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Path.hpp"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Profile/Profile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"

#include <vector>
#include <assert.h>

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class PlaneListWidget final
  : public ListWidget, private ActionListener {

  struct ListItem {
    StaticString<32> name;
    AllocatedPath path;

    ListItem(const TCHAR *_name, Path _path)
      :name(_name), path(_path) {}

    bool operator<(const ListItem &i2) const {
      return StringCollate(name, i2.name) < 0;
    }
  };

  class PlaneFileVisitor: public File::Visitor
  {
    std::vector<ListItem> &list;

  public:
    PlaneFileVisitor(std::vector<ListItem> &_list):list(_list) {}

    void Visit(Path path, Path filename) override {
      list.emplace_back(filename.c_str(), path);
    }
  };

  enum Buttons {
    NEW,
    EDIT,
    DELETE,
    LOAD,
  };

  WndForm *form;
  Button *edit_button, *delete_button, *load_button;

  std::vector<ListItem> list;

  TwoTextRowsRenderer row_renderer;

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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;

protected:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem(gcc_unused unsigned index) const override {
    return true;
  }

  void OnActivateItem(unsigned index) override;

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

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
  load_button = dialog.AddButton(_("Activate"), *this, LOAD);
}

void
PlaneListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
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

  if (Profile::GetPathIsEqual("PlanePath", list[i].path)) {
    StaticString<256> buffer;
    buffer.Format(_T("%s - %s"), list[i].name.c_str(), _("Active"));
    row_renderer.DrawFirstRow(canvas, rc, buffer);
  } else
    row_renderer.DrawFirstRow(canvas, rc, list[i].name);

  row_renderer.DrawSecondRow(canvas, rc, list[i].path.c_str());
}

static bool
LoadFile(Path path)
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  if (!PlaneGlue::ReadFile(settings.plane, path))
    return false;

  Profile::SetPath("PlanePath", path);
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
  bool result = Load(i);
  if (!result) {
    const TCHAR *title = _("Error");
    StaticString<256> text;
    text.Format(_("Activating plane \"%s\" failed."),
                list[i].name.c_str());
    ShowMessageBox(text, title, MB_OK);
  }

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

  while (dlgPlaneDetailsShowModal(plane)) {
    if (plane.registration.empty()) {
      ShowMessageBox(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    const auto path = LocalPath(filename);

    if (File::Exists(path)) {
      StaticString<256> tmp;
      tmp.Format(_("Plane \"%s\" already exists. "
                   "Overwrite it?"),
                   plane.registration.c_str());
      if (ShowMessageBox(tmp, _("Overwrite"), MB_YESNO) != IDYES)
        continue;
    }

    try {
      PlaneGlue::WriteFile(plane, path);
    } catch (const std::runtime_error &e) {
      ShowError(e, _("Failed to save file."));
      return;
    }

    UpdateList();
    break;
  }
}

inline void
PlaneListWidget::EditClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  const unsigned index = GetList().GetCursorIndex();
  const Path old_path = list[index].path;
  const TCHAR *old_filename = list[index].name;

  Plane plane;
  PlaneGlue::ReadFile(plane, old_path);

  while (dlgPlaneDetailsShowModal(plane)) {
    if (plane.registration.empty()) {
      ShowMessageBox(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += _T(".xcp");

    if (filename != old_filename) {
      const auto path = AllocatedPath::Build(old_path.GetParent(),
                                             filename);

      if (File::Exists(path)) {
        StaticString<256> tmp;
        tmp.Format(_("Plane \"%s\" already exists. "
                     "Overwrite it?"),
                     plane.registration.c_str());
        if (ShowMessageBox(tmp, _("Overwrite"), MB_YESNO) != IDYES)
          continue;
      }

      File::Delete(old_path);

      try {
        PlaneGlue::WriteFile(plane, path);
      } catch (const std::runtime_error &e) {
        ShowError(e, _("Failed to save file."));
        return;
      }

      if (Profile::GetPathIsEqual("PlanePath", old_path)) {
        list[index].path = Path(path);
        list[index].name = filename;
        Load(index);
      }
    } else {
      try {
        PlaneGlue::WriteFile(plane, old_path);
      } catch (const std::runtime_error &e) {
        ShowError(e, _("Failed to save file."));
        return;
      }

      if (Profile::GetPathIsEqual("PlanePath", old_path))
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
  StaticString<256> tmp_name(list[GetList().GetCursorIndex()].name.c_str());
  if (tmp_name.length() > 4)
    tmp_name.Truncate(tmp_name.length() - 4);

  tmp.Format(_("Delete plane \"%s\"?"),
             tmp_name.c_str());
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
  tmp.Format(_("Activate plane \"%s\"?"),
             list[i].name.c_str());

  if (ShowMessageBox(tmp, _T(" "), MB_YESNO) == IDYES)
    LoadWithDialog(i);
}

void
dlgPlanesShowModal()
{
  PlaneListWidget widget;
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Planes"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
  dialog.StealWidget();
}
