/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Panel.hpp"
#include "Form/List.hpp"
#include "Form/Draw.hpp"
#include "Form/Button.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Widget/ManagedWidget.hpp"
#include "Widget/Widget.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "LocalPath.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "util/Compiler.h"
#include "Language/Language.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Profile/Current.hpp"
#include "Profile/Map.hpp"
#include "Profile/ProfileKeys.hpp"
#include "system/RunFile.hpp"
#include "system/Path.hpp"
#include "system/ConvertPathName.hpp"
#include "LogFile.hpp"
#include "util/StringPointer.hxx"
#include "util/AllocatedString.hxx"

#include <cassert>

#ifdef HAVE_RUN_FILE

class WaypointExternalFileListHandler final
  : public ListItemRenderer, public ListCursorHandler {
  const WaypointPtr waypoint;

  TextRowRenderer row_renderer;

public:
  explicit WaypointExternalFileListHandler(WaypointPtr _waypoint)
    :waypoint(std::move(_waypoint)) {}

  auto &GetRowRenderer() noexcept {
    return row_renderer;
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  bool CanActivateItem(gcc_unused unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
WaypointExternalFileListHandler::OnActivateItem(unsigned i) noexcept
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);

  RunFile(LocalPath(file->c_str()).c_str());
}

void
WaypointExternalFileListHandler::OnPaintItem(Canvas &canvas,
                                             const PixelRect paint_rc,
                                             unsigned i) noexcept
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);
  row_renderer.DrawTextRow(canvas, paint_rc, file->c_str());
}
#endif

class WaypointDetailsWidget final
  : public NullWidget {
  struct Layout {
    PixelRect goto_button;
    PixelRect magnify_button, shrink_button;
    PixelRect previous_button, next_button;
    PixelRect close_button;
    PixelRect main;

    PixelRect details_text;

#ifdef HAVE_RUN_FILE
    unsigned file_list_item_height;
    PixelRect file_list;
#endif

    explicit Layout(const PixelRect &rc,
#ifdef HAVE_RUN_FILE
                    TextRowRenderer &row_renderer,
#endif
                    const Waypoint &waypoint) noexcept;
  };

  WidgetDialog &dialog;
  const DialogLook &look{dialog.GetLook()};

  const WaypointPtr waypoint;

  ProtectedTaskManager *const task_manager;

  Button goto_button;
  Button magnify_button, shrink_button;
  Button previous_button, next_button;
  Button close_button;

  int page = 0, last_page = 0;

  ManagedWidget info_widget{new WaypointInfoWidget(look, waypoint)};
  PanelControl details_panel;
  ManagedWidget commands_widget;
  WndOwnerDrawFrame image_window;

#ifdef HAVE_RUN_FILE
  ListControl file_list{look};
  WaypointExternalFileListHandler file_list_handler{waypoint};
#endif

  LargeTextWindow details_text;

  StaticArray<Bitmap, 5> images;
  int zoom = 0;

public:
  WaypointDetailsWidget(WidgetDialog &_dialog, WaypointPtr _waypoint,
                        ProtectedTaskManager *_task_manager, bool allow_edit) noexcept
    :dialog(_dialog),
     waypoint(std::move(_waypoint)),
     task_manager(_task_manager),
     commands_widget(new WaypointCommandsWidget(look, &dialog, waypoint,
                                                task_manager, allow_edit)) {}

  void UpdatePage() noexcept;
  void UpdateZoomControls();

  void NextPage(int step);

  void OnNextClicked() {
    NextPage(+1);
  }

  void OnPrevClicked() {
    NextPage(-1);
  }

  void OnMagnifyClicked();
  void OnShrinkClicked();

  void OnGotoClicked();

  void OnImagePaint(Canvas &canvas, const PixelRect &rc);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                        file_list_handler.GetRowRenderer(),
#endif
                        *waypoint);

    if (task_manager != nullptr)
      goto_button.MoveAndShow(layout.goto_button);

    if (!images.empty()) {
      magnify_button.MoveAndShow(layout.magnify_button);
      shrink_button.MoveAndShow(layout.shrink_button);
    }

    previous_button.MoveAndShow(layout.previous_button);
    next_button.MoveAndShow(layout.next_button);

    close_button.MoveAndShow(layout.close_button);

    info_widget.Move(layout.main);
    details_panel.Move(layout.main);
    details_text.Move(layout.details_text);
