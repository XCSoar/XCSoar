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

#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "LocalTime.hpp"
#include "UtilsText.hpp"
#include "OS/PathName.hpp"
#include "Math/SunEphemeris.hpp"
#include "ComputerSettings.hpp"
#include "LocalPath.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "Task/TaskManager.hpp"
#include "Task/MapTaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Compiler.h"
#include "Compatibility/path.h"
#include "Input/InputEvents.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Util/Macros.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

static int page = 0;
static WndForm *wf = NULL;
static WndFrame *wDetails = NULL;
static WndFrame *wInfo = NULL;
static WndFrame *wCommand = NULL;
static WndOwnerDrawFrame *wImage = NULL;
static WndButton *wMagnify = NULL;
static WndButton *wShrink = NULL;
static const Waypoint *waypoint = NULL;

static StaticArray<Bitmap, 5> images;
static int zoom = 0;

static void
NextPage(int Step)
{
  assert(waypoint);

  int last_page = 2 + images.size();
  do {
    page += Step;
    if (page < 0)
      page = last_page;
    else if (page > last_page)
      page = 0;
    // skip wDetails frame, if there are no details
  } while (page == 1 &&
#ifdef ANDROID
           waypoint->files_external.empty() &&
#endif
           waypoint->details.empty());

  wInfo->set_visible(page == 0);
  wDetails->set_visible(page == 1);
  wCommand->set_visible(page == 2);
  wImage->set_visible(page >= 3);
  zoom = 0;
  wMagnify->set_visible(page >= 3);
  wMagnify->set_enabled(true);
  wShrink->set_visible(page >= 3);
  wShrink->set_enabled(false);
}

static void
OnMagnifyClicked(gcc_unused WndButton &button)
{
  if (zoom >= 5)
    return;
  zoom++;

  wMagnify->set_enabled(zoom < 5);
  wShrink->set_enabled(zoom > 0);
  wImage->invalidate();
}

static void
OnShrinkClicked(gcc_unused WndButton &button)
{
  if (zoom <= 0)
    return;
  zoom--;

  wMagnify->set_enabled(zoom < 5);
  wShrink->set_enabled(zoom > 0);
  wImage->invalidate();
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  NextPage(+1);
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnGotoClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  assert(waypoint != NULL);

  protected_task_manager->DoGoto(*waypoint);
  wf->SetModalResult(mrOK);

  CommonInterface::main_window.full_redraw();
}

static void
OnReplaceClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::ReplaceInTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("No active task point."), _("Replace in task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
  case MapTaskManager::MUTATED_FROM_GOTO:
    break;
  }
}

static void 
OnNewHomeClicked(gcc_unused WndButton &button)
{
  assert(waypoint != NULL);

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();

  settings_computer.SetHome(*waypoint);

  {
    ScopeSuspendAllThreads suspend;
    WaypointGlue::SetHome(way_points, terrain,
                          settings_computer,
                          false);
  }

  wf->SetModalResult(mrOK);
}

static void
OnInsertInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::InsertInTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;

  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

static void
OnAppendInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::AppendToTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

#if 0
// JMW disabled until 6.2 work, see #996
static task_edit_result
goto_and_clear_task(const Waypoint &wp)
{
  if (protected_task_manager == NULL)
    return INVALID;

  protected_task_manager->DoGoto(wp);
  TaskEvents task_events;
  const OrderedTask blank(task_events,
                          XCSoarInterface::GetComputerSettings(),
                          XCSoarInterface::Calculated().glide_polar_task);
  protected_task_manager->task_commit(blank);

  return SUCCESS;
}

static unsigned
ordered_task_size()
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  const OrderedTask &ot = task_manager->get_ordered_task();
  if (ot.check_task())
    return ot.TaskSize();

  return 0;
}

