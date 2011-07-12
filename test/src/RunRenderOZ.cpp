/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
 * This program demonstrates the RenderObservationZone library.  It
 * shows a list of shapes, and draws the selected shape on the right.
 *
 */

#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/Init.hpp"
#include "Screen/Fonts.hpp"
#include "Look/DialogLook.hpp"
#include "Look/AirspaceLook.hpp"
#include "Look/TaskLook.hpp"
#include "Form/List.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "RenderObservationZone.hpp"
#include "Engine/Task/Tasks/BaseTask/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/FAISectorZone.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Engine/Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Engine/Task/ObservationZones/BGAStartSectorZone.hpp"
#include "Engine/Task/ObservationZones/AnnularSectorZone.hpp"
#include "Projection.hpp"
#include "Airspace/AirspaceRendererSettings.hpp"
#include "ResourceLoader.hpp"

enum {
  NUM_OZ_TYPES = 9,
};

static const TCHAR *const oz_type_names[NUM_OZ_TYPES] = {
  _T("Line"),
  _T("Cylinder"),
  _T("Sector"),
  _T("FAI Sector"),
  _T("Keyhole"),
  _T("BGA Fixed Course"),
  _T("BGA Enhanced Option"),
  _T("BGA Start"),
  _T("Annular sector"),
};

static GeoPoint location(Angle::degrees(fixed(7.7061111111111114)),
                         Angle::degrees(fixed(51.051944444444445)));

static GeoPoint previous(Angle::degrees(fixed(10.6)),
                         Angle::degrees(fixed(49)));

static GeoPoint next(Angle::degrees(fixed(10.2)),
                     Angle::degrees(fixed(51.4)));

static AirspaceRendererSettings airspace_renderer_settings;

class OZWindow : public PaintWindow {
  RenderObservationZone roz;
  ObservationZonePoint *oz;
  Projection projection;

public:
  OZWindow(const TaskLook &task_look, const AirspaceLook &airspace_look)
    :roz(task_look, airspace_look), oz(NULL) {
    projection.SetGeoLocation(location);
    set_shape(ObservationZonePoint::LINE);
  }

  ~OZWindow() {
    delete oz;
  }

  void set_shape(enum ObservationZonePoint::shape shape) {
    if (oz != NULL && shape == oz->shape)
      return;

    delete oz;
    oz = NULL;

    fixed radius(10000);

    switch (shape) {
    case ObservationZonePoint::LINE:
      oz = new LineSectorZone(location, 2 * radius);
      break;

    case ObservationZonePoint::CYLINDER:
      oz = new CylinderZone(location, radius);
      break;

    case ObservationZonePoint::SECTOR:
      oz = new SectorZone(location, radius,
                          Angle::degrees(fixed(0)),
                          Angle::degrees(fixed(70)));
      break;

    case ObservationZonePoint::ANNULAR_SECTOR:
      oz = new AnnularSectorZone(location, radius,
                                 Angle::degrees(fixed(0)),
                                 Angle::degrees(fixed(70)),
                                 radius*fixed_half);
      break;

    case ObservationZonePoint::FAI_SECTOR:
      oz = new FAISectorZone(location);
      break;

    case ObservationZonePoint::KEYHOLE:
      oz = new KeyholeZone(location);
      break;

    case ObservationZonePoint::BGAFIXEDCOURSE:
      oz = new BGAFixedCourseZone(location);
      break;

    case ObservationZonePoint::BGAENHANCEDOPTION:
      oz = new BGAEnhancedOptionZone(location);
      break;

    case ObservationZonePoint::BGA_START:
      oz = new BGAStartSectorZone(location);
      break;
    }

    if (oz != NULL)
      oz->set_legs(&previous, &location, &next);

    invalidate();
  }

protected:
  virtual void on_paint(Canvas &canvas);

  virtual bool on_resize(unsigned width, unsigned height) {
    PaintWindow::on_resize(width, height);
    projection.SetScale(fixed(width) / 21000);
    projection.SetScreenOrigin(width / 2, height / 2);
    return true;
  }
};

