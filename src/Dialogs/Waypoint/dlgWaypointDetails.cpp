// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "WaypointInfoWidget.hpp"
#include "WaypointCommandsWidget.hpp"
#include "Simulator.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Panel.hpp"
#include "Form/Button.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Widget/Widget.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ImageZoomView.hpp"
#include "Widget/ImageZoomFrame.hpp"
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
#include "Input/InputEvents.hpp"
#include "util/StringAPI.hxx"

#include <functional>
#include <optional>
#include <tuple>

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

class WaypointDetailsPageWidget final : public NullWidget {
  struct Layout {
    PixelRect text;

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

  const DialogLook &look;
  const WaypointPtr waypoint;

  PanelControl panel;
  LargeTextWindow details_text;

#ifdef HAVE_RUN_FILE
  ListControl file_list{look};
  WaypointExternalFileListHandler file_list_handler{waypoint};
#endif

public:
  WaypointDetailsPageWidget(const DialogLook &_look, WaypointPtr _waypoint) noexcept
    :look(_look), waypoint(std::move(_waypoint)) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;

  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
};

WaypointDetailsPageWidget::Layout::Layout(
  const PixelRect &rc,
#ifdef HAVE_RUN_FILE
  TextRowRenderer &row_renderer,
#endif
  [[maybe_unused]] const Waypoint &waypoint) noexcept
{
  text.left = 0;
  text.top = 0;
  text.right = rc.GetWidth();
  text.bottom = rc.GetHeight();

#ifdef HAVE_RUN_FILE
  const unsigned num_files = std::distance(waypoint.files_external.begin(),
                                           waypoint.files_external.end());
  if (num_files > 0) {
    file_list_item_height = row_renderer.CalculateLayout(*UIGlobals::GetDialogLook().list.font);
    file_list = text.CutTopSafe(file_list_item_height * std::min(num_files, 5u));
  }
#endif
}

void
WaypointDetailsPageWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
{
  WindowStyle panel_style;
  panel_style.Hide();
  panel_style.ControlParent();

  const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                      file_list_handler.GetRowRenderer(),
#endif
                      *waypoint);
  panel.Create(parent, look, rc, panel_style);
  details_text.Create(panel, layout.text);
#ifndef USE_WINUSER
  details_text.SetFont(look.text_font);
#endif
  details_text.SetColors(look.ReadOnlyValueBackground(), look.list.text_color,
                         look.ReadOnlyValueBorderColor());
  details_text.SetText(waypoint->details.c_str());

#ifdef HAVE_RUN_FILE
  const unsigned num_files = std::distance(waypoint->files_external.begin(),
                                           waypoint->files_external.end());
  if (num_files > 0) {
    file_list.Create(panel, layout.file_list, WindowStyle(),
                     layout.file_list_item_height);
    file_list.SetItemRenderer(&file_list_handler);
    file_list.SetCursorHandler(&file_list_handler);
    file_list.SetLength(num_files);
  }
#endif
}

void
WaypointDetailsPageWidget::Show(const PixelRect &rc) noexcept
{
  const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                      file_list_handler.GetRowRenderer(),
#endif
                      *waypoint);
  panel.MoveAndShow(rc);
  details_text.Move(layout.text);
#ifdef HAVE_RUN_FILE
  if (!waypoint->files_external.empty())
    file_list.Move(layout.file_list);
#endif
}

void
WaypointDetailsPageWidget::Hide() noexcept
{
  panel.Hide();
}

void
WaypointDetailsPageWidget::Move(const PixelRect &rc) noexcept
{
  const Layout layout(rc,
#ifdef HAVE_RUN_FILE
                      file_list_handler.GetRowRenderer(),
#endif
                      *waypoint);
  panel.Move(rc);
  details_text.Move(layout.text);
#ifdef HAVE_RUN_FILE
  if (!waypoint->files_external.empty())
    file_list.Move(layout.file_list);
#endif
}

bool
WaypointDetailsPageWidget::SetFocus() noexcept
{
#ifdef HAVE_RUN_FILE
  if (!waypoint->files_external.empty()) {
    file_list.SetFocus();
    return true;
  }
#endif

  details_text.SetFocus();
  return true;
}

bool
WaypointDetailsPageWidget::HasFocus() const noexcept
{
  return details_text.HasFocus()
#ifdef HAVE_RUN_FILE
    || (!waypoint->files_external.empty() && file_list.HasFocus())
#endif
    ;
}