static void
OnGotoAndClearTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  if ((ordered_task_size() > 2) && MessageBoxX(_("Clear current task?"),
                        _("Goto and clear task"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  switch (goto_and_clear_task(*waypoint)) {
  case SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case NOTASK:
  case UNMODIFIED:
  case INVALID:
    MessageBoxX(_("Unknown error creating task."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  }
}
#endif

static void
OnRemoveFromTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::RemoveFromTask(*waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("Waypoint not in task."), _("Remove from task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
  case MapTaskManager::MUTATED_TO_GOTO:
    break;
  }
}

static void
OnActivatePanClicked(gcc_unused WndButton &button)
{
  GlueMapWindow *map_window = CommonInterface::main_window.ActivateMap();
  if (map_window == NULL)
    return;

  map_window->PanTo(waypoint->location);
  XCSoarInterface::main_window.SetFullScreen(true);
  InputEvents::setMode(InputEvents::MODE_PAN);
  wf->SetModalResult(mrOK);
}

static void
OnImagePaint(gcc_unused WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.ClearWhite();
  if (page >= 3 && page < 3 + (int)images.size()) {
    Bitmap &img = images[page-3];
    static const int zoom_factors[] = { 1, 2, 4, 8, 16, 32 };
    RasterPoint img_pos, screen_pos;
    PixelSize screen_size;
    PixelSize img_size = img.GetSize();
    fixed scale = std::min((fixed)canvas.get_width() / (fixed)img_size.cx,
                           (fixed)canvas.get_height() / (fixed)img_size.cy) *
                  zoom_factors[zoom];

    // centered image and optionally zoomed into the center of the image
    fixed scaled_size = img_size.cx * scale;
    if (scaled_size <= (fixed)canvas.get_width()) {
      img_pos.x = 0;
      screen_pos.x = (int) (((fixed)canvas.get_width() - scaled_size) / 2);
      screen_size.cx = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.get_width() / scale;
      img_pos.x = (int) (((fixed)img_size.cx - scaled_size) / 2);
      img_size.cx = (int) scaled_size;
      screen_pos.x = 0;
      screen_size.cx = canvas.get_width();
    }
    scaled_size = img_size.cy * scale;
    if (scaled_size <= (fixed)canvas.get_height()) {
      img_pos.y = 0;
      screen_pos.y = (int) (((fixed)canvas.get_height() - scaled_size) / 2);
      screen_size.cy = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.get_height() / scale;
      img_pos.y = (int) (((fixed)img_size.cy - scaled_size) / 2);
      img_size.cy = (int) scaled_size;
      screen_pos.y = 0;
      screen_size.cy = canvas.get_height();
    }
    canvas.stretch(screen_pos.x, screen_pos.y, screen_size.cx, screen_size.cy,
                   img, img_pos.x, img_pos.y, img_size.cx, img_size.cy);
  }
}

#ifdef ANDROID

// TODO: support other platforms

static void
OnFileListEnter(gcc_unused unsigned i)
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);

  TCHAR path[MAX_PATH];
  LocalPath(path, file->c_str());

  native_view->openFile(path);
}

static void
OnFileListItemPaint(Canvas &canvas, const PixelRect paint_rc, unsigned i)
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);
  canvas.text(paint_rc.left + Layout::Scale(2),
              paint_rc.top + Layout::Scale(2), file->c_str());
}
#endif

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
    DeclareCallBackEntry(OnMagnifyClicked),
    DeclareCallBackEntry(OnShrinkClicked),
    DeclareCallBackEntry(OnNextClicked),
    DeclareCallBackEntry(OnPrevClicked),
    DeclareCallBackEntry(OnGotoClicked),
    DeclareCallBackEntry(OnCloseClicked),
    DeclareCallBackEntry(OnReplaceClicked),
    DeclareCallBackEntry(OnInsertInTaskClicked),
    DeclareCallBackEntry(OnAppendInTaskClicked),
    DeclareCallBackEntry(OnRemoveFromTaskClicked),
    DeclareCallBackEntry(OnNewHomeClicked),
    DeclareCallBackEntry(OnActivatePanClicked),
    DeclareCallBackEntry(OnImagePaint),
    DeclareCallBackEntry(NULL)
};

static void
ShowTaskCommands()
{
  if (protected_task_manager == NULL)
    return;

  WndButton *wb = ((WndButton *)wf->FindByName(_T("cmdRemoveFromTask")));
  assert(wb != NULL);
  wb->set_visible(MapTaskManager::GetIndexInTask(*waypoint) >= 0);
}

static void
UpdateCaption(const TCHAR *waypoint_name)
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: '%s'"), _("Waypoint Info"), waypoint_name);
  wf->SetCaption(buffer);
}

