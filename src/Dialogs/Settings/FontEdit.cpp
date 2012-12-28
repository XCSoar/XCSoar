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

#include "FontEdit.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Util/StringUtil.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Compiler.h"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

enum ControlIndex {
#ifdef USE_GDI
  FACE,
#endif
  HEIGHT,
  WEIGHT,
  ITALIC,
  PREVIEW,
};

class FontEditWidget
  : public RowFormWidget, public ActionListener, DataFieldListener
{
  LOGFONT data;
  const LOGFONT &default_data;

  Font font;

public:
  FontEditWidget(const LOGFONT &_data, const LOGFONT &_default_data)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     data(_data), default_data(_default_data) {}

  const LOGFONT &GetData() const {
    return data;
  }

  void Load();
  void SaveValues();
  void UpdatePreview();

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);

  /* methods from ActionListener */
  virtual void OnAction(int id);

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);
};

void
FontEditWidget::Load()
{
#ifdef USE_GDI
  {
    DataFieldEnum &df = (DataFieldEnum &)GetDataField(FACE);
    df.SetStringAutoAdd(data.lfFaceName);
    GetControl(FACE).RefreshDisplay();
  }
#endif

  LoadValue(HEIGHT, (int)data.lfHeight);
  LoadValue(WEIGHT, data.lfWeight > 500);
  LoadValue(ITALIC, !!data.lfItalic);

  UpdatePreview();
}

inline void
FontEditWidget::SaveValues()
{
#ifdef USE_GDI
  CopyString(data.lfFaceName, GetDataField(FACE).GetAsString(), LF_FACESIZE);
#endif

  data.lfHeight = GetValueInteger(HEIGHT);
  data.lfWeight = GetValueBoolean(WEIGHT) ? 700 : 500;
  data.lfItalic = GetValueBoolean(ITALIC);
}

void
FontEditWidget::UpdatePreview()
{
  /* revert to default font first, to avoid freeing the Font while it
     is still being referenced */
  LargeTextWindow &preview = (LargeTextWindow &)GetGeneric(PREVIEW);
  preview.SetFont(*GetLook().text_font);

  font.Load(data);

#ifdef ENABLE_OPENGL
  TextCache::Flush();
#endif

  if (font.IsDefined()) {
    preview.SetFont(font);
    preview.SetText(_T("Sample Text\n123"));
  } else {
    preview.SetText(_("Font not found."));
  }
}

void
FontEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

#ifdef USE_GDI
  WndProperty *wp = AddEnum(_("Font face"), NULL, this);
  {
    DataFieldEnum &df = *(DataFieldEnum *)wp->GetDataField();
    df.addEnumText(_T("Tahoma"));
    df.addEnumText(_T("TahomaBD"));
    df.addEnumText(_T("DejaVu Sans Condensed"));
  }
#else
  /* we cannot obtain a list of fonts on SDL/OpenGL currently */
#endif

  AddInteger(_("Height"), NULL, _T("%d"), _T("%d"), 1, 200, 1, 0, this);
  AddBoolean(_T("Bold"), NULL, false, this);
  AddBoolean(_T("Italic"), NULL, false, this);

  PixelRect preview_rc { 0, 0, Layout::Scale(250), Layout::Scale(100) };
  LargeTextWindowStyle preview_style;
  preview_style.Border();
  LargeTextWindow *preview = new LargeTextWindow();
  preview->Create(*(ContainerWindow *)GetWindow(), preview_rc, preview_style);
  Add(preview);

  Load();
}

void
FontEditWidget::OnModified(DataField &df)
{
  SaveValues();
  UpdatePreview();
}

void
FontEditWidget::OnAction(int id)
{
  data = default_data;
  Load();
}

bool
dlgFontEditShowModal(const TCHAR *type, LOGFONT &data,
                     const LOGFONT &default_data)
{
  StaticString<128> title;
  title.Format(_T("%s: %s"), _("Edit Font"), type);

  FontEditWidget *widget =
    new FontEditWidget(data, default_data);

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), title, widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Reset"), *widget, 1);
  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() != mrOK)
    return false;

  data = widget->GetData();
  return true;
}
