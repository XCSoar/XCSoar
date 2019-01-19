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

#include "ProfileListDialog.hpp"
#include "ProfilePasswordDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/TextListWidget.hpp"
#include "Form/Button.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Path.hpp"
#include "LocalPath.hpp"
#include "Profile/Map.hpp"
#include "Profile/File.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <vector>

#include <assert.h>

/* this macro exists in the WIN32 API */
#ifdef DELETE
#undef DELETE
#endif

class ProfileListWidget final
  : public TextListWidget, private ActionListener {

  struct ListItem {
    StaticString<32> name;
    AllocatedPath path;

    ListItem(const TCHAR *_name, Path _path)
      :name(_name), path(_path) {}

    bool operator<(const ListItem &i2) const {
      return StringCollate(name, i2.name) < 0;
    }
  };

  class ProfileFileVisitor: public File::Visitor
  {
    std::vector<ListItem> &list;

  public:
    ProfileFileVisitor(std::vector<ListItem> &_list):list(_list) {}

    void Visit(Path path, Path filename) override {
      list.emplace_back(filename.c_str(), path);
    }
  };

  enum Buttons {
    NEW,
    PASSWORD,
    COPY,
    DELETE,
  };

  const bool select;

  WndForm *form;
  Button *password_button;
  Button *copy_button, *delete_button;

  std::vector<ListItem> list;

public:
  ProfileListWidget(bool _select=false):select(_select) {}

  void CreateButtons(WidgetDialog &dialog);

  gcc_pure
  Path GetSelectedPath() const {
    if (list.empty())
      return nullptr;

    return list[GetList().GetCursorIndex()].path;
  }

  void SelectPath(Path path);

private:
  void UpdateList();

  gcc_pure
  int FindPath(Path path) const;

  void NewClicked();
  void PasswordClicked();
  void CopyClicked();
  void DeleteClicked();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

protected:
  /* virtual methods from TextListWidget */
  const TCHAR *GetRowText(unsigned i) const override {
    return list[i].name;
  }

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return select;
  }

  virtual void OnActivateItem(unsigned index) override {
    form->SetModalResult(mrOK);
  }

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
ProfileListWidget::UpdateList()
{
  list.clear();

  ProfileFileVisitor pfv(list);
  VisitDataFiles(_T("*.prf"), pfv);

  unsigned len = list.size();

  if (len > 0)
    std::sort(list.begin(), list.end());

  ListControl &list_control = GetList();
  list_control.SetLength(len);
  list_control.Invalidate();

  const bool empty = list.empty();
  password_button->SetEnabled(!empty);
  copy_button->SetEnabled(!empty);
  delete_button->SetEnabled(!empty);
}

int
ProfileListWidget::FindPath(Path path) const
{
  for (unsigned n = list.size(), i = 0u; i < n; ++i)
    if (path == list[i].path)
      return i;

  return -1;
}

void
ProfileListWidget::SelectPath(Path path)
{
  auto i = FindPath(path);
  if (i >= 0)
    GetList().SetCursorIndex(i);
}

void
ProfileListWidget::CreateButtons(WidgetDialog &dialog)
{
  form = &dialog;

  dialog.AddButton(_("New"), *this, NEW);
  password_button = dialog.AddButton(_("Password"), *this, PASSWORD);
  copy_button = dialog.AddButton(_("Copy"), *this, COPY);
  delete_button = dialog.AddButton(_("Delete"), *this, DELETE);
}

void
ProfileListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  TextListWidget::Prepare(parent, rc);
  UpdateList();
}

inline void
ProfileListWidget::NewClicked()
{
  StaticString<64> name;
  name.clear();
  if (!TextEntryDialog(name, _("Profile name")))
      return;

  StaticString<80> filename;
  filename = name;
  filename += _T(".prf");

  const auto path = LocalPath(filename);
  if (!File::CreateExclusive(path)) {
    ShowMessageBox(name, _("File exists already."), MB_OK|MB_ICONEXCLAMATION);
    return;
  }

  UpdateList();
  SelectPath(path);
}