static void
UpdateComment(const TCHAR *comment)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpWpComment"));
  assert(wp != NULL);
  wp->SetText(comment);
}

static void
UpdateLocation(const GeoPoint &location)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Location"));
  assert(wp != NULL);

  TCHAR buffer[64];
  if (FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer)) != NULL)
    wp->SetText(buffer);
}

static void
UpdateDistanceBearing(const GeoPoint &own_location,
                      const GeoPoint &waypoint_location)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("BearingDistance"));
  assert(wp != NULL);

  GeoVector vector = own_location.DistanceBearing(waypoint_location);

  TCHAR distance_buffer[32];
  Units::FormatUserDistanceSmart(vector.distance, distance_buffer,
                            ARRAY_SIZE(distance_buffer));

  TCHAR buffer[64];
  FormatBearing(buffer, ARRAY_SIZE(buffer), vector.bearing, distance_buffer);

  wp->SetText(buffer);
}

static void
UpdateElevation(fixed elevation)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpAltitude"));
  assert(wp != NULL);

  TCHAR buffer[64];
  Units::FormatUserAltitude(elevation, buffer, ARRAY_SIZE(buffer));
  wp->SetText(buffer);
}

static void
UpdateRadioFrequency(const RadioFrequency &radio_frequency)
{
  if (!radio_frequency.IsDefined())
    return;

  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Radio"));
  assert(wp != NULL);

  TCHAR buffer[64];
  if (radio_frequency.Format(buffer, ARRAY_SIZE(buffer)) != NULL) {
    _tcscat(buffer, _T(" MHz"));
    wp->SetText(buffer);
  }
}

static void
UpdateRunwayInformation(const Runway &runway)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Runway"));
  assert(wp != NULL);

  StaticString<64> buffer;
  if (runway.IsDirectionDefined())
    buffer.Format(_T("%02u"), runway.GetDirectionName());
  else
    buffer.clear();

  if (runway.IsLengthDefined()) {
    if (!buffer.empty())
      buffer += _T("; ");

    TCHAR length_buffer[16];
    Units::FormatSmallUserDistance(length_buffer, ARRAY_SIZE(length_buffer),
                                   fixed(runway.GetLength()));
    buffer += length_buffer;
  }

  wp->SetText(buffer);
}

static void
UpdateSunsetTime(const GeoPoint &location, const BrokenDateTime &date_time)
{
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpSunset"));
  assert(wp != NULL);

  SunEphemeris::Result sun = SunEphemeris::CalcSunTimes(
      location, date_time, fixed(GetUTCOffset()) / 3600);

  int sunset_hour = (int)sun.time_of_sunset;
  int sunset_minute = (int)((sun.time_of_sunset - fixed(sunset_hour)) * 60);

  StaticString<64> buffer;
  buffer.Format(_T("%02d:%02d"), sunset_hour, sunset_minute);

  wp->SetText(buffer);
}

/**
 * Show the GlideResult in the form control.
 */
static void
ShowGlideResult(WndProperty &wp, const GlideResult &result)
{
  TCHAR buffer[64];
  Units::FormatRelativeUserAltitude(result.altitude_difference,
                           buffer, ARRAY_SIZE(buffer));
  wp.SetText(buffer);
}

static void
UpdateArrivalAltitudes(const ComputerSettings &settings_computer,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const Waypoint &waypoint)
{
  GlidePolar glide_polar = settings_computer.glide_polar_task;
  const GlidePolar &safety_polar = calculated.glide_polar_safety;

  const GlideState glide_state(basic.location.DistanceBearing(waypoint.location),
                               waypoint.altitude + settings_computer.task.safety_height_arrival,
                               basic.nav_altitude,
                               calculated.GetWindOrZero());

  // alt reqd at current mc
  WndProperty *wp = (WndProperty *)wf->FindByName(_T("prpMc2"));
  assert(wp != NULL);

  GlideResult r = MacCready::Solve(settings_computer.task.glide,
                                   glide_polar, glide_state);

  ShowGlideResult(*wp, r);

  // alt reqd at mc 0
  wp = (WndProperty *)wf->FindByName(_T("prpMc0"));
  assert(wp != NULL);

  glide_polar.SetMC(fixed_zero);
  r = MacCready::Solve(settings_computer.task.glide,
                       glide_polar, glide_state);

  ShowGlideResult(*wp, r);

  // alt reqd at safety mc
  wp = (WndProperty *)wf->FindByName(_T("prpMc1"));
  assert(wp != NULL);

  r = MacCready::Solve(settings_computer.task.glide,
                       safety_polar, glide_state);

  ShowGlideResult(*wp, r);
}

