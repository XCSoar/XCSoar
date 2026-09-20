// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/TextListWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Widget/ImageZoomView.hpp"
#include "Widget/ImageZoomFrame.hpp"
#include "Widget/Widget.hpp"
#include "Weather/PCMet/Images.hpp"
#include "Weather/PCMet/Georeference.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Look/MapLook.hpp"
#include "MapSettings.hpp"
#include "Asset.hpp"
#include "Math/Angle.hpp"
#include "co/InvokeTask.hxx"
#include "co/Task.hxx"
#include "net/http/Init.hpp"
#include "system/Path.hpp"
#include "Interface.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <chrono>
#include <cmath>

class PCMetImageWidget final : public NullWidget {
  const Bitmap &bitmap;

  /**
   * The geographic extent of #bitmap; nullptr if it is not known, in
   * which case the aircraft symbol is not drawn.
   */
  const PCMet::ImageGeoreference *const georeference;

  ImageZoomFrame image_window;
  double zoom_factor = ImageZoomView::FIT_ZOOM_FACTOR;

  Button *magnify_button = nullptr;
  Button *shrink_button = nullptr;

  UI::PeriodicTimer update_timer{[this]{ OnAircraftTimer(); }};

  /** Bitmap pixel and heading last painted.  A still aircraft does
      not redraw the image. */
  bool aircraft_drawn = false;
  int aircraft_x = 0;
  int aircraft_y = 0;
  Angle aircraft_heading = Angle::Zero();

  void OnAircraftTimer() noexcept
  {
    if (georeference == nullptr)
      return;

    const auto &basic = CommonInterface::Basic();
    if (!basic.location_available) {
      if (aircraft_drawn)
        image_window.Invalidate();
      return;
    }

    const auto pixel = georeference->ToPixel(basic.location);
    const int x = int(std::lround(pixel.x));
    const int y = int(std::lround(pixel.y));
    if (aircraft_drawn &&
        x == aircraft_x && y == aircraft_y &&
        basic.attitude.heading.CompareRoughly(aircraft_heading,
                                               Angle::Degrees(5)))
      return;

    image_window.Invalidate();
  }

  void UpdateZoomControls() noexcept
  {
    if (magnify_button != nullptr)
      magnify_button->SetEnabled(zoom_factor < ImageZoomView::MAX_ZOOM_FACTOR);
    if (shrink_button != nullptr)
      shrink_button->SetEnabled(!ImageZoomView::IsFitZoomFactor(zoom_factor));
  }

  void AdjustView(const double old_zoom, const double new_zoom) noexcept
  {
    if (!image_window.IsDefined())
      return;

    const PixelRect rc = image_window.GetClientRect();
    ImageZoomView::AdjustImageViewOnZoomChange(old_zoom, new_zoom,
                                               image_window.GetViewPosition(),
                                               rc.GetSize(), bitmap.GetSize());
    image_window.ClearPendingOffset();
  }

  /**
   * Draw the aircraft symbol at the current GPS position, on top of
   * the image.
   */
  void DrawAircraft(Canvas &canvas,
                    const ImageZoomView::Layout &layout) noexcept
  {
    if (georeference == nullptr)
      return;

    const auto &basic = CommonInterface::Basic();
    if (!basic.location_available) {
      aircraft_drawn = false;
      return;
    }

    const auto pixel = georeference->ToPixel(basic.location);
    aircraft_x = int(std::lround(pixel.x));
    aircraft_y = int(std::lround(pixel.y));
    aircraft_heading = basic.attitude.heading;
    aircraft_drawn = true;

    if (!georeference->IsInside(pixel))
      /* outside the map section this image shows */
      return;

    /* the georeference refers to the nominal image size; scale in case
       the DWD ever delivers a different one */
    const PixelSize size = bitmap.GetSize();
    const PixelSize nominal = georeference->nominal_size;
    const auto position = layout.BitmapToScreen({
      pixel.x * size.width / nominal.width,
      pixel.y * size.height / nominal.height,
    });

    if (!layout.screen_rect.Contains(position))
      /* scrolled out of view */
      return;

    AircraftRenderer::Draw(canvas, CommonInterface::GetMapSettings(),
                           UIGlobals::GetMapLook().aircraft,
                           basic.attitude.heading
                           - georeference->GetUpBearing(basic.location),
                           position);
  }

public:
  PCMetImageWidget(const Bitmap &_bitmap,
                   const PCMet::ImageGeoreference *_georeference) noexcept
    :bitmap(_bitmap), georeference(_georeference) {}