class WaypointImagePageWidget final : public NullWidget {
  struct Layout {
    PixelRect image;
    PixelRect magnify_button, shrink_button;

    explicit Layout(const PixelRect &rc) noexcept;
  };

  const DialogLook &look;
  Bitmap bitmap;
  const std::function<void(int)> change_page;

  ImageZoomFrame image_window;
  Button magnify_button, shrink_button;
  int zoom = 0;
  const int pan_step = ::Layout::Scale(50);
  std::optional<InputEvents::Mode> wptimg_mode;

  void UpdateZoomControls() noexcept;
  void AdjustViewForZoomChange(int old_zoom, int new_zoom) noexcept;
  void ResetZoom() noexcept;
  bool TryWaypointImageKey(unsigned key_code) noexcept;

public:
  WaypointImagePageWidget(const DialogLook &_look, Bitmap &&_bitmap,
                          std::function<void(int)> &&_change_page) noexcept
    :look(_look), bitmap(std::move(_bitmap)),
     change_page(std::move(_change_page)) {}

  void Magnify() noexcept;
  void Shrink() noexcept;
  void OnWaypointImageEvent(const char *misc) noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};

WaypointImagePageWidget::Layout::Layout(const PixelRect &rc) noexcept
  :image(rc)
{
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  if (rc.GetWidth() > rc.GetHeight()) {
    auto buttons = image.CutLeftSafe(::Layout::Scale(70));
    std::tie(magnify_button, shrink_button) =
      buttons.CutTopSafe(button_height).VerticalSplit();
  } else {
    const unsigned padding = ::Layout::GetTextPadding();

    shrink_button.left = image.left + padding;
    shrink_button.top = image.top + padding;
    shrink_button.right = shrink_button.left + button_height;
    shrink_button.bottom = shrink_button.top + button_height;

    magnify_button.right = image.right - padding;
    magnify_button.top = image.top + padding;
    magnify_button.left = magnify_button.right - button_height;
    magnify_button.bottom = magnify_button.top + button_height;
  }
}

void
WaypointImagePageWidget::UpdateZoomControls() noexcept
{
  magnify_button.SetEnabled(zoom < ImageZoomView::max_zoom_level);
  shrink_button.SetEnabled(zoom > 0);
}

void
WaypointImagePageWidget::AdjustViewForZoomChange(const int old_zoom,
                                                  const int new_zoom) noexcept
{
  if (!image_window.IsDefined())
    return;

  const PixelRect rc = image_window.GetClientRect();
  ImageZoomView::AdjustImageViewOnZoomChange(old_zoom, new_zoom,
                                             image_window.GetViewPosition(),
                                             rc.GetSize(), bitmap.GetSize());
  image_window.ClearPendingOffset();
}

void
WaypointImagePageWidget::ResetZoom() noexcept
{
  if (zoom == 0)
    return;

  const int old_zoom = zoom;
  zoom = 0;
  AdjustViewForZoomChange(old_zoom, zoom);
  UpdateZoomControls();
  image_window.Invalidate();
}

void
WaypointImagePageWidget::Magnify() noexcept
{
  if (zoom >= ImageZoomView::max_zoom_level)
    return;

  const int old_zoom = zoom;
  ++zoom;
  AdjustViewForZoomChange(old_zoom, zoom);
  image_window.Invalidate();
  UpdateZoomControls();
}

void
WaypointImagePageWidget::Shrink() noexcept
{
  if (zoom <= 0)
    return;

  const int old_zoom = zoom;
  --zoom;
  AdjustViewForZoomChange(old_zoom, zoom);
  image_window.Invalidate();
  UpdateZoomControls();
}

bool
WaypointImagePageWidget::TryWaypointImageKey(unsigned key_code) noexcept
{
  return wptimg_mode.has_value() &&
    InputEvents::ProcessKeyInMode(*wptimg_mode, key_code);
}

