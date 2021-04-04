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

#include "PCMetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "UIGlobals.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/Error.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/TextListWidget.hpp"
#include "Widget/ViewImageWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/PCMet/Images.hpp"
#include "net/http/Init.hpp"
#include "Interface.hpp"

static void
BitmapDialog(const Bitmap &bitmap)
{
  TWidgetDialog<ViewImageWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _T("pc_met"), new ViewImageWidget(bitmap));
  dialog.AddButton(_("Close"), mrOK);
//  dialog.SetWidget();
  dialog.ShowModal();
}

static void
BitmapDialog(const PCMet::ImageType &type, const PCMet::ImageArea &area)
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;

  DialogJobRunner runner(UIGlobals::GetMainWindow(),
                         UIGlobals::GetDialogLook(),
                         _("Download"), true);

  try {
    Bitmap bitmap = PCMet::DownloadLatestImage(type.uri, area.name,
                                               settings, *Net::curl, runner);
    if (!bitmap.IsDefined()) {
      ShowMessageBox(_("Failed to download file."),
                     _T("pc_met"), MB_OK);
      return;
    }

    BitmapDialog(bitmap);
  } catch (...) {
    ShowError(std::current_exception(), _T("pc_met"));
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
  const TCHAR *GetRowText(unsigned i) const noexcept override {
    return areas[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem(unsigned index) const noexcept override {
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
  const TCHAR *GetRowText(unsigned i) const noexcept override {
    return PCMet::image_types[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  void OnCursorMoved(unsigned index) noexcept override {
    area_list.SetType(&PCMet::image_types[index]);
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    area_list.SetFocus();
  }
};

std::unique_ptr<Widget>
CreatePCMetWidget()
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;
  if (!settings.www_credentials.IsDefined())
    return std::make_unique<LargeTextWidget>(UIGlobals::GetDialogLook(),
                                             _T("No account was configured."));

  auto area_widget = std::make_unique<ImageAreaListWidget>();
  auto type_widget = std::make_unique<ImageTypeListWidget>(*area_widget);

  return std::make_unique<TwoWidgets>(std::move(type_widget),
                                      std::move(area_widget),
                                      false);
}

#endif  // HAVE_PCMET
