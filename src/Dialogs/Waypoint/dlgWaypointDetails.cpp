// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "WaypointInfoWidget.hpp"
#include "WaypointCommandsWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Panel.hpp"
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
#include "ui/control/LargeTextWindow.hpp"
#include "ui/control/List.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Profile/Current.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "system/RunFile.hpp"
#include "system/Path.hpp"
#include "system/ConvertPathName.hpp"
#include "io/CupxArchive.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "LogFile.hpp"
#include "util/StringPointer.hxx"
#include "util/AllocatedString.hxx"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Protection.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Pan.hpp"

#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

static bool
ActivatePan(const Waypoint &waypoint)
{
  return PanTo(waypoint.location);
}

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

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
WaypointExternalFileListHandler::OnActivateItem(unsigned i) noexcept
{
  auto file = waypoint->files_external.begin();
  std::advance(file, i);

#ifdef ANDROID
  /* on Android, the ContentProvider API needs to be used to give
     other apps access to this file */
  native_view->OpenWaypointFile(Java::GetEnv(), waypoint->id, file->c_str());
#else
  RunFile(LocalPath(file->c_str()).c_str());
#endif
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

class DrawPanFrame final : public WndOwnerDrawFrame {
  protected:
    PixelPoint last_mouse_pos, img_pos, offset;
    bool is_dragging = false;

  std::function<void(Canvas &canvas, const PixelRect &rc, PixelPoint &offset,
    PixelPoint &img_pos)> mOnPaintCallback2;

  public:
    template<typename CB>
    void Create(ContainerWindow &parent,
                PixelRect rc, const WindowStyle style,
                  CB &&_paint) {
      mOnPaintCallback2 = std::move(_paint);
      PaintWindow::Create(parent, rc, style);
    }


  protected:
    /** from class Window */
    bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
    bool OnMouseDown(PixelPoint p) noexcept override;
    bool OnMouseUp(PixelPoint p) noexcept override;

    bool OnKeyCheck(unsigned key_code) const noexcept override;
    bool OnKeyDown(unsigned key_code) noexcept override;

  void
  OnPaint(Canvas &canvas) noexcept override
  {
    if (mOnPaintCallback2 == nullptr)
      return;

    mOnPaintCallback2(canvas, GetClientRect(), offset, img_pos);
  }
};

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

  StaticString<256> base_caption;

  AllocatedPath source_path{nullptr};

  ManagedWidget info_widget{new WaypointInfoWidget(look, waypoint)};
  PanelControl details_panel;
  ManagedWidget commands_widget;
  DrawPanFrame image_window;

#ifdef HAVE_RUN_FILE
  ListControl file_list{look};
  WaypointExternalFileListHandler file_list_handler{waypoint};
#endif

  LargeTextWindow details_text;

  StaticArray<Bitmap, 5> images;
  int zoom = 0;

public:
  WaypointDetailsWidget(WidgetDialog &_dialog,
                        Waypoints *waypoints, WaypointPtr _waypoint,
                        ProtectedTaskManager *_task_manager, bool allow_edit) noexcept
    :dialog(_dialog),
     waypoint(std::move(_waypoint)),
     task_manager(_task_manager),
     commands_widget(new WaypointCommandsWidget(look, &dialog, waypoints, waypoint,
                                                task_manager, allow_edit)) {}

  /**
   * Resolve the source file path for this waypoint from its
   * origin and file_num fields.
   */
  [[gnu::pure]]
  AllocatedPath GetSourcePath() const noexcept;

  void InitCaption() noexcept;
  void UpdateCaption() noexcept;
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

  void OnImagePaint(Canvas &canvas, const PixelRect &rc, PixelPoint &offset,
    PixelPoint &img_pos);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                        file_list_handler.GetRowRenderer(),
#endif
                        *waypoint);

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
    goto_button.SetFocus();
    return true;
  }

  bool HasFocus() const noexcept override {
    return (task_manager != nullptr && goto_button.HasFocus()) ||
      (!images.empty() && (magnify_button.HasFocus() ||
                           shrink_button.HasFocus())) ||
       previous_button.HasFocus() || next_button.HasFocus() ||
       close_button.HasFocus() ||
       info_widget.HasFocus() ||
       details_panel.HasFocus() || details_text.HasFocus() ||
#ifdef HAVE_RUN_FILE
       (!waypoint->files_external.empty() && file_list.HasFocus()) ||
#endif
       commands_widget.HasFocus() ||
       (!images.empty() && image_window.HasFocus());
  }

  bool KeyPress(unsigned key_code) noexcept override;
};

