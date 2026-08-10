// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlaneDialogs.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Plane/Plane.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "Profile/Profile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
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
    StaticString<6> competition_id;
    StaticString<32> type;
    AllocatedPath path;

    ListItem(std::string_view _name, const char *_competition_id,
             const char *_type, Path _path) noexcept
      :name(_name), competition_id(_competition_id), type(_type),
       path(_path) {}

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
      std::string_view name{filename.c_str()};
      RemoveSuffix(name, std::string_view{".xcp"});

      Plane plane{};
      PlaneGlue::ReadFile(plane, path);
      list.emplace_back(name, plane.competition_id.c_str(),
                        plane.type.c_str(), path);
    }
  };

  WidgetDialog *form;
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

  bool IsMouseActivateHotspot(unsigned index, PixelPoint relative,
                              PixelSize item_size) const noexcept override;

  bool OnMouseActivateItem(unsigned index, PixelPoint relative,
                           PixelSize item_size) noexcept override;
};

void
PlaneListWidget::UpdateList() noexcept
{
  list.clear();

  PlaneFileVisitor pfv(list);
  VisitDataFiles(GetFileTypePatterns(FileType::PLANE), pfv);

  unsigned len = list.size();

  if (len > 0)
    std::sort(list.begin(), list.end());

  ListControl &list_control = GetList();
  list_control.SetLength(len);
  list_control.Invalidate();

  const bool empty = list.empty();
  const bool only_active = list.size() == 1 &&
    Profile::GetPathIsEqual("PlanePath", list[0].path);
  /* Empty, or sole plane already active: Activate is a no-op → arm
     New.  Otherwise prefer Activate (including after New adds a
     plane and Activate becomes usable again). */
  load_button->SetEnabled(!empty && !only_active);
  edit_button->SetEnabled(!empty);
  copy_button->SetEnabled(!empty);
  delete_button->SetEnabled(!empty);

  if (form != nullptr)
    form->SelectFirstEnabledButton();
}

void
PlaneListWidget::CreateButtons(WidgetDialog &dialog) noexcept
{
  form = &dialog;

  /* Activate first so EnableCursorSelection(0) arms it; when the
     list is empty or the sole plane is already active, UpdateList()
     disables Activate and SelectFirstEnabledButton() arms New.
     After a plane is added, Activate is re-enabled and armed again. */
  load_button = dialog.AddButton(_("Activate"), [this](){ LoadClicked(); });
  dialog.AddButton(_("New"), [this](){ NewClicked(); });
  edit_button = dialog.AddButton(_("Edit"), [this](){ EditClicked(false); });
  copy_button = dialog.AddButton(_("Copy"), [this](){ EditClicked(true); });
  delete_button = dialog.AddButton(_("Delete"), [this](){ DeleteClicked(); });
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

  const DialogLook &look = UIGlobals::GetDialogLook();
  const bool active =
    Profile::GetPathIsEqual("PlanePath", list[i].path);

  PixelRect box_rc = GetListRowCheckBoxRect(rc);
  if (box_rc.GetWidth() > 0)
    DrawCheckBox(canvas, look, box_rc, active, false, false, true);

  PixelRect text_rc = rc;
  if (box_rc.GetWidth() > 0)
    text_rc.left = box_rc.right + 2 * (int)Layout::GetTextPadding();

  if (!list[i].type.empty()) {
    StaticString<96> buffer;
    buffer.Format("%s - %s", list[i].name.c_str(), list[i].type.c_str());
    row_renderer.DrawFirstRow(canvas, text_rc, buffer);
  } else
    row_renderer.DrawFirstRow(canvas, text_rc, list[i].name);

  Path path = list[i].path;
  if (auto relative_path = RelativePath(path); relative_path != nullptr)
    path = relative_path;

  row_renderer.DrawSecondRow(canvas, text_rc, path.c_str());

  if (!list[i].competition_id.empty())
    row_renderer.DrawRightFirstRow(canvas, text_rc,
                                   list[i].competition_id.c_str());
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
  Profile::Save();

  const char *label = !settings.plane.polar_name.empty()
    ? settings.plane.polar_name.c_str()
    : settings.plane.registration.c_str();
  if (settings.polar.glide_polar_task.IsValid())
    Message::AddMessage(_("Polar changed"), label);
  else
    Message::AddMessage(_("Invalid Polar"), label);

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
    const char *title = _("Error");
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
    filename += ".xcp";

    const auto path = LocalPath(AllocatedPath::Build(
      GetFileTypeDefaultDir(FileType::PLANE), filename));

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

    /* First plane: activate immediately so settings apply, the
       checkbox reflects active, and LoadFile()'s status message
       can fire. */
    if (list.size() == 1 && LoadFile(path))
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
  const char *old_filename = list[index].name;

  Plane plane;
  PlaneGlue::ReadFile(plane, old_path);

  while (dlgPlaneDetailsShowModal(plane)) {
    if (plane.registration.empty()) {
      ShowMessageBox(_("Please enter the registration of the plane!"),
                  _("Error"), MB_OK);
      continue;
    }

    StaticString<42> filename(plane.registration);
    filename += ".xcp";

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

  if (ShowMessageBox(tmp, " ", MB_YESNO) == IDYES &&
      LoadWithDialog(i))
    UpdateList();
}

bool
PlaneListWidget::IsMouseActivateHotspot([[maybe_unused]] unsigned index,
                                       PixelPoint relative,
                                       PixelSize item_size) const noexcept
{
  /* Grow the hit target slightly for touch. */
  PixelRect box = GetListRowCheckBoxRect(PixelRect{item_size});
  box.Grow((int)Layout::GetTextPadding());
  return box.GetWidth() > 0 && box.Contains(relative);
}

bool
PlaneListWidget::OnMouseActivateItem(unsigned index, PixelPoint relative,
                                     PixelSize item_size) noexcept
{
  PixelRect box = GetListRowCheckBoxRect(PixelRect{item_size});
  box.Grow((int)Layout::GetTextPadding());
  if (box.GetWidth() == 0 || !box.Contains(relative))
    return false;

  assert(index < list.size());

  /* Checkbox tap activates without the confirm dialog; already
     active is a no-op. */
  if (!Profile::GetPathIsEqual("PlanePath", list[index].path) &&
      LoadWithDialog(index))
    UpdateList();

  return true;
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
  /* Like Alternates: list cursor picks the plane; Left/Right arm an
     action (Activate/New/…); Enter runs it. Empty list or sole
     active plane: Activate is disabled and
     SelectFirstEnabledButton() arms New. */
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}