void
WaypointImagePageWidget::OnWaypointImageEvent(const char *misc) noexcept
{
  if (StringIsEqual(misc, "magnify")) {
    Magnify();
    return;
  }
  if (StringIsEqual(misc, "shrink")) {
    Shrink();
    return;
  }
  if (StringIsEqual(misc, "reset")) {
    ResetZoom();
    return;
  }

  if (StringIsEqual(misc, "left")) {
    if (zoom == 0)
      change_page(-1);
    else
      image_window.NudgeViewByPixelOffset({-pan_step, 0});
    return;
  }
  if (StringIsEqual(misc, "right")) {
    if (zoom == 0)
      change_page(+1);
    else
      image_window.NudgeViewByPixelOffset({pan_step, 0});
    return;
  }
  if (zoom == 0)
    return;

  if (StringIsEqual(misc, "up"))
    image_window.NudgeViewByPixelOffset({0, -pan_step});
  else if (StringIsEqual(misc, "down"))
    image_window.NudgeViewByPixelOffset({0, pan_step});
}

void
WaypointImagePageWidget::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  const Layout layout(rc);

  WindowStyle image_style;
  image_style.Hide();
  image_style.ControlParent();

  WindowStyle button_style;
  button_style.Hide();
  button_style.TabStop();

  magnify_button.Create(parent, layout.magnify_button, button_style,
                        std::make_unique<SymbolButtonRenderer>(look.button, "+"),
                        [this](){ Magnify(); });
  shrink_button.Create(parent, layout.shrink_button, button_style,
                       std::make_unique<SymbolButtonRenderer>(look.button, "-"),
                       [this](){ Shrink(); });

  image_window.Create(parent, layout.image, image_style);
  image_window.SetContent(&bitmap, &zoom);

  const int mode_id = InputEvents::GetModeId("wptimg");
  wptimg_mode = mode_id >= 0
    ? std::make_optional(static_cast<InputEvents::Mode>(mode_id))
    : std::nullopt;
  image_window.SetTryKeyInput(
    [this](unsigned key_code) { return KeyPress(key_code); });
  UpdateZoomControls();
}

void
WaypointImagePageWidget::Unprepare() noexcept
{
  image_window.SetTryKeyInput(nullptr);
  wptimg_mode.reset();
}

void
WaypointImagePageWidget::Show(const PixelRect &rc) noexcept
{
  const Layout layout(rc);
  image_window.MoveAndShow(layout.image);
  magnify_button.MoveAndShow(layout.magnify_button);
  shrink_button.MoveAndShow(layout.shrink_button);
}

void
WaypointImagePageWidget::Hide() noexcept
{
  image_window.Hide();
  magnify_button.Hide();
  shrink_button.Hide();
}

void
WaypointImagePageWidget::Move(const PixelRect &rc) noexcept
{
  const Layout layout(rc);
  image_window.Move(layout.image);
  magnify_button.Move(layout.magnify_button);
  shrink_button.Move(layout.shrink_button);
}

bool
WaypointImagePageWidget::SetFocus() noexcept
{
  image_window.SetFocus();
  return true;
}

bool
WaypointImagePageWidget::HasFocus() const noexcept
{
  return image_window.HasFocus() || magnify_button.HasFocus() ||
    shrink_button.HasFocus();
}

bool
WaypointImagePageWidget::KeyPress(unsigned key_code) noexcept
{
  if (TryWaypointImageKey(key_code))
    return true;

  switch (key_code) {
  case KEY_F1:
    ResetZoom();
    return true;

  case KEY_F2:
    Magnify();
    return true;

  case KEY_F3:
    Shrink();
    return true;

  case KEY_LEFT:
    if (zoom == 0)
      change_page(-1);
    else
      image_window.NudgeViewByPixelOffset({-pan_step, 0});
    return true;

  case KEY_RIGHT:
    if (zoom == 0)
      change_page(+1);
    else
      image_window.NudgeViewByPixelOffset({pan_step, 0});
    return true;

  case KEY_UP:
    if (zoom == 0)
      return false;
    image_window.NudgeViewByPixelOffset({0, -pan_step});
    return true;

  case KEY_DOWN:
    if (zoom == 0)
      return false;
    image_window.NudgeViewByPixelOffset({0, pan_step});
    return true;

  case KEY_ESCAPE:
    if (zoom == 0)
      return false;
    ResetZoom();
    return true;

  default:
    return false;
  }
}

class WaypointDetailsWidget final : public TabWidget {
  WidgetDialog &dialog;
  const DialogLook &look{dialog.GetLook()};
  Waypoints *const waypoints;
  const WaypointPtr waypoint;
  ProtectedTaskManager *const task_manager;
  const bool allow_edit;
  const WaypointDetailsNesting nesting;
  const bool sim_jump_active;