#ifdef HAVE_RUN_FILE
    if (!waypoint->files_external.empty())
      file_list.Move(layout.file_list);
#endif

    commands_widget.Move(layout.main);

    if (!images.empty())
      image_window.Move(layout.main);

    UpdatePage();
  }

  void Hide() noexcept override {
    if (task_manager != nullptr)
      goto_button.Hide();

    if (!images.empty()) {
      magnify_button.Hide();
      shrink_button.Hide();
    }

    previous_button.Hide();
    next_button.Hide();

    close_button.Hide();

    info_widget.Hide();

    details_panel.Hide();
    commands_widget.Hide();

    if (!images.empty())
      image_window.Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                        file_list_handler.GetRowRenderer(),
#endif
                        *waypoint);

    if (task_manager != nullptr)
      goto_button.Move(layout.goto_button);

    if (!images.empty()) {
      magnify_button.Move(layout.magnify_button);
      shrink_button.Move(layout.shrink_button);
    }

    previous_button.Move(layout.previous_button);
    next_button.Move(layout.next_button);

    close_button.Move(layout.close_button);

    info_widget.Move(layout.main);
    details_panel.Move(layout.main);
    details_text.Move(layout.details_text);
#ifdef HAVE_RUN_FILE
    if (!waypoint->files_external.empty())
      file_list.Move(layout.file_list);
#endif
    commands_widget.Move(layout.main);

    if (!images.empty())
      image_window.Move(layout.main);
  }

  bool SetFocus() noexcept override {
    if (task_manager != nullptr) {
      goto_button.SetFocus();
      return true;
    } else
      return false;
  }

  bool KeyPress(unsigned key_code) noexcept override;
};

WaypointDetailsWidget::Layout::Layout(const PixelRect &rc,
#ifdef HAVE_RUN_FILE
                                      TextRowRenderer &row_renderer,
#endif
                                      const Waypoint &waypoint) noexcept
{
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  main = rc;

  if (width > height) {
    main.left += ::Layout::Scale(70);

    PixelRect buttons = rc;
    buttons.right = main.left;

    goto_button = buttons;
    goto_button.bottom = buttons.top += button_height;

    magnify_button = buttons;
    magnify_button.bottom = buttons.top += button_height;

    shrink_button = magnify_button;
    magnify_button.right = shrink_button.left =
      (buttons.left + buttons.right) / 2;

    close_button = buttons;
    close_button.top = buttons.bottom -= button_height;

    previous_button = buttons;
    previous_button.top = buttons.bottom -= button_height;
    next_button = previous_button;
    previous_button.right = next_button.left =
      (buttons.left + buttons.right) / 2;
  } else {
    main.bottom -= button_height;

    PixelRect buttons = rc;
    buttons.top = main.bottom;

    const unsigned one_third = (2 * buttons.left + buttons.right) / 3;
    const unsigned two_thirds = (buttons.left + 2 * buttons.right) / 3;

    goto_button = buttons;
    goto_button.right = one_third;

    close_button = buttons;
    close_button.left = two_thirds;

    previous_button = buttons;
    previous_button.left = one_third;
    next_button = buttons;
    next_button.right = two_thirds;
    previous_button.right = next_button.left = (one_third + two_thirds) / 2;

    const unsigned padding = ::Layout::GetTextPadding();
    shrink_button.left = main.left + padding;
    shrink_button.top = main.top + padding;
    shrink_button.right = shrink_button.left + button_height;
    shrink_button.bottom = shrink_button.top + button_height;

    magnify_button.right = main.right - padding;
    magnify_button.top = main.top + padding;
    magnify_button.left = magnify_button.right - button_height;
    magnify_button.bottom = magnify_button.top + button_height;
  }

  details_text.left = 0;
  details_text.top = 0;
  details_text.right = main.GetWidth();
  details_text.bottom = main.GetHeight();

#ifdef HAVE_RUN_FILE
  const unsigned num_files = std::distance(waypoint.files_external.begin(),
                                           waypoint.files_external.end());
  if (num_files > 0) {
    file_list_item_height = row_renderer.CalculateLayout(*UIGlobals::GetDialogLook().list.font);
    file_list = details_text;

    unsigned list_height = file_list_item_height * std::min(num_files, 5u);
    file_list.bottom = details_text.top += list_height;
  }
#endif
}

