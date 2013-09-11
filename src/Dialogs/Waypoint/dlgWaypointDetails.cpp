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

#include "WaypointDialogs.hpp"
#include "WaypointInfoWidget.hpp"
#include "WaypointCommandsWidget.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/XML.hpp"
#include "UIGlobals.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Widget/DockWindow.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "LocalPath.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/TaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Compiler.h"
#include "Language/Language.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileKeys.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <assert.h>
#include <stdio.h>
#include <windef.h> /* for MAX_PATH */

class WndOwnerDrawFrame;

static int page = 0;
static int last_page = 0;

static WndForm *form = nullptr;
static Window *details_panel = nullptr;
static DockWindow *info_widget = nullptr;
static DockWindow *commands_widget = nullptr;
static PaintWindow *image_window = nullptr;
static WndButton *magnify_button = nullptr;
static WndButton *shrink_button = nullptr;
static const Waypoint *waypoint = nullptr;

static StaticArray<Bitmap, 5> images;
static int zoom = 0;

static void
UpdatePage()
{
  info_widget->SetVisible(page == 0);
  details_panel->SetVisible(page == 1);
  commands_widget->SetVisible(page == 2);

  bool image_page = page >= 3;
  image_window->SetVisible(image_page);
  magnify_button->SetVisible(image_page);
  shrink_button->SetVisible(image_page);
}

static void
UpdateZoomControls()
{
  magnify_button->SetEnabled(zoom < 5);
  shrink_button->SetEnabled(zoom > 0);
}

static void
NextPage(int step)
{
  assert(waypoint);
  assert(last_page > 0);

  do {
    page += step;
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

  UpdatePage();

  if (page >= 3) {
    zoom = 0;
    UpdateZoomControls();
  }
}

static void
OnMagnifyClicked()
{
  if (zoom >= 5)
    return;
  zoom++;

  UpdateZoomControls();
  image_window->Invalidate();
}

static void
OnShrinkClicked()
{
  if (zoom <= 0)
    return;
  zoom--;

  UpdateZoomControls();
  image_window->Invalidate();
}

static void
OnNextClicked()
{
  NextPage(+1);
}

static void
OnPrevClicked()
{
  NextPage(-1);
}

static bool
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)form->FindByName(_T("cmdPrev")))->SetFocus();
    NextPage(-1);
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)form->FindByName(_T("cmdNext")))->SetFocus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnGotoClicked()
{
  if (protected_task_manager == nullptr)
    return;

  assert(waypoint != nullptr);

  protected_task_manager->DoGoto(*waypoint);
  form->SetModalResult(mrOK);

  CommonInterface::main_window->FullRedraw();
}

#if 0
// JMW disabled until 6.2 work, see #996
static task_edit_result
goto_and_clear_task(const Waypoint &wp)
{
  if (protected_task_manager == nullptr)
    return INVALID;

  protected_task_manager->DoGoto(wp);
  TaskEvents task_events;
  const OrderedTask blank(CommonInterface::GetComputerSettings().task);
  protected_task_manager->task_commit(blank);

  return SUCCESS;
}

static unsigned
ordered_task_size()
{
  assert(protected_task_manager != nullptr);
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  const OrderedTask &ot = task_manager->get_ordered_task();
  if (ot.check_task())
    return ot.TaskSize();

  return 0;
}