inline void
ProfileListWidget::PasswordClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  const auto &item = list[GetList().GetCursorIndex()];

  ProfileMap data;

  try {
    Profile::LoadFile(data, item.path);
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to load file."));
    return;
  }

  if (!CheckProfilePasswordResult(CheckProfilePassword(data)) ||
      !SetProfilePasswordDialog(data))
    return;

  try {
    Profile::SaveFile(data, item.path);
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to save file."));
    return;
  }
}

inline void
ProfileListWidget::CopyClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  const auto &item = list[GetList().GetCursorIndex()];
  const Path old_path = item.path;

  ProfileMap data;

  try {
    Profile::LoadFile(data, old_path);
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to load file."));
    return;
  }

  if (!CheckProfilePasswordResult(CheckProfilePassword(data)))
    return;

  StaticString<64> new_name;
  new_name.clear();
  if (!TextEntryDialog(new_name, _("Profile name")) || new_name.empty())
      return;

  StaticString<80> new_filename;
  new_filename = new_name;
  new_filename += _T(".prf");

  const auto new_path = LocalPath(new_filename);

  if (File::ExistsAny(new_path)) {
    ShowMessageBox(new_name, _("File exists already."),
                   MB_OK|MB_ICONEXCLAMATION);
    return;
  }

  try {
    Profile::SaveFile(data, new_path);
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Failed to save file."));
    return;
  }

  UpdateList();
  SelectPath(new_path);
}

static bool
ConfirmDeleteProfile(const TCHAR *name)
{
  StaticString<256> tmp;
  StaticString<256> tmp_name(name);
  if (tmp_name.length() > 4)
    tmp_name.Truncate(tmp_name.length() - 4);

  tmp.Format(_("Delete \"%s\"?"),
             tmp_name.c_str());
  return ShowMessageBox(tmp, _("Delete"), MB_YESNO) == IDYES;
}

inline void
ProfileListWidget::DeleteClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  const auto &item = list[GetList().GetCursorIndex()];

  try {
    const auto password_result = CheckProfileFilePassword(item.path);
    switch (password_result) {
    case ProfilePasswordResult::UNPROTECTED:
      if (!ConfirmDeleteProfile(item.name))
        return;

      break;

    case ProfilePasswordResult::MATCH:
      break;

    case ProfilePasswordResult::MISMATCH:
    case ProfilePasswordResult::CANCEL:
      CheckProfilePasswordResult(password_result);
      return;
    }
  } catch (const std::runtime_error &e) {
    ShowError(e, _("Password"));
    return;
  }

  File::Delete(item.path);
  UpdateList();
}

void
ProfileListWidget::OnAction(int id)
{
  switch ((Buttons)id) {
  case NEW:
    NewClicked();
    break;

  case PASSWORD:
    PasswordClicked();
    break;

  case COPY:
    CopyClicked();
    break;

  case DELETE:
    DeleteClicked();
    break;
  }
}

void
ProfileListDialog()
{
  ProfileListWidget widget;
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Profiles"), &widget);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);
  dialog.EnableCursorSelection();

  dialog.ShowModal();
  dialog.StealWidget();
}

AllocatedPath
SelectProfileDialog(Path selected_path)
{
  ProfileListWidget widget(true);
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Select profile"), &widget);
  dialog.AddButton(_("Select"), mrOK);
  widget.CreateButtons(dialog);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.EnableCursorSelection();

  if (!selected_path.IsNull()) {
    dialog.PrepareWidget();
    widget.SelectPath(selected_path);
  }

  auto result = dialog.ShowModal();

  selected_path = result == mrOK
    ? widget.GetSelectedPath()
    : nullptr;
  dialog.StealWidget();

  return selected_path;
}