  void SetZoomButtons(Button *magnify, Button *shrink) noexcept
  {
    magnify_button = magnify;
    shrink_button = shrink;
    UpdateZoomControls();
  }

  void SetZoomFactor(const double new_zoom_factor) noexcept
  {
    const double old_zoom_factor = zoom_factor;
    zoom_factor = ImageZoomView::ClampZoomFactor(new_zoom_factor);
    if (zoom_factor == old_zoom_factor)
      return;

    AdjustView(old_zoom_factor, zoom_factor);
    image_window.Invalidate();
    UpdateZoomControls();
  }

  void Magnify() noexcept
  {
    SetZoomFactor(zoom_factor * ImageZoomView::ZOOM_STEP_FACTOR);
  }

  void Shrink() noexcept
  {
    SetZoomFactor(zoom_factor / ImageZoomView::ZOOM_STEP_FACTOR);
  }

  bool
  TryImageKey(unsigned key_code) noexcept
  {
    const int step = Layout::Scale(ImageZoomView::PAN_STEP);

    switch (key_code) {
    case KEY_F2:
      Magnify();
      return true;

    case KEY_F3:
      Shrink();
      return true;

    case KEY_LEFT:
      if (ImageZoomView::IsFitZoomFactor(zoom_factor))
        return false;
      image_window.NudgeViewByPixelOffset({-step, 0});
      return true;

    case KEY_RIGHT:
      if (ImageZoomView::IsFitZoomFactor(zoom_factor))
        return false;
      image_window.NudgeViewByPixelOffset({step, 0});
      return true;

    case KEY_UP:
      if (ImageZoomView::IsFitZoomFactor(zoom_factor))
        return false;
      image_window.NudgeViewByPixelOffset({0, -step});
      return true;

    case KEY_DOWN:
      if (ImageZoomView::IsFitZoomFactor(zoom_factor))
        return false;
      image_window.NudgeViewByPixelOffset({0, step});
      return true;

    default:
      return false;
    }
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    /* no ControlParent() here: the image window is a PaintWindow
       without children, and WindowList::FindControl() casts a
       "control parent" to ContainerWindow while looking for the next
       control */
    WindowStyle image_style;
    image_style.Hide();

    image_window.Create(parent, rc, image_style);
    image_window.SetContent(&bitmap, &zoom_factor);
    image_window.SetTryKeyInput(
      [this](unsigned key_code) { return TryImageKey(key_code); });
    image_window.SetOnZoomChanged([this]() { UpdateZoomControls(); });

    if (georeference != nullptr)
      image_window.SetOverlayRenderer(
        [this](Canvas &canvas,
               const ImageZoomView::Layout &layout) noexcept {
          /* ImageZoomFrame::OnPaint() is noexcept and calls this
             through a std::function, so the contract has to be
             visible here; DrawAircraft() is noexcept too */
          DrawAircraft(canvas, layout);
        });

    UpdateZoomControls();
  }

  void Unprepare() noexcept override
  {
    image_window.SetTryKeyInput(nullptr);
    image_window.SetOnZoomChanged(nullptr);
    image_window.SetOverlayRenderer(nullptr);
  }

