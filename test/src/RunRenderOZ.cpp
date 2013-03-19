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
#include "Look/AirspaceLook.hpp"
#include "Look/TaskLook.hpp"
#include "Form/List.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/MatCylinderZone.hpp"
#include "Engine/Task/ObservationZones/FAISectorZone.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Engine/Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Engine/Task/ObservationZones/BGAStartSectorZone.hpp"
#include "Engine/Task/ObservationZones/AnnularSectorZone.hpp"
#include "Engine/Task/ObservationZones/Boundary.hpp"
#include "Projection/Projection.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"

enum {
  NUM_OZ_TYPES = 11,
};

static const TCHAR *const oz_type_names[NUM_OZ_TYPES] = {
  _T("Line"),
  _T("Cylinder"),
  _T("MAT Cylinder"),
  _T("Sector"),
  _T("FAI Sector"),
  _T("Keyhole"),
  _T("BGA Fixed Course"),
  _T("BGA Enhanced Option"),
  _T("BGA Start"),
  _T("Annular sector"),
  _T("Symmetric quadrant"),
};

static GeoPoint location(Angle::Degrees(7.7061111111111114),
                         Angle::Degrees(51.051944444444445));

static GeoPoint previous(Angle::Degrees(10.6),
                         Angle::Degrees(49));

static GeoPoint next(Angle::Degrees(10.2),
                     Angle::Degrees(51.4));

static AirspaceRendererSettings airspace_renderer_settings;

class OZWindow : public PaintWindow {
  OZRenderer roz;
  ObservationZonePoint *oz;
  Projection projection;

public:
  OZWindow(const TaskLook &task_look, const AirspaceLook &airspace_look)
    :roz(task_look, airspace_look, airspace_renderer_settings), oz(NULL) {
    projection.SetGeoLocation(location);
    set_shape(ObservationZone::Shape::LINE);
  }

  ~OZWindow() {
    delete oz;
  }

  void set_shape(ObservationZone::Shape shape) {
    if (oz != NULL && shape == oz->GetShape())
      return;

    delete oz;
    oz = NULL;

    fixed radius(10000);

    switch (shape) {
    case ObservationZone::Shape::LINE:
      oz = new LineSectorZone(location, 2 * radius);
      break;

    case ObservationZone::Shape::CYLINDER:
      oz = new CylinderZone(location, radius);
      break;

    case ObservationZone::Shape::MAT_CYLINDER:
      oz = new MatCylinderZone(location);
      break;

    case ObservationZone::Shape::SECTOR:
      oz = new SectorZone(location, radius,
                          Angle::Degrees(0), Angle::Degrees(70));
      break;

    case ObservationZone::Shape::ANNULAR_SECTOR:
      oz = new AnnularSectorZone(location, radius,
                                 Angle::Degrees(0), Angle::Degrees(70),
                                 Half(radius));
      break;

    case ObservationZone::Shape::FAI_SECTOR:
      oz = new FAISectorZone(location);
      break;

    case ObservationZone::Shape::DAEC_KEYHOLE:
      oz = KeyholeZone::CreateDAeCKeyholeZone(location);
      break;

    case ObservationZone::Shape::BGAFIXEDCOURSE:
      oz = new BGAFixedCourseZone(location);
      break;

    case ObservationZone::Shape::BGAENHANCEDOPTION:
      oz = new BGAEnhancedOptionZone(location);
      break;

    case ObservationZone::Shape::BGA_START:
      oz = new BGAStartSectorZone(location);
      break;

    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
      oz = new SymmetricSectorZone(location);
      break;
    }

    if (oz != NULL)
      oz->SetLegs(&previous, &next);

    if (IsDefined())
      Invalidate();
  }

protected:
  virtual void OnPaint(Canvas &canvas) override;

  virtual void OnResize(PixelSize new_size) override {
    PaintWindow::OnResize(new_size);
    projection.SetScale(fixed(new_size.cx) / 21000);
    projection.SetScreenOrigin(new_size.cx / 2, new_size.cy / 2);
  }
};

void
OZWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();
  if (oz == NULL)
    return;

  const int offset = 0;

  roz.Draw(canvas, OZRenderer::LAYER_SHADE, projection, *oz, offset);
  roz.Draw(canvas, OZRenderer::LAYER_INACTIVE, projection, *oz, offset);
  roz.Draw(canvas, OZRenderer::LAYER_ACTIVE, projection, *oz, offset);

  /* debugging for ObservationZone::GetBoundary() */
  Pen pen(1, COLOR_RED);
  canvas.Select(pen);
  const OZBoundary boundary = oz->GetBoundary();
  for (auto i = boundary.begin(), end = boundary.end(); i != end; ++i) {
    RasterPoint p = projection.GeoToScreen(*i);
    canvas.DrawLine(p.x - 3, p.y - 3, p.x + 3, p.y + 3);
    canvas.DrawLine(p.x + 3, p.y - 3, p.x - 3, p.y + 3);
  }
}

class TestWindow : public SingleWindow,
                   ListItemRenderer, ListCursorHandler {
  ButtonWindow close_button;
  ListControl *type_list;
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

  void Create(const DialogLook &look, PixelSize size) {
    SingleWindow::Create(_T("RunRenderOZ"), size);

    const PixelRect rc = GetClientRect();

    WindowStyle with_border;
    with_border.Border();

    PixelRect oz_rc = rc;
    oz_rc.left = oz_rc.right / 2;
    oz.Create(*this, oz_rc, with_border);

    const PixelRect list_rc(0, 0, rc.right / 2, rc.bottom - 30);

    type_list = new ListControl(*this, look, list_rc,
                                with_border, 25);

    type_list->SetItemRenderer(this);
    type_list->SetCursorHandler(this);
    type_list->SetLength(NUM_OZ_TYPES);

    PixelRect button_rc = rc;
    button_rc.right = (rc.left + rc.right) / 2;
    button_rc.top = button_rc.bottom - 30;
    close_button.Create(*this, _T("Close"), ID_CLOSE, button_rc);

    oz.set_shape(ObservationZone::Shape::LINE);

    type_list->SetFocus();
  }

protected:
  virtual bool OnCommand(unsigned id, unsigned code) override {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
  }

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override {
    canvas.DrawText(rc.left + 2, rc.top + 2, oz_type_names[idx]);
  }

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned idx) override {
    assert(idx < NUM_OZ_TYPES);

    oz.set_shape((ObservationZone::Shape)idx);
  }
};

static void
Main()
{
  airspace_renderer_settings.SetDefaults();

  TaskLook *task_look = new TaskLook();
  task_look->Initialise();

  AirspaceLook *airspace_look = new AirspaceLook();
  airspace_look->Initialise(airspace_renderer_settings);

  TestWindow window(*task_look, *airspace_look);
  window.Create(*dialog_look, {480, 480});

  window.Show();
  window.RunEventLoop();

  delete airspace_look;
  delete task_look;
}