WaypointDetailsWidget::Layout::Layout(const PixelRect &rc,
#ifdef HAVE_RUN_FILE
                                      TextRowRenderer &row_renderer,
#endif
                                      [[maybe_unused]] const Waypoint &waypoint) noexcept
{
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  main = rc;

  if (width > height) {
    auto buttons = main.CutLeftSafe(::Layout::Scale(70));

    goto_button = buttons.CutTopSafe(button_height);
    std::tie(magnify_button, shrink_button) = buttons.CutTopSafe(button_height).VerticalSplit();

    close_button = buttons.CutBottomSafe(button_height);

    std::tie(previous_button, next_button) = buttons.CutBottomSafe(button_height).VerticalSplit();
  } else {
    auto buttons = main.CutBottomSafe(button_height);

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

    unsigned list_height = file_list_item_height * std::min(num_files, 5u);
    file_list = details_text.CutTopSafe(list_height);
  }
#endif
}

void
WaypointDetailsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const bool is_cupx = source_path != nullptr &&
    source_path.EndsWithIgnoreCase(".cupx");

  for (const auto &i : waypoint->files_embed) {
    if (images.full())
      break;

    try {
      if (is_cupx) {
        auto data = CupxArchive::ExtractImage(source_path, i);
        if (data.empty())
          continue;

#if !defined(USE_GDI) && !defined(ANDROID)
        if (!images.append().Load(std::span<const std::byte>(data)))
          images.shrink(images.size() - 1);
#else
        {
          const auto tmp_dir = MakeCacheDirectory("cupx");
          const auto tmp_file = AllocatedPath::Build(tmp_dir, i.c_str());

          FileOutputStream fos(tmp_file,
                               FileOutputStream::Mode::CREATE_VISIBLE);
          fos.Write(std::as_bytes(std::span{data}));
          fos.Commit();

          if (!images.append().LoadFile(tmp_file))
            images.shrink(images.size() - 1);

          File::Delete(tmp_file);
        }
#endif
      } else {
        if (!images.append().LoadFile(LocalPath(i.c_str())))
          images.shrink(images.size() - 1);
      }
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
  else {
	goto_button.Create(parent, look.button, _("Pan To"), layout.goto_button,
                       button_style, [this](){
    if (ActivatePan(*waypoint))
      dialog.SetModalResult(mrOK);
  });  
  }

  if (!images.empty()) {
    magnify_button.Create(parent, layout.magnify_button, button_style,
                          std::make_unique<SymbolButtonRenderer>(look.button, "+"),
                          [this](){ OnMagnifyClicked(); });
    shrink_button.Create(parent, layout.shrink_button, button_style,
                         std::make_unique<SymbolButtonRenderer>(look.button, "-"),
                         [this](){ OnShrinkClicked(); });
  }

  previous_button.Create(parent, layout.previous_button, button_style,
                         std::make_unique<SymbolButtonRenderer>(look.button, "<"),
                         [this](){ NextPage(-1); });

  next_button.Create(parent, layout.next_button, button_style,
                     std::make_unique<SymbolButtonRenderer>(look.button, ">"),
                     [this](){ NextPage(1); });

  close_button.Create(parent, look.button, _("Close"), layout.close_button,
                      button_style, dialog.MakeModalResultCallback(mrOK));

  info_widget.Initialise(parent, layout.main);
  info_widget.Prepare();

  details_panel.Create(parent, look, layout.main, dock_style);
  details_text.Create(details_panel, layout.details_text);
#ifndef USE_WINUSER
  details_text.SetFont(look.text_font);
#endif
  details_text.SetColors(look.background_color, look.text_color,
                         look.dark_mode ? COLOR_GRAY : COLOR_BLACK);
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
                        [this](Canvas &canvas, const PixelRect &rc, PixelPoint offset,
                          PixelPoint &img_pos) {
                                 OnImagePaint(canvas, rc, offset, img_pos);
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

  UpdateCaption();
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
  if (!images.empty())
    image_window.Invalidate();

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
}

void
WaypointDetailsWidget::OnShrinkClicked()
{
  if (zoom <= 0)
    return;
  zoom--;

  UpdateZoomControls();
}

bool
WaypointDetailsWidget::KeyPress(unsigned key_code) noexcept {
  switch (key_code) {
  case KEY_F1:
    if (!images.empty() && image_window.IsVisible()) {
      magnify_button.SetFocus();
      magnify_button.Click();
      image_window.Invalidate();
      image_window.SetFocus();
      return true;
    }
    return false;

  case KEY_F2:
    if (!images.empty() && image_window.IsVisible()) {
      shrink_button.SetFocus();
      shrink_button.Click();
      if (zoom == 0) {
        next_button.SetFocus();
      } else {
        image_window.Invalidate();
        image_window.SetFocus();
      }
      return true;
    }
    return false;

  case KEY_LEFT:
    if (zoom == 0) {
      previous_button.SetFocus();
      NextPage(-1);
    }
    return false;

  case KEY_RIGHT:
    if (zoom == 0) {
      next_button.SetFocus();
      NextPage(+1);
      return true;
    }
    return false;

    case KEY_ESCAPE:
      if (!images.empty() && zoom > 0) {
        zoom = 0;
        image_window.Invalidate();
        shrink_button.SetEnabled(false);
        goto_button.SetFocus();
        return true;
      }
      return false;

    case KEY_UP:
      if (!images.empty() && image_window.IsVisible() && goto_button.HasFocus()) {
        close_button.SetFocus();
        return true;
      }
      return false;

    case KEY_DOWN:
      if (!images.empty() && image_window.IsVisible() && close_button.HasFocus()) {
        goto_button.SetFocus();
        return true;
      }
      return false;

    default:
      return false;
    }
}

void
WaypointDetailsWidget::OnGotoClicked()
{
  if (task_manager == nullptr)
    return;

  // Remove old temporary goto waypoint when selecting a regular waypoint
  if (data_components != nullptr && data_components->waypoints != nullptr) {
    auto &way_points = *data_components->waypoints;
    {
      ScopeSuspendAllThreads suspend;
      way_points.EraseTempGoto();
    }
  }

  task_manager->DoGoto(waypoint);
  dialog.SetModalResult(mrOK);

  CommonInterface::main_window->FullRedraw();
}

void
WaypointDetailsWidget::OnImagePaint(Canvas &canvas, [[maybe_unused]] const PixelRect &rc,
                  PixelPoint &offset, PixelPoint &img_pos)
{
  const auto &dlook = UIGlobals::GetDialogLook();
  canvas.Clear(dlook.background_color);

  if (page < 3 || page >= 3 + static_cast<int>(images.size())) {
    return;
  }

  Bitmap &img = images[page - 3];
  static constexpr int zoom_factors[] = {1, 2, 4, 8, 16, 32};
  PixelSize img_size = img.GetSize();
  double scale = std::min(static_cast<double>(canvas.GetWidth()) / img_size.width,
                          static_cast<double>(canvas.GetHeight()) / img_size.height) *
                 zoom_factors[zoom];

  PixelPoint screen_pos;
  PixelSize screen_size;

  // Calculate horizontal scaling and positioning
  double scaled_width = img_size.width * scale;
  if (scaled_width <= canvas.GetWidth()) {
    img_pos.x = zoom == 0 ? 0 : img_pos.x + offset.x / scale;
    screen_pos.x = (canvas.GetWidth() - static_cast<int>(scaled_width)) / 2;
    screen_size.width = static_cast<unsigned>(scaled_width);
  } else {
    double visible_width = canvas.GetWidth() / scale;
    img_pos.x = zoom == 0 ? (img_size.width - visible_width) / 2 : img_pos.x + offset.x / scale;
    img_pos.x = std::clamp(img_pos.x, 0, static_cast<int>(img_size.width - visible_width));
    img_size.width = static_cast<unsigned>(visible_width);
    screen_pos.x = 0;
    screen_size.width = canvas.GetWidth();
  }

  // Calculate vertical scaling and positioning
  double scaled_height = img_size.height * scale;
  if (scaled_height <= canvas.GetHeight()) {
    img_pos.y = 0;
    screen_pos.y = (canvas.GetHeight() - static_cast<int>(scaled_height)) / 2;
    screen_size.height = static_cast<unsigned>(scaled_height);
  } else {
    double visible_height = canvas.GetHeight() / scale;
    img_pos.y = zoom == 0 ? (img_size.height - visible_height) / 2 : img_pos.y + offset.y / scale;
    img_pos.y = std::clamp(img_pos.y, 0, static_cast<int>(img_size.height - visible_height));
    img_size.height = static_cast<unsigned>(visible_height);
    screen_pos.y = 0;
    screen_size.height = canvas.GetHeight();
  }
  canvas.Stretch(screen_pos, screen_size, img, img_pos, img_size);
}

bool
DrawPanFrame::OnMouseMove(PixelPoint p, [[maybe_unused]] unsigned keys) noexcept
{
  if (!is_dragging)
    return false;

  offset = last_mouse_pos - p;
  last_mouse_pos = p;
  Invalidate();
  return true;
}

bool
DrawPanFrame::OnMouseDown(PixelPoint p) noexcept
{
  is_dragging = true;
  last_mouse_pos = p;
  return true;
}

bool
DrawPanFrame::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  is_dragging = false;
  return true;
}

bool
DrawPanFrame::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_UP:
    case KEY_DOWN:
    return true;

  default:
    return false;
  }
}