  void Show(const PixelRect &rc) noexcept override
  {
    image_window.MoveAndShow(rc);
    image_window.SetFocus();

    if (georeference != nullptr) {
      /* Kobo flips the whole panel on every redraw.  These images
         are about a kilometre per pixel, so half a minute still
         tracks a glider without flashing the page every second. */
      update_timer.Schedule(HasEPaper()
                            ? std::chrono::seconds{30}
                            : std::chrono::seconds{1});
    }
  }

  void Hide() noexcept override
  {
    update_timer.Cancel();
    image_window.Hide();
  }

  bool SetFocus() noexcept override
  {
    if (!image_window.IsDefined())
      return false;

    image_window.SetFocus();
    return true;
  }

  bool KeyPress(unsigned key_code) noexcept override
  {
    return TryImageKey(key_code);
  }
};

static void
BitmapDialog(const Bitmap &bitmap,
             const PCMet::ImageGeoreference *georeference)
{
  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      "Flugwetter",
                      new PCMetImageWidget(bitmap, georeference));
  auto &image = static_cast<PCMetImageWidget &>(dialog.GetWidget());

  dialog.AddButton(_("Close"), mrOK);
  image.SetZoomButtons(
    dialog.AddSymbolButton("+", [&image]() { image.Magnify(); }),
    dialog.AddSymbolButton("-", [&image]() { image.Shrink(); }));
  dialog.ShowModal();
}

static void
BitmapDialog(const PCMet::ImageType &type, const PCMet::ImageArea &area)
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;

  try {
    PluggableOperationEnvironment env;

    auto path = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                     UIGlobals::GetDialogLook(),
                                     _("Download"),
                                     PCMet::DownloadLatestImage(type.uri, area.name,
                                                                settings,
                                                                *Net::curl, env),
                                     &env);
    if (!path)
      return;

    Bitmap bitmap;
    bitmap.LoadFile(*path);
    BitmapDialog(bitmap, PCMet::FindImageGeoreference(type.uri, area.name));
  } catch (...) {
    ShowError(std::current_exception(), "Flugwetter");
  }
}

class ImageAreaListWidget final : public TextListWidget {
  const PCMet::ImageType *type = nullptr;
  const PCMet::ImageArea *areas = nullptr;

public:
  void SetType(const PCMet::ImageType *_type) {
    if (_type == type)
      return;

    type = _type;
    areas = type != nullptr ? type->areas : nullptr;

    unsigned n = 0;
    if (areas != nullptr)
      while (areas[n].name != nullptr)
        ++n;

    auto &list_control = GetList();
    list_control.SetLength(n);
    list_control.Invalidate();
  }

protected:
  /* virtual methods from TextListWidget */
  const char *GetRowText(unsigned i) const noexcept override {
    return areas[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    BitmapDialog(*type, areas[index]);
  }
};

class ImageTypeListWidget final : public TextListWidget {
  ImageAreaListWidget &area_list;

public:
  explicit ImageTypeListWidget(ImageAreaListWidget &_area_list)
    :area_list(_area_list) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
      TextListWidget::Prepare(parent, rc);

      unsigned n = 0;
      while (PCMet::image_types[n].uri != nullptr)
        ++n;
      GetList().SetLength(n);
  }

  void Show(const PixelRect &rc) noexcept override {
    TextListWidget::Show(rc);
    area_list.SetType(&PCMet::image_types[GetList().GetCursorIndex()]);
  }

protected:
  /* virtual methods from TextListWidget */
  const char *GetRowText(unsigned i) const noexcept override {
    return PCMet::image_types[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  void OnCursorMoved(unsigned index) noexcept override {
    area_list.SetType(&PCMet::image_types[index]);
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    area_list.SetFocus();
  }
};

std::unique_ptr<Widget>
CreatePCMetMainWidget()
{
  auto area_widget = std::make_unique<ImageAreaListWidget>();
  auto type_widget = std::make_unique<ImageTypeListWidget>(*area_widget);

  return std::make_unique<TwoWidgets>(std::move(type_widget),
                                      std::move(area_widget),
                                      false);
}

#endif  // HAVE_PCMET