void
OZWindow::on_paint(Canvas &canvas)
{
  canvas.clear_white();
  if (oz == NULL)
    return;

  const int offset = 0;

  roz.set_layer(RenderObservationZone::LAYER_SHADE);
  if (roz.draw_style(canvas, airspace_renderer_settings, offset)) {
    roz.Draw(canvas, projection, *oz);
    roz.un_draw_style(canvas);
  }

  roz.set_layer(RenderObservationZone::LAYER_INACTIVE);
  if (roz.draw_style(canvas, airspace_renderer_settings, offset)) {
    roz.Draw(canvas, projection, *oz);
    roz.un_draw_style(canvas);
  }

  roz.set_layer(RenderObservationZone::LAYER_ACTIVE);
  if (roz.draw_style(canvas, airspace_renderer_settings, offset)) {
    roz.Draw(canvas, projection, *oz);
    roz.un_draw_style(canvas);
  }
}

static void
paint_oz_type_name(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  assert(idx < NUM_OZ_TYPES);

  canvas.text(rc.left + 2, rc.top + 2, oz_type_names[idx]);
}

static OZWindow *oz_window;

static void
oz_type_cursor_callback(unsigned idx)
{
  assert(idx < NUM_OZ_TYPES);

  oz_window->set_shape((enum ObservationZonePoint::shape)idx);
}

class TestWindow : public SingleWindow {
  ButtonWindow close_button;
  WndListFrame *type_list;
  OZWindow oz;

  enum {
    ID_START = 100,
    ID_CLOSE
  };

public:
  TestWindow(const TaskLook &task_look, const AirspaceLook &airspace_look)
    :type_list(NULL), oz(task_look, airspace_look) {}
  ~TestWindow() {
    delete type_list;
  }

#ifdef USE_GDI
  static bool register_class(HINSTANCE hInstance) {
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Window::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = 0;
    wc.lpszClassName = _T("RunRenderOZ");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void set(const DialogLook &look,
           int left, int top, unsigned width, unsigned height) {
    SingleWindow::set(_T("RunRenderOZ"), _T("RunRenderOZ"),
                      left, top, width, height);

    const PixelRect rc = get_client_rect();

    WindowStyle with_border;
    with_border.border();

    oz.set(*this, rc.right / 2, 0, rc.right - (rc.right / 2), rc.bottom,
           with_border);
    oz_window = &oz;

    type_list = new WndListFrame(*this, look,
                                 0, 0, rc.right / 2, rc.bottom - 30,
                                 with_border, 25);
    type_list->SetPaintItemCallback(paint_oz_type_name);
    type_list->SetCursorCallback(oz_type_cursor_callback);
    type_list->SetLength(NUM_OZ_TYPES);

    close_button.set(*this, _T("Close"), ID_CLOSE,
                     0, rc.bottom - 30, rc.right / 2, 30);

    oz.set_shape(ObservationZonePoint::LINE);

    type_list->set_focus();
  }

protected:
  virtual bool on_command(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      close();
      return true;
    }

    return SingleWindow::on_command(id, code);
  }
};

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  ScreenGlobalInit screen_init;

  airspace_renderer_settings.SetDefaults();

#ifdef USE_GDI
  ResourceLoader::Init(hInstance);
  TestWindow::register_class(hInstance);
#endif

  DialogLook *look = new DialogLook();
  look->Initialise();

  TaskLook *task_look = new TaskLook();
  task_look->Initialise();

  AirspaceLook *airspace_look = new AirspaceLook();
  airspace_look->Initialise(airspace_renderer_settings);

  TestWindow window(*task_look, *airspace_look);
  window.set(*look, 0, 0, 480, 480);

  Fonts::Initialize();

  window.show();
  window.event_loop();

  delete airspace_look;
  delete task_look;
  delete look;
  Fonts::Deinitialize();

  return 0;
}
