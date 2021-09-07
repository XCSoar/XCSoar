/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "StartupDialog.hpp"
#include "Error.hpp"
#include "ProfilePasswordDialog.hpp"
#include "ProfileListDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/File.hpp"
#include "Language/Language.hpp"
#include "Gauge/LogoView.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"

class LogoWindow final : public PaintWindow {
  LogoView logo;

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();
    logo.draw(canvas, GetClientRect());
  }
};

class LogoQuitWidget final : public NullWidget {
  const ButtonLook &look;
  WndForm &dialog;

  LogoWindow logo;
  Button quit;

public:
  LogoQuitWidget(const ButtonLook &_look, WndForm &_dialog) noexcept
    :look(_look), dialog(_dialog) {}

private:
  PixelRect GetButtonRect(PixelRect rc) {
    rc.left = rc.right - Layout::Scale(75);
    rc.bottom = rc.top + Layout::GetMaximumControlHeight();
    return rc;
  }

public:
  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return { 150, 150 };
  }

  PixelSize GetMaximumSize() const noexcept override {
    /* use as much as possible */
    return { 8192, 8192 };
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    WindowStyle button_style(style);
    button_style.Hide();
    button_style.TabStop();

    quit.Create(parent, look, _("Quit"), rc,
                button_style, dialog.MakeModalResultCallback(mrCancel));
    logo.Create(parent, rc, style);
  }

  void Show(const PixelRect &rc) noexcept override {
    quit.MoveAndShow(GetButtonRect(rc));
    logo.MoveAndShow(rc);
  }

  void Hide() noexcept override {
    quit.FastHide();
    logo.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    quit.Move(GetButtonRect(rc));
    logo.Move(rc);
  }
};

class StartupWidget final : public RowFormWidget {
  enum Controls {
    PROFILE,
    CONTINUE,
  };

  WndForm &dialog;
  DataField *const df;

public:
  StartupWidget(const DialogLook &look, WndForm &_dialog,
                DataField *_df)
    :RowFormWidget(look), dialog(_dialog), df(_df) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  bool SetFocus() noexcept override {
    /* focus the "Continue" button by default */
    GetRow(CONTINUE).SetFocus();
    return true;
  }
};

static bool
SelectProfileCallback(const TCHAR *caption, DataField &_df,
                      const TCHAR *help_text)
{
  FileDataField &df = (FileDataField &)_df;

  const auto path = SelectProfileDialog(df.GetValue());
  if (path == nullptr)
    return false;

  df.ForceModify(path);
  return true;
}

void
StartupWidget::Prepare(ContainerWindow &parent,
                       const PixelRect &rc) noexcept
{
  auto *pe = Add(_("Profile"), nullptr, df);
  pe->SetEditCallback(SelectProfileCallback);

  AddButton(_("Continue"), dialog.MakeModalResultCallback(mrOK));
}

static bool
SelectProfile(Path path)
{
  try {
    if (!CheckProfilePasswordResult(CheckProfileFilePassword(path)))
      return false;
  } catch (...) {
    ShowError(std::current_exception(), _("Password"));
    return false;
  }

  Profile::SetFiles(path);

  if (RelativePath(path) == nullptr)
    /* When a profile from a secondary data path is used, this path
       becomes the primary data path */
    SetPrimaryDataPath(path.GetParent());

  File::Touch(path);
  return true;
}

bool
StartupWidget::Save(bool &changed) noexcept
{
  const auto &dff = (const FileDataField &)GetDataField(PROFILE);
  if (!SelectProfile(dff.GetValue()))
    return false;

  changed = true;

  return true;
}

bool
dlgStartupShowModal()
{
  LogFormat("Startup dialog");

  /* scan all profile files */
  auto *dff = new FileDataField();
  dff->ScanDirectoryTop(_T("*.prf"));

  if (dff->GetNumFiles() == 1) {
    /* skip this dialog if there is only one */
    const auto path = dff->GetValue();
    if (ProfileFileHasPassword(path) == TriState::FALSE &&
        SelectProfile(path)) {
      delete dff;
      return true;
    }
  } else if (dff->GetNumFiles() == 0) {
    /* no profile exists yet: create default profile */
    Profile::SetFiles(nullptr);
    return true;
  }

  /* preselect the most recently used profile */
  unsigned best_index = 0;
  std::chrono::system_clock::time_point best_timestamp =
    std::chrono::system_clock::time_point::min();
  unsigned length = dff->size();

  for (unsigned i = 0; i < length; ++i) {
    const auto path = dff->GetItem(i);
    const auto timestamp = File::GetLastModification(path);
    if (timestamp > best_timestamp) {
      best_timestamp = timestamp;
      best_index = i;
    }
  }

  dff->SetIndex(best_index);

  /* show the dialog */
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<TwoWidgets> dialog(WidgetDialog::Full{},
                                   UIGlobals::GetMainWindow(),
                                   UIGlobals::GetDialogLook(),
                                   nullptr);

  dialog.SetWidget(std::make_unique<LogoQuitWidget>(look.button, dialog),
                   std::make_unique<StartupWidget>(look, dialog, dff));

  return dialog.ShowModal() == mrOK;
}