bool
DrawPanFrame::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {
    case KEY_LEFT: {
      offset = {-50, 0};
      break;
    }
    case KEY_RIGHT: {
      offset = {50, 0};
      break;
    }
    case KEY_UP: {
      offset = {0, -50};
      break;
    }
    case KEY_DOWN: {
      offset = {0, 50};
      break;
    }
    default:
      return false;
  }
  Invalidate();
  return true;
}

/**
 * Map a WaypointOrigin to the profile key that stores its source
 * file list, or return an empty view for origins without a key.
 */
static std::string_view
OriginToProfileKey(WaypointOrigin origin) noexcept
{
  switch (origin) {
  case WaypointOrigin::PRIMARY:
    return ProfileKeys::WaypointFileList;
  case WaypointOrigin::WATCHED:
    return ProfileKeys::WatchedWaypointFileList;
  case WaypointOrigin::MAP:
    return ProfileKeys::MapFile;
  default:
    return {};
  }
}

AllocatedPath
WaypointDetailsWidget::GetSourcePath() const noexcept
{
  const auto key = OriginToProfileKey(waypoint->origin);
  if (key.empty())
    return {};

  auto paths = Profile::GetMultiplePaths(key, nullptr);
  if (waypoint->file_num < paths.size())
    return std::move(paths[waypoint->file_num]);

  return {};
}