void
WaypointDetailsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  for (const auto &i : waypoint->files_embed) {
    if (images.full())
      break;

    try {
      if (!images.append().LoadFile(LocalPath(i.c_str())))
        images.shrink(images.size() - 1);
    } catch (const std::exception &e) {
      LogFormat("Failed to load %s: %s",
                (const char *)NarrowPathName(Path(i.c_str())),
                e.what());
      images.shrink(images.size() - 1);
    }
  }

  const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                      file_list_handler.GetRowRenderer(),
#endif
                      *waypoint);

  WindowStyle dock_style;
  dock_style.Hide();
  dock_style.ControlParent();

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  if (task_manager != nullptr)
    goto_button.Create(parent, look.button, _("GoTo"), layout.goto_button,
                       button_style, [this](){ OnGotoClicked(); });

  if (!images.empty()) {
    magnify_button.Create(parent, layout.magnify_button, button_style,
                          std::make_unique<SymbolButtonRenderer>(look.button, _T("+")),
                          [this](){ OnMagnifyClicked(); });
    shrink_button.Create(parent, layout.shrink_button, button_style,
                         std::make_unique<SymbolButtonRenderer>(look.button, _T("-")),
                         [this](){ OnShrinkClicked(); });
  }

  previous_button.Create(parent, layout.previous_button, button_style,
                         std::make_unique<SymbolButtonRenderer>(look.button, _T("<")),
                         [this](){ NextPage(-1); });

  next_button.Create(parent, layout.next_button, button_style,
                     std::make_unique<SymbolButtonRenderer>(look.button, _T(">")),
                     [this](){ NextPage(1); });

  close_button.Create(parent, look.button, _("Close"), layout.close_button,
                      button_style, dialog.MakeModalResultCallback(mrOK));

  info_widget.Initialise(parent, layout.main);
  info_widget.Prepare();

  details_panel.Create(parent, look, layout.main, dock_style);
  details_text.Create(details_panel, layout.details_text);
  details_text.SetFont(look.text_font);
  details_text.SetText(waypoint->details.c_str());

#ifdef HAVE_RUN_FILE
  const unsigned num_files = std::distance(waypoint->files_external.begin(),
                                           waypoint->files_external.end());
  if (num_files > 0) {
    file_list.Create(details_panel, layout.file_list,
                     WindowStyle(), layout.file_list_item_height);
    file_list.SetItemRenderer(&file_list_handler);
    file_list.SetCursorHandler(&file_list_handler);
    file_list.SetLength(num_files);
  }
#endif

  commands_widget.Initialise(parent, layout.main);
  commands_widget.Prepare();

  if (!images.empty())
    image_window.Create(parent, layout.main, dock_style,
                        [this](Canvas &canvas, const PixelRect &rc){
                          OnImagePaint(canvas, rc);
                        });

  last_page = 2 + images.size();
}

void
WaypointDetailsWidget::Unprepare() noexcept
{
  info_widget.Unprepare();
  commands_widget.Unprepare();
}

void
WaypointDetailsWidget::UpdatePage() noexcept
{
  info_widget.SetVisible(page == 0);
  details_panel.SetVisible(page == 1);
  commands_widget.SetVisible(page == 2);

  bool image_page = page >= 3;
  if (!images.empty()) {
    image_window.SetVisible(image_page);
    magnify_button.SetVisible(image_page);
    shrink_button.SetVisible(image_page);
  }
}

void
WaypointDetailsWidget::UpdateZoomControls()
{
  magnify_button.SetEnabled(zoom < 5);
  shrink_button.SetEnabled(zoom > 0);
}

void
WaypointDetailsWidget::NextPage(int step)
{
  assert(last_page > 0);

  do {
    page += step;
    if (page < 0)
      page = last_page;
    else if (page > last_page)
      page = 0;
    // skip wDetails frame, if there are no details
  } while (page == 1 &&
#ifdef HAVE_RUN_FILE
           waypoint->files_external.empty() &&
#endif
           waypoint->details.empty());

  UpdatePage();

  if (page >= 3) {
    zoom = 0;
    UpdateZoomControls();
  }
}

void
WaypointDetailsWidget::OnMagnifyClicked()
{
  if (zoom >= 5)
    return;
  zoom++;

  UpdateZoomControls();
  image_window.Invalidate();
}