static void
OnGotoAndClearTaskClicked()
{
  if (protected_task_manager == nullptr)
    return;

  if ((ordered_task_size() > 2) && ShowMessageBox(_("Clear current task?"),
                        _("Goto and clear task"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  switch (goto_and_clear_task(*waypoint)) {
  case SUCCESS:
    protected_task_manager->TaskSaveDefault();
    form->SetModalResult(mrOK);
    break;
  case NOTASK:
  case UNMODIFIED:
  case INVALID:
    ShowMessageBox(_("Unknown error creating task."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  }
}
#endif

static void
OnImagePaint(gcc_unused WndOwnerDrawFrame *sender, Canvas &canvas)
{
  canvas.ClearWhite();
  if (page >= 3 && page < 3 + (int)images.size()) {
    Bitmap &img = images[page-3];
    static constexpr int zoom_factors[] = { 1, 2, 4, 8, 16, 32 };
    RasterPoint img_pos, screen_pos;
    PixelSize screen_size;
    PixelSize img_size = img.GetSize();
    fixed scale = std::min((fixed)canvas.GetWidth() / (fixed)img_size.cx,
                           (fixed)canvas.GetHeight() / (fixed)img_size.cy) *
                  zoom_factors[zoom];

    // centered image and optionally zoomed into the center of the image
    fixed scaled_size = img_size.cx * scale;
    if (scaled_size <= (fixed)canvas.GetWidth()) {
      img_pos.x = 0;
      screen_pos.x = (int) (((fixed)canvas.GetWidth() - scaled_size) / 2);
      screen_size.cx = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.GetWidth() / scale;
      img_pos.x = (int) (((fixed)img_size.cx - scaled_size) / 2);
      img_size.cx = (int) scaled_size;
      screen_pos.x = 0;
      screen_size.cx = canvas.GetWidth();
    }
    scaled_size = img_size.cy * scale;
    if (scaled_size <= (fixed)canvas.GetHeight()) {
      img_pos.y = 0;
      screen_pos.y = (int) (((fixed)canvas.GetHeight() - scaled_size) / 2);
      screen_size.cy = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.GetHeight() / scale;
      img_pos.y = (int) (((fixed)img_size.cy - scaled_size) / 2);
      img_size.cy = (int) scaled_size;
      screen_pos.y = 0;
      screen_size.cy = canvas.GetHeight();
    }
    canvas.Stretch(screen_pos.x, screen_pos.y, screen_size.cx, screen_size.cy,
                   img, img_pos.x, img_pos.y, img_size.cx, img_size.cy);
  }
}

#ifdef ANDROID

// TODO: support other platforms

class WaypointExternalFileListHandler final
  : public ListItemRenderer, public ListCursorHandler {
public:
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;
};

void
WaypointExternalFileListHandler::OnActivateItem(unsigned i)
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);

  TCHAR path[MAX_PATH];
  LocalPath(path, file->c_str());

  native_view->openFile(path);
}

void
WaypointExternalFileListHandler::OnPaintItem(Canvas &canvas,
                                             const PixelRect paint_rc,
                                             unsigned i)
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);
  canvas.DrawText(paint_rc.left + Layout::GetTextPadding(),
                  paint_rc.top + Layout::GetTextPadding(),
                  file->c_str());
}
#endif

static constexpr CallBackTableEntry CallBackTable[] = {
    DeclareCallBackEntry(OnMagnifyClicked),
    DeclareCallBackEntry(OnShrinkClicked),
    DeclareCallBackEntry(OnNextClicked),
    DeclareCallBackEntry(OnPrevClicked),
    DeclareCallBackEntry(OnGotoClicked),
    DeclareCallBackEntry(OnImagePaint),
    DeclareCallBackEntry(nullptr)
};

static void
UpdateCaption()
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: %s"), _("Waypoint"), waypoint->name.c_str());

  const char *key = nullptr;
  switch (waypoint->file_num) {
  case 1:
    key = ProfileKeys::WaypointFile;
    break;
  case 2:
    key = ProfileKeys::AdditionalWaypointFile;
    break;
  case 3:
    key = ProfileKeys::WatchedWaypointFile;
    break;
  }

  if (key != nullptr) {
    const TCHAR *filename = Profile::GetPathBase(key);
    if (filename != nullptr)
      buffer.AppendFormat(_T(" (%s)"), filename);
  }

  form->SetCaption(buffer);
}

void 
dlgWaypointDetailsShowModal(const Waypoint &_waypoint,
                            bool allow_navigation)
{
  waypoint = &_waypoint;

  form = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_WAYPOINTDETAILS_L") :
                                      _T("IDR_XML_WAYPOINTDETAILS"));
  assert(form != nullptr);

  LastUsedWaypoints::Add(_waypoint);

  UpdateCaption();

  form->SetKeyDownFunction(FormKeyDown);

  info_widget = (DockWindow *)form->FindByName(_T("info"));
  assert(info_widget != nullptr);
  info_widget->SetWidget(new WaypointInfoWidget(UIGlobals::GetDialogLook(),
                                          _waypoint));

  commands_widget = (DockWindow *)form->FindByName(_T("commands"));
  assert(commands_widget != nullptr);
  commands_widget->SetWidget(new WaypointCommandsWidget(UIGlobals::GetDialogLook(),
                                                 form, _waypoint,
                                                 protected_task_manager));
  commands_widget->Hide();

  details_panel = form->FindByName(_T("frmDetails"));
  assert(details_panel != nullptr);

  ListControl *wFilesList = (ListControl *)form->FindByName(_T("Files"));
  assert(wFilesList != nullptr);

  LargeTextWindow *wDetailsText = (LargeTextWindow *)
    form->FindByName(_T("Details"));
  assert(wDetailsText != nullptr);
  wDetailsText->SetText(waypoint->details.c_str());

#ifdef ANDROID
  WaypointExternalFileListHandler handler;
  int num_files = std::distance(waypoint->files_external.begin(),
                                waypoint->files_external.end());
  if (num_files > 0) {
    wFilesList->SetItemRenderer(&handler);
    wFilesList->SetCursorHandler(&handler);

    unsigned list_height = wFilesList->GetItemHeight() * std::min(num_files, 5);
    wFilesList->Resize(wFilesList->GetWidth(), list_height);
    wFilesList->SetLength(num_files);

    PixelRect rc = wDetailsText->GetPosition();
    rc.top += list_height;
    wDetailsText->Move(rc);
  } else
#endif
    wFilesList->Hide();

  image_window = (PaintWindow *)form->FindByName(_T("frmImage"));
  assert(image_window != nullptr);
  magnify_button = (WndButton *)form->FindByName(_T("cmdMagnify"));
  assert(magnify_button != nullptr);
  shrink_button = (WndButton *)form->FindByName(_T("cmdShrink"));
  assert(shrink_button != nullptr);

  if (!allow_navigation) {
    for (const TCHAR *button_name :
         { _T("cmdPrev"), _T("cmdNext"), _T("cmdGoto") }) {
      Window *button = form->FindByName(button_name);
      assert(button != nullptr);
      button->Hide();
    }
  }

  for (auto it = waypoint->files_embed.begin(),
       it_end = waypoint->files_embed.end();
       it != it_end && !images.full(); it++) {
    TCHAR path[MAX_PATH];
    LocalPath(path, it->c_str());
    if (!images.append().LoadFile(path))
      images.shrink(images.size() - 1);
  }

  last_page = 2 + images.size();

  page = 0;
  UpdatePage();

  form->ShowModal();

  delete form;

  for (auto image = images.begin(); image < images.end(); image++)
    image->Reset();

  images.clear();
}