void
WaypointDetailsWidget::InitCaption() noexcept
{
  source_path = GetSourcePath();

  base_caption.Format("%s: %s", _("Waypoint"), waypoint->name.c_str());

  if (source_path != nullptr) {
    const auto filename = source_path.GetBase();
    if (filename != nullptr)
      base_caption.AppendFormat(" (%s)", filename.c_str());
  } else if (waypoint->origin == WaypointOrigin::USER) {
    base_caption.AppendFormat(" (%s)", "user.cup");
  }
}

void
WaypointDetailsWidget::UpdateCaption() noexcept
{
  if (last_page == 0) {
    dialog.SetCaption(base_caption);
    return;
  }

  const bool details_skipped =
#ifdef HAVE_RUN_FILE
    waypoint->files_external.empty() &&
#endif
    waypoint->details.empty();

  const int total_pages = last_page + 1 - (details_skipped ? 1 : 0);

  int logical_page = page + 1;
  if (details_skipped && page > 1)
    --logical_page;

  StaticString<256> caption;
  caption.Format("%s (%d/%d)", base_caption.c_str(),
                 logical_page, total_pages);
  dialog.SetCaption(caption);
}

void
dlgWaypointDetailsShowModal(Waypoints *waypoints, WaypointPtr _waypoint,
                            bool allow_navigation, bool allow_edit)
{
  LastUsedWaypoints::Add(*_waypoint);

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WaypointDetailsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, nullptr);
  dialog.SetWidget(dialog, waypoints, _waypoint,
                   allow_navigation ? backend_components->protected_task_manager.get() : nullptr,
                   allow_edit);

  dialog.GetWidget().InitCaption();
  dialog.GetWidget().UpdateCaption();

  dialog.ShowModal();
}