void 
dlgWaypointDetailsShowModal(SingleWindow &parent, const Waypoint &_waypoint,
                            bool allow_navigation)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  waypoint = &_waypoint;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_WAYPOINTDETAILS_L") :
                                      _T("IDR_XML_WAYPOINTDETAILS"));
  assert(wf != NULL);

  UpdateCaption(waypoint->name.c_str());
  UpdateComment(waypoint->comment.c_str());

  UpdateLocation(waypoint->location);

  if (basic.location_available)
    UpdateDistanceBearing(basic.location, waypoint->location);

  UpdateElevation(waypoint->altitude);
  UpdateRadioFrequency(waypoint->radio_frequency);
  UpdateRunwayInformation(waypoint->runway);

  if (basic.connected)
    UpdateSunsetTime(waypoint->location, basic.date_time_utc);

  if (protected_task_manager != NULL)
    UpdateArrivalAltitudes(settings_computer, basic, calculated, *waypoint);

  wf->SetKeyDownNotify(FormKeyDown);

  wInfo = (WndFrame *)wf->FindByName(_T("frmInfos"));
  assert(wInfo != NULL);

  wCommand = (WndFrame *)wf->FindByName(_T("frmCommands"));
  assert(wCommand != NULL);
  wCommand->hide();

  wDetails = (WndFrame *)wf->FindByName(_T("frmDetails"));
  assert(wDetails != NULL);

  WndListFrame *wFilesList = (WndListFrame *)wf->FindByName(_T("Files"));
  assert(wFilesList != NULL);

  EditWindow *wDetailsText = (EditWindow *)wf->FindByName(_T("Details"));
  assert(wDetailsText != NULL);
  wDetailsText->set_text(waypoint->details.c_str());

#ifdef ANDROID
  int num_files = std::distance(waypoint->files_external.begin(),
                                waypoint->files_external.end());
  if (num_files > 0) {
    wFilesList->SetPaintItemCallback(OnFileListItemPaint);
    wFilesList->SetCursorCallback(OnFileListEnter);
    wFilesList->SetActivateCallback(OnFileListEnter);

    unsigned list_height = wFilesList->GetItemHeight() * std::min(num_files, 5);
    wFilesList->resize(wFilesList->get_width(), list_height);
    wFilesList->SetLength(num_files);

    PixelRect rc = wDetailsText->get_position();
    rc.top += list_height;
    wDetailsText->move(rc);
  } else
#endif
    wFilesList->hide();

  wImage = (WndOwnerDrawFrame *)wf->FindByName(_T("frmImage"));
  assert(wImage != NULL);
  wMagnify = (WndButton *)wf->FindByName(_T("cmdMagnify"));
  assert(wMagnify != NULL);
  wShrink = (WndButton *)wf->FindByName(_T("cmdShrink"));
  assert(wShrink != NULL);

  if (!allow_navigation) {
    WndButton* butnav = (WndButton *)wf->FindByName(_T("cmdPrev"));
    assert(butnav != NULL);
    butnav->hide();

    butnav = (WndButton *)wf->FindByName(_T("cmdNext"));
    assert(butnav != NULL);
    butnav->hide();

    butnav = (WndButton *)wf->FindByName(_T("cmdGoto"));
    assert(butnav != NULL);
    butnav->hide();
  }

  ShowTaskCommands();

  for (auto it = waypoint->files_embed.begin(),
       it_end = waypoint->files_embed.end();
       it != it_end && !images.full(); it++) {
    TCHAR path[MAX_PATH];
    LocalPath(path, it->c_str());
    if (!images.append().LoadFile(path))
      images.shrink(images.size() - 1);
  }

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  for (auto image = images.begin(); image < images.end(); image++)
    image->Reset();

  images.clear();
}
