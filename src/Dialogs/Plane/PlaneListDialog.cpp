// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "Profile/Profile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "Components.hpp"
#include "BackendComponents.hpp"

#include <vector>
#include <cassert>

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class PlaneListWidget final
  : public ListWidget {

  struct ListItem {
    StaticString<32> name;
    AllocatedPath path;

    ListItem(tstring_view _name, Path _path) noexcept
      :name(_name), path(_path) {}

    bool operator<(const ListItem &i2) const noexcept {
      return StringCollate(name, i2.name) < 0;
    }
  };

  class PlaneFileVisitor: public File::Visitor
  {
    std::vector<ListItem> &list;

  public:
    PlaneFileVisitor(std::vector<ListItem> &_list) noexcept:list(_list) {}

    void Visit(Path path, Path filename) override {
      tstring_view name{filename.c_str()};
      RemoveSuffix(name, tstring_view{_T(".xcp")});

      list.emplace_back(name, path);
    }
  };

  WndForm *form;
  Button *edit_button, *copy_button, *delete_button, *load_button;

  std::vector<ListItem> list;

  TwoTextRowsRenderer row_renderer;

public:
  void CreateButtons(WidgetDialog &dialog) noexcept;

private:
  void UpdateList() noexcept;
  bool Load(unsigned i) noexcept;
  bool LoadWithDialog(unsigned i) noexcept;

  void LoadClicked() noexcept;
  void NewClicked() noexcept;
  void EditClicked(bool copy) noexcept;
  void DeleteClicked() noexcept;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

protected:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
PlaneListWidget::UpdateList() noexcept
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
  copy_button->SetEnabled(!empty);
  delete_button->SetEnabled(!empty);
}

void
PlaneListWidget::CreateButtons(WidgetDialog &dialog) noexcept
{
  form = &dialog;

  dialog.AddButton(_("New"), [this](){ NewClicked(); });
  edit_button = dialog.AddButton(_("Edit"), [this](){ EditClicked(false); });
  copy_button = dialog.AddButton(_("Copy"), [this](){ EditClicked(true); });
  delete_button = dialog.AddButton(_("Delete"), [this](){ DeleteClicked(); });
  load_button = dialog.AddButton(_("Activate"), [this](){ LoadClicked(); });
}

void
PlaneListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
  UpdateList();
}

void
PlaneListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                             unsigned i) noexcept
{
  assert(i < list.size());

  if (Profile::GetPathIsEqual("PlanePath", list[i].path)) {
    StaticString<256> buffer;
    buffer.Format(_T("%s - %s"), list[i].name.c_str(), _("Active"));
    row_renderer.DrawFirstRow(canvas, rc, buffer);
  } else
    row_renderer.DrawFirstRow(canvas, rc, list[i].name);

  Path path = list[i].path;
  if (auto relative_path = RelativePath(path); relative_path != nullptr)
    path = relative_path;

  row_renderer.DrawSecondRow(canvas, rc, path.c_str());
}

static bool
LoadFile(Path path) noexcept
{
  ComputerSettings &settings = CommonInterface::SetComputerSettings();

  if (!PlaneGlue::ReadFile(settings.plane, path))
    return false;

  Profile::SetPath("PlanePath", path);
  PlaneGlue::Synchronize(settings.plane, settings,
                         settings.polar.glide_polar_task);
  backend_components->SetTaskPolar(settings.polar);

  return true;
}

bool
PlaneListWidget::Load(unsigned i) noexcept
{
  assert(i < list.size());

  return LoadFile(list[i].path);
}

bool
PlaneListWidget::LoadWithDialog(unsigned i) noexcept
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
PlaneListWidget::LoadClicked() noexcept
{
  if (LoadWithDialog(GetList().GetCursorIndex()))
    form->SetModalResult(mrOK);
}

inline void
PlaneListWidget::NewClicked() noexcept
{
  Plane plane{};

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
    } catch (...) {
      ShowError(std::current_exception(), _("Failed to save file."));
      return;
    }

    UpdateList();
    break;
  }
}

void
PlaneListWidget::EditClicked(bool copy) noexcept
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

    if (copy || filename != old_filename) {
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

      if (!copy)
        File::Delete(old_path);

      try {
        PlaneGlue::WriteFile(plane, path);
      } catch (...) {
        ShowError(std::current_exception(), _("Failed to save file."));
        return;
      }

      if (!copy && Profile::GetPathIsEqual("PlanePath", old_path)) {
        list[index].path = Path(path);
        list[index].name = filename;
        Load(index);
      }
    } else {
      try {
        PlaneGlue::WriteFile(plane, old_path);
      } catch (...) {
        ShowError(std::current_exception(), _("Failed to save file."));
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
PlaneListWidget::DeleteClicked() noexcept
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
PlaneListWidget::OnActivateItem(unsigned i) noexcept
{
  assert(i < list.size());

  StaticString<256> tmp;
  tmp.Format(_("Activate plane \"%s\"?"),
             list[i].name.c_str());

  if (ShowMessageBox(tmp, _T(" "), MB_YESNO) == IDYES)
    LoadWithDialog(i);
}

void
dlgPlanesShowModal() noexcept
{
  TWidgetDialog<PlaneListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _("Planes"));
  dialog.SetWidget();
  dialog.GetWidget().CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}
