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

#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Draw.hpp"
#include "UIGlobals.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Gauge/LogoView.hpp"
#include "resource.h"
#include "Screen/Layout.hpp"
#include "Simulator.hpp"

#include <stdio.h>

#ifdef SIMULATOR_AVAILABLE

class WndButton;

enum {
  mrFly = 1000,
  mrSimulator,
};

static WndForm *wf = NULL;
static LogoView *logo;

static void
OnLogoPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.ClearWhite();
  logo->draw(canvas, Sender->GetClientRect());
}

static void
OnSimulatorClicked()
{
  wf->SetModalResult(mrSimulator);
}

static void
OnFlyClicked()
{
  wf->SetModalResult(mrFly);
}

static void
OnQuitClicked()
{
  wf->SetModalResult(mrCancel);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnLogoPaint),
  DeclareCallBackEntry(OnFlyClicked),
  DeclareCallBackEntry(OnSimulatorClicked),
  DeclareCallBackEntry(OnQuitClicked),
  DeclareCallBackEntry(NULL)
};

#endif

SimulatorPromptResult
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  int result;

  logo = new LogoView();
  do {
    wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                    Layout::landscape ? _T("IDR_XML_SIMULATORPROMPT_L") :
                                        _T("IDR_XML_SIMULATORPROMPT"));
    assert(wf != NULL);

    result = wf->ShowModal();

    delete wf;
  } while (result == mrChangeLayout);
  delete logo;

  switch (result) {
  case mrFly:
    return SPR_FLY;

  case mrSimulator:
    return SPR_SIMULATOR;

  default:
    return SPR_QUIT;
  }
#else
  return SPR_FLY;
#endif
}