  StaticString<256> base_caption;
  AllocatedPath source_path{nullptr};
  std::optional<unsigned> first_image_page;
  bool caption_initialised = false;

  bool HasDetails() const noexcept;
  void AddImageTabs() noexcept;
  void ChangePage(int step) noexcept;
  WaypointImagePageWidget *GetCurrentImagePage() noexcept;
  void OnWaypointImageEvent(const char *misc) noexcept;

  friend void WaypointDetailsDispatchImageInput(const char *misc) noexcept;

public:
  WaypointDetailsWidget(WidgetDialog &_dialog,
                        Waypoints *_waypoints, WaypointPtr _waypoint,
                        ProtectedTaskManager *_task_manager, bool _allow_edit,
                        const WaypointDetailsNesting &_nesting) noexcept
    :TabWidget(Orientation::AUTO), dialog(_dialog), waypoints(_waypoints),
     waypoint(std::move(_waypoint)), task_manager(_task_manager),
     allow_edit(_allow_edit), nesting(_nesting),
     sim_jump_active(is_simulator()) {}

  [[gnu::pure]]
  AllocatedPath GetSourcePath() const noexcept;

  void InitCaption() noexcept;

  bool IsNavigationAllowed() const noexcept {
    return task_manager != nullptr;
  }

  bool IsSimJumpActive() const noexcept {
    return sim_jump_active;
  }

  void OnGotoClicked();
  void OnPanClicked();
  void OnSimJumpClicked();
  void OnCloseClicked() noexcept;

  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
};

static WaypointDetailsWidget *waypoint_image_input_target = nullptr;

void
WaypointDetailsDispatchImageInput(const char *misc) noexcept
{
  if (waypoint_image_input_target != nullptr && misc != nullptr)
    waypoint_image_input_target->OnWaypointImageEvent(misc);
}

bool
WaypointDetailsWidget::HasDetails() const noexcept
{
#ifdef HAVE_RUN_FILE
  return !waypoint->details.empty() || !waypoint->files_external.empty();
#else
  return !waypoint->details.empty();
#endif
}

void
WaypointDetailsWidget::AddImageTabs() noexcept
{
  const bool is_cupx = source_path != nullptr &&
    source_path.EndsWithIgnoreCase(".cupx");
  unsigned image_number = 0;

  for (const auto &i : waypoint->files_embed) {
    if (image_number == 5)
      break;

    try {
      Bitmap bitmap;
      bool loaded = false;

      if (is_cupx) {
        auto data = CupxArchive::ExtractImage(source_path, i);
        if (data.empty())
          continue;

#if !defined(USE_GDI) && !defined(ANDROID)
        loaded = bitmap.Load(std::span<const std::byte>(data));
#else
        const auto tmp_dir = MakeCacheDirectory("cupx");
        const auto tmp_file = AllocatedPath::Build(tmp_dir, i.c_str());

        FileOutputStream fos(tmp_file, FileOutputStream::Mode::CREATE_VISIBLE);
        fos.Write(std::as_bytes(std::span{data}));
        fos.Commit();

        loaded = bitmap.LoadFile(tmp_file);
        File::Delete(tmp_file);
#endif
      } else {
        loaded = bitmap.LoadFile(LocalPath(i.c_str()));
      }

      if (!loaded)
        continue;

      if (!first_image_page.has_value())
        first_image_page = GetSize();

      StaticString<32> caption;
      caption.Format(_("Image %u"), ++image_number);
      AddTab(std::make_unique<WaypointImagePageWidget>(
               look, std::move(bitmap),
               [this](int step) { ChangePage(step); }),
             caption);
    } catch (const std::exception &e) {
      LogFormat("Failed to load %s: %s",
                (const char *)NarrowPathName(Path(i.c_str())), e.what());
    }
  }
}

void
WaypointDetailsWidget::ChangePage(int step) noexcept
{
  if (step < 0)
    Previous(true);
  else if (step > 0)
    Next(true);
}

WaypointImagePageWidget *
WaypointDetailsWidget::GetCurrentImagePage() noexcept
{
  if (!first_image_page.has_value() ||
      GetCurrentIndex() < *first_image_page)
    return nullptr;

  return static_cast<WaypointImagePageWidget *>(&GetCurrentWidget());
}

