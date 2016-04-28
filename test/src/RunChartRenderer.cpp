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

#define ENABLE_DIALOG_LOOK

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Look/ChartLook.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Util/Macros.hpp"
#include "Renderer/ChartRenderer.hpp"

static const TCHAR *const chart_names[] = {
  _T("Line"),
  _T("Line2"),
};

class ChartWindow : public PaintWindow {
  unsigned chart;
  const ChartLook &look;

public:
  ChartWindow(const ChartLook &_look)
    :look(_look)
  {
    SetChart(0);
  }

  void SetChart(unsigned _chart) {
    assert(_chart < ARRAY_SIZE(chart_names));
    chart = _chart;
  }

protected:
  virtual void OnPaint(Canvas &canvas) override;

  void DrawChart(ChartRenderer &renderer);
};

void
ChartWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();
  ChartRenderer renderer(look, canvas, canvas.GetRect());
  DrawChart(renderer);
}

void
ChartWindow::DrawChart(ChartRenderer &renderer)
{
  renderer.ScaleXFromValue(0);
  renderer.ScaleXFromValue(100);

  renderer.ScaleYFromValue(0);
  renderer.ScaleYFromValue(100);

  if (chart == 0) {
    renderer.DrawLine(0, 10, 100, 70,
                      look.GetPen(ChartLook::STYLE_BLUETHINDASH));
  } else if (chart == 1) {
    renderer.ScaleXFromValue(-50);
    renderer.ScaleXFromValue(110);
    renderer.ScaleYFromValue(110);

    renderer.DrawLine(0, 10, 100, 70,
                      look.GetPen(ChartLook::STYLE_BLUETHINDASH));

    renderer.DrawLine(0, 10, 100, 80,
                      look.GetPen(ChartLook::STYLE_GREENDASH));

    renderer.DrawLine(0, 10, 100, 100,
                      look.GetPen(ChartLook::STYLE_BLACK));

    renderer.DrawXGrid(20, 20, ChartRenderer::UnitFormat::NUMERIC);

    renderer.DrawYGrid(20, 20, ChartRenderer::UnitFormat::NUMERIC);

    renderer.DrawLabel(_T("hello"), 50, 50);
    renderer.DrawXLabel(_T("VVV"),_T("m/s"));
    renderer.DrawYLabel(_T("AAA"),_T("m/s"));
  }
}

class TestWindow : public SingleWindow,
                   ActionListener,
                   ListItemRenderer, ListCursorHandler {
  Button close_button;
  ListControl *type_list;
  ChartWindow chart;

  enum Buttons {
    CLOSE,
  };

public:
  TestWindow(const ChartLook &chart_look)
    :type_list(NULL), chart(chart_look) {}
  ~TestWindow() {
    delete type_list;
  }

  void Create(const DialogLook &look, PixelSize size) {
    SingleWindow::Create(_T("RunChartRenderer"), size);

    const PixelRect rc = GetClientRect();

    WindowStyle with_border;
    with_border.Border();

    const PixelRect list_rc(0, 0, 250, rc.bottom - 30);

    type_list = new ListControl(*this, look, list_rc,
                                with_border, 25);

    type_list->SetItemRenderer(this);
    type_list->SetCursorHandler(this);
    type_list->SetLength(ARRAY_SIZE(chart_names));

    PixelRect chart_rc = rc;
    chart_rc.left = list_rc.right;
    chart.Create(*this, chart_rc, with_border);

    PixelRect button_rc = rc;
    button_rc.right = list_rc.right;
    button_rc.top = button_rc.bottom - 30;
    close_button.Create(*this, *button_look, _T("Close"), button_rc,
                        WindowStyle(),
                        *this, CLOSE);

    type_list->SetFocus();
  }

protected:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override {
    switch (id) {
    case CLOSE:
      Close();
      break;
    }
  }

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override {
    assert(idx < ARRAY_SIZE(chart_names));

    canvas.DrawText(rc.left + 2, rc.top + 2, chart_names[idx]);
  }

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned idx) override {
    assert(idx < ARRAY_SIZE(chart_names));

    chart.SetChart(idx);
  }
};

static void
Main()
{
  ChartLook chart_look;
  chart_look.Initialise();

  TestWindow window(chart_look);
  window.Create(*dialog_look, {640, 480});

  window.Show();
  window.RunEventLoop();
}
