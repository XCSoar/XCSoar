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

/*
 * This program demonstrates the OZRenderer library.  It
 * shows a list of shapes, and draws the selected shape on the right.
 *
 */

#define ENABLE_DIALOG_LOOK

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Look/ChartLook.hpp"
#include "Form/List.hpp"
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
  virtual void OnPaint(Canvas &canvas);

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
  renderer.ScaleXFromValue(fixed(0));
  renderer.ScaleXFromValue(fixed(100));

  renderer.ScaleYFromValue(fixed(0));
  renderer.ScaleYFromValue(fixed(100));

  if (chart == 0) {
    renderer.DrawLine(fixed(0), fixed(10), fixed(100), fixed(70),
                      look.GetPen(ChartLook::STYLE_BLUETHIN));
  } else if (chart == 1) {
    renderer.ScaleXFromValue(fixed(-50));
    renderer.ScaleXFromValue(fixed(110));
    renderer.ScaleYFromValue(fixed(110));

    renderer.DrawLine(fixed(0), fixed(10), fixed(100), fixed(70),
                      look.GetPen(ChartLook::STYLE_BLUETHIN));

    renderer.DrawLine(fixed(0), fixed(10), fixed(100), fixed(80),
                      look.GetPen(ChartLook::STYLE_DASHGREEN));

    renderer.DrawLine(fixed(0), fixed(10), fixed(100), fixed(100),
                      look.GetPen(ChartLook::STYLE_MEDIUMBLACK));

    renderer.DrawXGrid(fixed(20), look.GetPen(ChartLook::STYLE_THINDASHPAPER),
                       fixed(20), true);

    renderer.DrawYGrid(fixed(20), look.GetPen(ChartLook::STYLE_THINDASHPAPER),
                       fixed(20), true);
}
}

class TestWindow : public SingleWindow,
                   ListItemRenderer, ListCursorHandler {
  ButtonWindow close_button;
  ListControl *type_list;
  ChartWindow chart;

  enum {
    ID_START = 100,
    ID_CLOSE
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
    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);

    type_list->SetFocus();
  }

protected:
  virtual bool OnCommand(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
  }

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) {
    assert(idx < ARRAY_SIZE(chart_names));

    canvas.DrawText(rc.left + 2, rc.top + 2, chart_names[idx]);
  }

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned idx) gcc_override {
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
