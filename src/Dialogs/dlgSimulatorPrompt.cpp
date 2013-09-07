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

#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Form/SymbolButton.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Screen/Canvas.hpp"
#include "Gauge/LogoView.hpp"
#include "Screen/Layout.hpp"
#include "Simulator.hpp"

#ifdef SIMULATOR_AVAILABLE

class WndButton;

enum {
  mrFly = 1000,
  mrSimulator,
};

class SimulatorPromptWindow final : public ContainerWindow {
  const DialogLook &look;
  ActionListener &action_listener;

  LogoView logo_view;
  PixelRect logo_rect;

  WndButton quit_button;
  WndSymbolButton fly_button, sim_button;
  RasterPoint label_position;

public:
  SimulatorPromptWindow(const DialogLook &_look,
                        ActionListener &_action_listener)
    :look(_look), action_listener(_action_listener),
    quit_button(look.button),
    fly_button(look.button), sim_button(look.button) {}

protected:
  /* virtual methods from class Window */
  virtual void OnCreate() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnPaint(Canvas &canvas) override;
};

void
SimulatorPromptWindow::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();

  ButtonWindowStyle style;
  style.TabStop();

  fly_button.Create(*this, _T("Fly"), rc, style,
                    action_listener, mrFly);
  sim_button.Create(*this, _T("Simulator"), rc, style,
                    action_listener, mrSimulator);
  quit_button.Create(*this, _("Quit"), rc, style,
                     action_listener, mrCancel);
}

void
SimulatorPromptWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  const PixelRect rc = GetClientRect();

  const unsigned h_middle = new_size.cx / 2;
  const unsigned bottom_padding = Layout::Scale(15);
  const unsigned button_width = Layout::Scale(112);
  const unsigned button_height = Layout::Scale(30);
  const unsigned label_height =
    look.text_font->GetHeight() + Layout::GetTextPadding();

  PixelRect button_rc;
  button_rc.left = h_middle - button_width;
  button_rc.right = h_middle;
  button_rc.bottom = rc.bottom - bottom_padding;
  button_rc.top = button_rc.bottom - button_height;
  fly_button.Move(button_rc);

  label_position.x = button_rc.left;
  label_position.y = button_rc.top - label_height;

  button_rc.left = button_rc.right;
  button_rc.right = h_middle + button_width;
  sim_button.Move(button_rc);

  logo_rect = rc;
  logo_rect.bottom = button_rc.top - label_height - Layout::Scale(5);

  button_rc = rc;
  button_rc.left = button_rc.right - Layout::Scale(75);
  button_rc.bottom = button_rc.top + Layout::GetMaximumControlHeight();
  quit_button.Move(button_rc);
}

void
SimulatorPromptWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();
  logo_view.draw(canvas, logo_rect);

  canvas.Select(*look.text_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(label_position.x, label_position.y,
                  _("What do you want to do?"));

  ContainerWindow::OnPaint(canvas);
}

class SimulatorPromptWidget final : public WindowWidget {
  SimulatorPromptWindow w;

public:
  SimulatorPromptWidget(const DialogLook &_look,
                        ActionListener &_action_listener)
    :w(_look, _action_listener) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    WindowStyle style;
    style.Hide();
    style.ControlParent();

    w.Create(parent, rc, style);
    SetWindow(&w);
  }
};

#endif

SimulatorPromptResult
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  SimulatorPromptWidget widget(look, dialog);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _T(""), &widget);

  const int result = dialog.ShowModal();
  dialog.StealWidget();

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

