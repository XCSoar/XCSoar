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

#include "MacCreadyEdit.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Screen/Layout.hpp"
#include "Simulator.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "Form/Button.hpp"
#include "Form/XMLWidget.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Util/Macros.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"

class MacCreadyEditPanel : public XMLWidget {
  unsigned id;

public:
  MacCreadyEditPanel(unsigned _id):id(_id) {}

  void QuickAccess(const TCHAR *value) {
    InfoBoxManager::ProcessQuickAccess(id, value);
  }

  virtual PixelSize GetMinimumSize() const gcc_override {
    return PixelSize{Layout::Scale(205u), Layout::Scale(49)};
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static MacCreadyEditPanel *instance;

static void
PnlEditOnPlusSmall()
{
  instance->QuickAccess(_T("+0.1"));
}

static void
PnlEditOnPlusBig()
{
  instance->QuickAccess(_T("+0.5"));
}

static void
PnlEditOnMinusSmall()
{
  instance->QuickAccess(_T("-0.1"));
}

static void
PnlEditOnMinusBig()
{
  instance->QuickAccess(_T("-0.5"));
}

static constexpr CallBackTableEntry call_back_table[] = {
  DeclareCallBackEntry(PnlEditOnPlusSmall),
  DeclareCallBackEntry(PnlEditOnPlusBig),
  DeclareCallBackEntry(PnlEditOnMinusSmall),
  DeclareCallBackEntry(PnlEditOnMinusBig),
  DeclareCallBackEntry(NULL)
};

void
MacCreadyEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(call_back_table, parent, _T("IDR_XML_INFOBOXMACCREADYEDIT"));

  const fixed step = Units::ToSysVSpeed(GetUserVerticalSpeedStep());
  TCHAR caption[16];

  WndButton *button = (WndButton *)form.FindByName(_T("cmdPlusBig"));
  assert(button != NULL);
  FormatUserVerticalSpeed(step * 5, caption, false);
  button->SetCaption(caption);

  button = (WndButton *)form.FindByName(_T("cmdPlusSmall"));
  assert(button != NULL);
  FormatUserVerticalSpeed(step, caption, false);
  button->SetCaption(caption);

  button = (WndButton *)form.FindByName(_T("cmdMinusBig"));
  assert(button != NULL);
  FormatUserVerticalSpeed(step * -5, caption, false);
  button->SetCaption(caption);

  button = (WndButton *)form.FindByName(_T("cmdMinusSmall"));
  assert(button != NULL);
  FormatUserVerticalSpeed(-step, caption, false);
  button->SetCaption(caption);
}

Widget *
LoadMacCreadyEditPanel(unsigned id)
{
  return instance = new MacCreadyEditPanel(id);
}
