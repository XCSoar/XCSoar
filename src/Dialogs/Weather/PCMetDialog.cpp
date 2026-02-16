// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/TextListWidget.hpp"
#include "Widget/ViewImageWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/PCMet/Images.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "co/InvokeTask.hxx"
#include "co/Task.hxx"
#include "net/http/Init.hpp"
#include "system/Path.hpp"
#include "Interface.hpp"

static void
BitmapDialog(const Bitmap &bitmap)
{
  TWidgetDialog<ViewImageWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           "pc_met", new ViewImageWidget(bitmap));
  dialog.AddButton(_("Close"), mrOK);
//  dialog.SetWidget();
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