void
WaypointDetailsWidget::OnWaypointImageEvent(const char *misc) noexcept
{
  if (auto *page = GetCurrentImagePage())
    page->OnWaypointImageEvent(misc);
}

void
WaypointDetailsWidget::Initialise(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  if (!caption_initialised)
    InitCaption();

  TabWidget::Initialise(parent, rc);
  AddTab(std::make_unique<WaypointInfoWidget>(look, waypoint), _("Info"));

  if (HasDetails())
    AddTab(std::make_unique<WaypointDetailsPageWidget>(look, waypoint),
           _("Details"));

  AddTab(std::make_unique<WaypointCommandsWidget>(
           look, &dialog, waypoints, waypoint, task_manager, allow_edit, nesting),
         _("Commands"));
  AddImageTabs();
}

void
WaypointDetailsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  TabWidget::Prepare(parent, rc);

  if (first_image_page.has_value())
    waypoint_image_input_target = this;
}

void
WaypointDetailsWidget::Unprepare() noexcept
{
  if (waypoint_image_input_target == this)
    waypoint_image_input_target = nullptr;

  TabWidget::Unprepare();
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
  if (nesting.state_change_committed != nullptr)
    *nesting.state_change_committed = true;
  dialog.SetModalResult(mrOK);

  CommonInterface::main_window->FullRedraw();
}

void
WaypointDetailsWidget::OnPanClicked()
{
  if (!ActivatePan(*waypoint))
    return;

  if (nesting.map_pan_from_details != nullptr)
    *nesting.map_pan_from_details = true;
  if (nesting.include_pan_in_parent_dismissal &&
      nesting.state_change_committed != nullptr)
    *nesting.state_change_committed = true;
  dialog.SetModalResult(mrOK);
}

void
WaypointDetailsWidget::OnSimJumpClicked()
{
  if (!sim_jump_active)
    return;

  if (SimJumpTo(waypoint->location)) {
    if (nesting.state_change_committed != nullptr)
      *nesting.state_change_committed = true;
    dialog.SetModalResult(mrOK);
  }
}

void
WaypointDetailsWidget::OnCloseClicked() noexcept
{
  if (nesting.state_change_committed != nullptr)
    *nesting.state_change_committed = false;
  dialog.SetModalResult(mrOK);
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
  dialog.SetCaption(base_caption);
  caption_initialised = true;
}

void
dlgWaypointDetailsShowModal(Waypoints *waypoints, WaypointPtr _waypoint,
                            bool allow_navigation, bool allow_edit,
                            const WaypointDetailsNesting *nesting) noexcept
{
  if (_waypoint == nullptr)
    return;

  LastUsedWaypoints::Add(*_waypoint);

  const WaypointDetailsNesting k_default;
  const WaypointDetailsNesting &N = nesting != nullptr ? *nesting : k_default;

  if (N.state_change_committed != nullptr)
    *N.state_change_committed = false;
  if (N.map_pan_from_details != nullptr)
    *N.map_pan_from_details = false;

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<WaypointDetailsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, nullptr);
  dialog.SetWidget(
    dialog, waypoints, _waypoint,
    allow_navigation ? backend_components->protected_task_manager.get() : nullptr,
    allow_edit, N);

  auto &widget = dialog.GetWidget();
  widget.InitCaption();

  if (widget.IsNavigationAllowed())
    dialog.AddButton(_("GoTo"), [&widget](){ widget.OnGotoClicked(); });
  else
    dialog.AddButton(_("Pan To"), [&widget](){ widget.OnPanClicked(); });

  if (widget.IsSimJumpActive())
    dialog.AddButton(C_("Button", "Sim: Jump to"),
                     [&widget](){ widget.OnSimJumpClicked(); });

  dialog.AddButton(_("Close"), [&widget](){ widget.OnCloseClicked(); });

  dialog.ShowModal();
}

bool
dlgWaypointDetailsShowModalForBrowseParent(
  Waypoints *waypoints, WaypointPtr &&waypoint, bool allow_navigation,
  bool allow_edit) noexcept
{
  bool state_change = false;
  const WaypointDetailsNesting nesting{
    .state_change_committed = &state_change,
  };
  dlgWaypointDetailsShowModal(waypoints, std::move(waypoint), allow_navigation,
                                allow_edit, &nesting);
  return state_change;
}
