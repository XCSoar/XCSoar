/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Dialogs/Internal.hpp"
#include "Screen/Layout.hpp"
#include "Gauge/ThermalAssistantWindow.hpp"
#include "Math/fixed.hpp"
#include "MainWindow.hpp"
#include "Compiler.h"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Blackboard/LiveBlackboard.hpp"

class ThermalAssistantListener : NullBlackboardListener {
  LiveBlackboard &blackboard;
  ThermalAssistantWindow &window;

public:
  ThermalAssistantListener(LiveBlackboard &_blackboard,
                           ThermalAssistantWindow &_window)
    :blackboard(_blackboard), window(_window) {
    blackboard.AddListener(*this);
    Update(blackboard.Calculated());
  }

  ~ThermalAssistantListener() {
    blackboard.RemoveListener(*this);
  }

private:
  void Update(const DerivedInfo &calculated) {
    window.Update(calculated.heading, calculated);
  }

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) {
    Update(calculated);
  }
};

static WndForm *wf = NULL;
static ThermalAssistantWindow *wta;

/**
 * This event handler is called when the "Close" button is pressed
 */
static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static Window *
OnCreateThermalAssistantControl(ContainerWindow &parent,
                                PixelScalar left, PixelScalar top,
                                UPixelScalar width, UPixelScalar height,
                            const WindowStyle style)
{
  wta = new ThermalAssistantWindow(Layout::FastScale(10), false);
  wta->set(parent, left, top, width, height, style);
  return wta;
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCreateThermalAssistantControl),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgThermalAssistantShowModal()
{
  const PixelRect targetRect = InfoBoxManager::layout.remaining;

  // Load dialog from XML
  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  _T("IDR_XML_THERMALASSISTANT"), &targetRect);

  if (!wf)
    return;

  {
    ThermalAssistantListener listener(CommonInterface::GetLiveBlackboard(),
                                      *wta);

    // Show the dialog
    wf->ShowModal();
  }

  // After dialog closed -> delete it
  delete wf;
}
