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
#include "Dialogs/WidgetDialog.hpp"
#include "FontEdit.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/FontConfig.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"
#include "Screen/TextWindow.hpp"
#include "UIGlobals.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Util/StringUtil.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Compiler.h"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#include <assert.h>

extern LOGFONT log_infobox;
extern LOGFONT log_title;
extern LOGFONT log_map;
extern LOGFONT log_infobox_small;
extern LOGFONT log_map_bold;
extern LOGFONT log_cdi;
extern LOGFONT log_map_label;
extern LOGFONT log_map_label_important;

static bool changed = false;

class FontPreviewAndButton : public NullWidget, private ActionListener {
  const TCHAR *const profile_key;
  const TCHAR *const text;

  const LOGFONT defaults;

  bool enabled;
  PixelRect position;

  Font font;

  TextWindow preview;
  WndButton *button;

public:
  FontPreviewAndButton(const TCHAR *_key, const TCHAR *_text,
                       const LOGFONT _defaults,
                       bool _enabled)
    :profile_key(_key), text(_text), defaults(_defaults),
     enabled(_enabled) {}

  void SetEnabled(bool _enabled) {
    if (_enabled == enabled)
      return;

    enabled = _enabled;
    UpdateLayout();
  }

private:
  void UpdateLayout() {
    if (enabled) {
      unsigned button_width = Layout::Scale(50);
      if (unsigned(position.right - position.left) < button_width * 2)
        button_width = unsigned(position.right - position.left) / 2;

      PixelRect rc = position;
      rc.right -= button_width;
      preview.Move(rc);

      rc.left = rc.right;
      rc.right = position.right;
      button->MoveAndShow(rc);
    } else {
      button->FastHide();
      preview.Move(position);
    }
  }

  void UpdateFont() {
    preview.SetFont(*UIGlobals::GetDialogLook().text_font);
    font.Destroy();

    LOGFONT custom;
    if (!enabled || !Profile::GetFont(profile_key, &custom))
      custom = defaults;

    if (font.Load(custom)) {
#ifdef ENABLE_OPENGL
      TextCache::Flush();
#endif
      preview.SetFont(font);
    }
  }

public:
  /* virtual methods from Widget */

  virtual PixelSize GetMinimumSize() const override {
    return { unsigned(Layout::Scale(150)), Layout::GetMinimumControlHeight() };
  }

  virtual PixelSize GetMaximumSize() const override {
    return { unsigned(Layout::Scale(250)), Layout::GetMaximumControlHeight() };
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    ButtonWindowStyle button_style;
    button_style.Hide();
    button_style.TabStop();
    button = new WndButton(parent, UIGlobals::GetDialogLook(),
                           _("Edit"), rc, button_style, *this, 0);

    TextWindowStyle preview_style;
    preview_style.Hide();
    preview.Create(parent, text, rc, preview_style);

    position = rc;

    UpdateLayout();
  }

  virtual void Unprepare() override {
    preview.Destroy();
    delete button;
    font.Destroy();
  }

  virtual void Show(const PixelRect &rc) override {
    position = rc;
    UpdateLayout();
    UpdateFont();

    preview.Show();
    if (enabled)
      button->Show();
  }

  virtual void Hide() override {
    preview.Hide();
    if (enabled)
      button->Hide();
  }

  virtual void Move(const PixelRect &rc) override {
    position = rc;
    UpdateLayout();
  }

  virtual bool SetFocus() override {
    button->SetFocus();
    return true;
  }

private:
  /* virtual methods from ActionListener */

  virtual void OnAction(int id) override {
    LOGFONT custom;
    if (!Profile::GetFont(profile_key, &custom))
      custom = defaults;

    if (dlgFontEditShowModal(text, custom, defaults)) {
      Profile::SetFont(profile_key, custom);
      changed = true;
      UpdateFont();
    }
  }
};

static const struct {
  const TCHAR *registry_key;
  const TCHAR *text;
  const LOGFONT &defaults;
} customisable_fonts[] = {
  { ProfileKeys::FontTitleWindowFont, N_("InfoBox titles"), log_title },
  { ProfileKeys::FontInfoWindowFont, N_("InfoBox values, normal"),
    log_infobox },
  { ProfileKeys::FontTitleSmallWindowFont, N_("InfoBox values, small"),
    log_infobox_small },
  { ProfileKeys::FontMapWindowFont, N_("Waypoint labels"), log_map },
  { ProfileKeys::FontMapLabelFont, N_("Topography labels, normal"),
    log_map_label },
  { ProfileKeys::FontMapLabelImportantFont,
    N_("Topography labels, important"),
    log_map_label_important},
  { ProfileKeys::FontMapWindowBoldFont, N_("Dialog text"), log_map_bold },
  { ProfileKeys::FontCDIWindowFont, N_("Gauges"), log_cdi },
  { nullptr, nullptr, *(const LOGFONT *)nullptr }
};

class CustomFontsWidget : public RowFormWidget, private DataFieldListener {
public:
  CustomFontsWidget(const DialogLook &look):RowFormWidget(look) {}

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    const UISettings &ui_settings = CommonInterface::GetUISettings();
    const bool custom_fonts = ui_settings.custom_fonts;

    AddBoolean(_("Customize fonts"), nullptr, custom_fonts, this);

    for (auto i = customisable_fonts; i->registry_key != nullptr; ++i)
      Add(new FontPreviewAndButton(i->registry_key, i->text, i->defaults,
                                   custom_fonts));
  }

private:
  /* virtual methods from DataFieldListener */
  virtual void OnModified(DataField &df) override {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    const bool value = dfb.GetAsBoolean();

    UISettings &ui_settings = CommonInterface::SetUISettings();
    ui_settings.custom_fonts = value;
    Profile::Set(ProfileKeys::UseCustomFonts, value);

    for (unsigned i = 0; customisable_fonts[i].registry_key != nullptr; ++i) {
      FontPreviewAndButton &widget =
        (FontPreviewAndButton &)GetRowWidget(i + 1);
      widget.SetEnabled(value);
    }

    changed = true;
  }
};

void dlgConfigFontsShowModal()
{
  changed = false;

  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Font Configuration"),
                    new CustomFontsWidget(look));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();

  if (changed) {
    Profile::Save();
    ShowMessageBox(_("Changes to configuration saved.  Restart XCSoar to apply changes."),
                   _T(""), MB_OK);
  }
}
