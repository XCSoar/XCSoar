// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
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
#include "Operation/PluggableOperationEnvironment.hpp"
#include "co/InvokeTask.hxx"
#include "co/Task.hxx"
#include "net/http/Init.hpp"
#include "system/Path.hpp"
#include "Interface.hpp"
#include "ui/event/KeyCode.hpp"

class PCMetImageWidget final : public NullWidget {
  const Bitmap &bitmap;
  ImageZoomFrame image_window;
  int zoom = 0;

  Button *magnify_button = nullptr;
  Button *shrink_button = nullptr;

  void UpdateZoomControls() noexcept
  {
    if (magnify_button != nullptr)
      magnify_button->SetEnabled(zoom < ImageZoomView::max_zoom_level);
    if (shrink_button != nullptr)
      shrink_button->SetEnabled(zoom > 0);
  }

  void AdjustView(const int old_zoom, const int new_zoom) noexcept
  {
    if (!image_window.IsDefined())
      return;

    const PixelRect rc = image_window.GetClientRect();
    ImageZoomView::AdjustImageViewOnZoomChange(old_zoom, new_zoom,
                                               image_window.GetViewPosition(),
                                               rc.GetSize(), bitmap.GetSize());
    image_window.ClearPendingOffset();
  }

public:
  explicit PCMetImageWidget(const Bitmap &_bitmap) noexcept
    :bitmap(_bitmap) {}

  void SetZoomButtons(Button *magnify, Button *shrink) noexcept
  {
    magnify_button = magnify;
    shrink_button = shrink;
    UpdateZoomControls();
  }

  void Magnify() noexcept
  {
    if (zoom >= ImageZoomView::max_zoom_level)
      return;

    const int old_zoom = zoom;
    ++zoom;
    AdjustView(old_zoom, zoom);
    image_window.Invalidate();
    UpdateZoomControls();
  }

  void Shrink() noexcept
  {
    if (zoom <= 0)
      return;

    const int old_zoom = zoom;
    --zoom;
    AdjustView(old_zoom, zoom);
    image_window.Invalidate();
    UpdateZoomControls();
  }

  bool
  TryImageKey(unsigned key_code) noexcept
  {
    switch (key_code) {
    case KEY_F2:
      Magnify();
      return true;

    case KEY_F3:
      Shrink();
      return true;

    case KEY_LEFT:
      if (zoom == 0)
        return false;
      image_window.NudgeViewByPixelOffset({-50, 0});
      return true;

    case KEY_RIGHT:
      if (zoom == 0)
        return false;
      image_window.NudgeViewByPixelOffset({50, 0});
      return true;

    case KEY_UP:
      if (zoom == 0)
        return false;
      image_window.NudgeViewByPixelOffset({0, -50});
      return true;

    case KEY_DOWN:
      if (zoom == 0)
        return false;
      image_window.NudgeViewByPixelOffset({0, 50});
      return true;

    default:
      return false;
    }
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    WindowStyle image_style;
    image_style.Hide();
    image_style.ControlParent();

    image_window.Create(parent, rc, image_style);
    image_window.SetContent(&bitmap, &zoom);
    image_window.SetTryKeyInput(
      [this](unsigned key_code) { return TryImageKey(key_code); });
    UpdateZoomControls();
  }

  void Unprepare() noexcept override
  {
    image_window.SetTryKeyInput(nullptr);
  }

  void Show(const PixelRect &rc) noexcept override
  {
    image_window.MoveAndShow(rc);
    image_window.SetFocus();
  }

  void Hide() noexcept override
  {
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
BitmapDialog(const Bitmap &bitmap)
{
  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      "pc_met", new PCMetImageWidget(bitmap));
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
    BitmapDialog(bitmap);
  } catch (...) {
    ShowError(std::current_exception(), "pc_met");
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
CreatePCMetWidget()
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;
  if (!settings.www_credentials.IsDefined())
    return std::make_unique<LargeTextWidget>(UIGlobals::GetDialogLook(),
                                             "No account was configured.");

  auto area_widget = std::make_unique<ImageAreaListWidget>();
  auto type_widget = std::make_unique<ImageTypeListWidget>(*area_widget);

  return std::make_unique<TwoWidgets>(std::move(type_widget),
                                      std::move(area_widget),
                                      false);
}

#endif  // HAVE_PCMET
