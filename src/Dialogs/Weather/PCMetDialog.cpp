/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/TextListWidget.hpp"
#include "Widget/ViewImageWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/PCMet/Images.hpp"
#include "Interface.hpp"

static void
BitmapDialog(const Bitmap &bitmap)
{
  ViewImageWidget widget(bitmap);
  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateFull(UIGlobals::GetMainWindow(), _T("pc_met"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
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
                                               settings, runner);
    if (!bitmap.IsDefined()) {
      ShowMessageBox(_("Failed to download file."),
                     _T("pc_met"), MB_OK);
      return;
    }

    BitmapDialog(bitmap);
  } catch (const std::exception &exception) {
    ShowError(exception, _T("pc_met"));
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
  const TCHAR *GetRowText(unsigned i) const override {
    return areas[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override {
    BitmapDialog(*type, areas[index]);
  }
};

class ImageTypeListWidget final : public TextListWidget {
  ImageAreaListWidget &area_list;

public:
  explicit ImageTypeListWidget(ImageAreaListWidget &_area_list)
    :area_list(_area_list) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override {
      TextListWidget::Prepare(parent, rc);

      unsigned n = 0;
      while (PCMet::image_types[n].uri != nullptr)
        ++n;
      GetList().SetLength(n);
  }

  void Show(const PixelRect &rc) override {
    TextListWidget::Show(rc);
    area_list.SetType(&PCMet::image_types[GetList().GetCursorIndex()]);
  }

protected:
  /* virtual methods from TextListWidget */
  const TCHAR *GetRowText(unsigned i) const override {
    return PCMet::image_types[i].display_name;
  }

  /* virtual methods from ListCursorHandler */
  void OnCursorMoved(unsigned index) override {
    area_list.SetType(&PCMet::image_types[index]);
  }

  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override {
    area_list.SetFocus();
  }
};

Widget *
CreatePCMetWidget()
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;
  if (!settings.www_credentials.IsDefined())
    return new LargeTextWidget(UIGlobals::GetDialogLook(),
                               _T("No account was configured."));

  auto *area_widget = new ImageAreaListWidget();
  auto *type_widget = new ImageTypeListWidget(*area_widget);

  return new TwoWidgets(type_widget, area_widget, false);
}

#endif