void
WaypointDetailsWidget::OnShrinkClicked()
{
  if (zoom <= 0)
    return;
  zoom--;

  UpdateZoomControls();
  image_window.Invalidate();
}

bool
WaypointDetailsWidget::KeyPress(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_LEFT:
    previous_button.SetFocus();
    NextPage(-1);
    return true;

  case KEY_RIGHT:
    next_button.SetFocus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

void
WaypointDetailsWidget::OnGotoClicked()
{
  if (task_manager == nullptr)
    return;

  task_manager->DoGoto(waypoint);
  dialog.SetModalResult(mrOK);

  CommonInterface::main_window->FullRedraw();
}

void
WaypointDetailsWidget::OnImagePaint(gcc_unused Canvas &canvas,
                                    gcc_unused const PixelRect &rc)
{
  canvas.ClearWhite();
  if (page >= 3 && page < 3 + (int)images.size()) {
    Bitmap &img = images[page-3];
    static constexpr int zoom_factors[] = { 1, 2, 4, 8, 16, 32 };
    PixelPoint img_pos, screen_pos;
    PixelSize screen_size;
    PixelSize img_size = img.GetSize();
    double scale = std::min((double)canvas.GetWidth() / img_size.width,
                            (double)canvas.GetHeight() / img_size.height) *
      zoom_factors[zoom];

    // centered image and optionally zoomed into the center of the image
    double scaled_size = img_size.width * scale;
    if (scaled_size <= canvas.GetWidth()) {
      img_pos.x = 0;
      screen_pos.x = (int) ((canvas.GetWidth() - scaled_size) / 2);
      screen_size.width = (unsigned) scaled_size;
    } else {
      scaled_size = canvas.GetWidth() / scale;
      img_pos.x = (int(img_size.width - scaled_size) / 2);
      img_size.width = (unsigned) scaled_size;
      screen_pos.x = 0;
      screen_size.width = canvas.GetWidth();
    }
    scaled_size = img_size.height * scale;
    if (scaled_size <= canvas.GetHeight()) {
      img_pos.y = 0;
      screen_pos.y = (int) ((canvas.GetHeight() - scaled_size) / 2);
      screen_size.height = (unsigned) scaled_size;
    } else {
      scaled_size = canvas.GetHeight() / scale;
      img_pos.y = (int) ((img_size.height - scaled_size) / 2);
      img_size.height = (unsigned) scaled_size;
      screen_pos.y = 0;
      screen_size.height = canvas.GetHeight();
    }
    canvas.Stretch(screen_pos, screen_size, img, img_pos, img_size);
  }
}

static void
UpdateCaption(WndForm *form, const Waypoint &waypoint)
{
  StaticString<256> buffer;
  buffer.Format(_T("%s: %s"), _("Waypoint"), waypoint.name.c_str());

  const char *key = nullptr;
  const TCHAR *name = nullptr;

  switch (waypoint.origin) {
  case WaypointOrigin::NONE:
    break;

  case WaypointOrigin::USER:
    name = _T("user.cup");
    break;

  case WaypointOrigin::PRIMARY:
    key = ProfileKeys::WaypointFile;
    break;

  case WaypointOrigin::ADDITIONAL:
    key = ProfileKeys::AdditionalWaypointFile;
    break;

  case WaypointOrigin::WATCHED:
    key = ProfileKeys::WatchedWaypointFile;
    break;

  case WaypointOrigin::MAP:
    key = ProfileKeys::MapFile;
    break;
  }

  if (key != nullptr) {
    const auto filename = Profile::map.GetPathBase(key);
    if (filename != nullptr)
      buffer.AppendFormat(_T(" (%s)"), filename.c_str());
  } else if (name != nullptr)
    buffer.AppendFormat(_T(" (%s)"), name);

  form->SetCaption(buffer);
}

void
dlgWaypointDetailsShowModal(WaypointPtr _waypoint,
                            bool allow_navigation, bool allow_edit)
{
  LastUsedWaypoints::Add(*_waypoint);

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WaypointDetailsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, nullptr);
  dialog.SetWidget(dialog, _waypoint,
                   allow_navigation ? protected_task_manager : nullptr,
                   allow_edit);

  UpdateCaption(&dialog, *_waypoint);

  dialog.ShowModal();
}
