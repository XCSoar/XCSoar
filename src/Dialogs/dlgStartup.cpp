/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "WidgetDialog.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Language/Language.hpp"
#include "Gauge/LogoView.hpp"
#include "LogFile.hpp"
#include "Util/StringUtil.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "Compiler.h"

#include <windef.h> /* for MAX_PATH */

class LogoWindow final : public PaintWindow {
  LogoView logo;

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();
    logo.draw(canvas, GetClientRect());
  }
};

class LogoQuitWidget final : public NullWidget {
  ActionListener &action_listener;

  LogoWindow logo;
  WndButton quit;

public:
  LogoQuitWidget(const ButtonLook &look, ActionListener &_action_listener)
    :action_listener(_action_listener), quit(look) {}

private:
  PixelRect GetButtonRect(PixelRect rc) {
    rc.left = rc.right - Layout::Scale(75);
    rc.bottom = rc.top + Layout::GetMaximumControlHeight();
    return rc;
  }

public:
  /* virtual methods from class Widget */
  virtual PixelSize GetMinimumSize() const override {
    return { 150, 150 };
  }

  virtual PixelSize GetMaximumSize() const override {
    /* use as much as possible */
    return { 8192, 8192 };
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    WindowStyle style;
    style.Hide();

    ButtonWindowStyle button_style(style);
    button_style.TabStop();

    quit.Create(parent, _("Quit"), rc,
                style, action_listener, mrCancel);
    logo.Create(parent, rc, style);
  }

  virtual void Unprepare() override {
    logo.Destroy();
    quit.Destroy();
  }

  virtual void Show(const PixelRect &rc) override {
    quit.MoveAndShow(GetButtonRect(rc));
    logo.MoveAndShow(rc);
  }

  virtual void Hide() override {
    quit.FastHide();
    logo.FastHide();
  }

  virtual void Move(const PixelRect &rc) override {
    quit.Move(GetButtonRect(rc));
    logo.Move(rc);
  }
};

class StartupWidget final : public RowFormWidget {
  enum Controls {
    PROFILE,
    CONTINUE,
  };

  ActionListener &action_listener;
  DataField *const df;

public:
  StartupWidget(const DialogLook &look, ActionListener &_action_listener,
                DataField *_df)
    :RowFormWidget(look), action_listener(_action_listener), df(_df) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  virtual bool SetFocus() override {
    /* focus the "Continue" button by default */
    GetRow(CONTINUE).SetFocus();
    return true;
  }
};

void
StartupWidget::Prepare(ContainerWindow &parent,
                       const PixelRect &rc)
{
  Add(_("Profile"), nullptr, df);
  AddButton(_("Continue"), action_listener, mrOK);
}

static void
SelectProfile(const TCHAR *path)
{
  if (StringIsEmpty(path))
    return;

  Profile::SetFiles(path);

  if (RelativePath(path) == nullptr) {
    /* When a profile from a secondary data path is used, this path
       becomes the primary data path */
    TCHAR temp[MAX_PATH];
    SetPrimaryDataPath(DirName(path, temp));
  }

  File::Touch(path);
}

bool
StartupWidget::Save(bool &changed)
{
  const DataFieldFileReader &dff =
    (const DataFieldFileReader &)GetDataField(PROFILE);
  SelectProfile(dff.GetPathFile());
  changed = true;

  return true;
}

bool
dlgStartupShowModal()
{
  LogFormat("Startup dialog");

  /* scan all profile files */
  DataFieldFileReader *dfe = new DataFieldFileReader();
  dfe->ScanDirectoryTop(_T("*.prf"));

  /* skip this dialog if there is only one (or none) */
  if (dfe->GetNumFiles() <= 1) {
    SelectProfile(dfe->GetPathFile());
    delete dfe;
    return true;
  }

  /* preselect the most recently used profile */
  unsigned best_index = 0;
  uint64_t best_timestamp = 0;
  unsigned length = dfe->size();

  for (unsigned i = 0; i < length; ++i) {
    const TCHAR *path = dfe->GetItem(i);
    uint64_t timestamp = File::GetLastModification(path);
    if (timestamp > best_timestamp) {
      best_timestamp = timestamp;
      best_index = i;
    }
  }

  dfe->Set(best_index);

  /* show the dialog */
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  TwoWidgets widget(new LogoQuitWidget(look.button, dialog),
                    new StartupWidget(look, dialog, dfe));

  dialog.CreateFull(UIGlobals::GetMainWindow(), _T(""), &widget);

  const int result = dialog.ShowModal();
  dialog.StealWidget();

  return result == mrOK;
}
