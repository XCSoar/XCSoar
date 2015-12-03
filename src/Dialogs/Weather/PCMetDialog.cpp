/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "WeatherDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "UIGlobals.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Form/Draw.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Widget/WindowWidget.hpp"
#include "Weather/PCMet.hpp"
#include "Interface.hpp"

class ViewImageWidget : public WindowWidget {
  const Bitmap &bitmap;
  WndOwnerDrawFrame view;

public:
  explicit ViewImageWidget(const Bitmap &_bitmap)
    :bitmap(_bitmap) {}

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override {
    WindowStyle hidden;
    hidden.Hide();

    view.Create(parent, rc, hidden,
                [this](Canvas &canvas, const PixelRect &rc){
                  OnImagePaint(canvas, rc);
                });
    SetWindow(&view);
  }

  void Unprepare() override {
    view.Destroy();
  }

private:
  void OnImagePaint(Canvas &canvas, const PixelRect &rc) {
    canvas.ClearWhite();

    const PixelSize bitmap_size = bitmap.GetSize();
    const PixelSize window_size(rc.right - rc.left, rc.bottom - rc.top);

    PixelSize fit_size(window_size.cx,
                       window_size.cx * bitmap_size.cy / bitmap_size.cx);
    if (fit_size.cy > window_size.cy) {
      fit_size.cy = window_size.cy;
      fit_size.cx = window_size.cy * bitmap_size.cx / bitmap_size.cy;
    }

    canvas.Stretch((rc.left + rc.right - fit_size.cx) / 2,
                   (rc.top + rc.bottom - fit_size.cy) / 2,
                   fit_size.cx, fit_size.cy,
                   bitmap);
  }
};

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

static const PCMet::ImageType *
PickImageType()
{
  ComboList combo_list;
  for (const auto *i = PCMet::image_types; i->uri != nullptr; ++i)
    combo_list.Append(i->display_name);

  int i = ComboPicker(_T("Pick an image type"), combo_list);
  return i >= 0
    ? &PCMet::image_types[i]
    : nullptr;
}

static const PCMet::ImageArea *
PickImageArea(const PCMet::ImageType &type)
{
  ComboList combo_list;
  for (const auto *i = type.areas; i->name != nullptr; ++i)
    combo_list.Append(i->display_name);

  int i = ComboPicker(_T("Pick an area"), combo_list);
  return i >= 0
    ? &type.areas[i]
    : nullptr;
}

void
ShowPCMetDialog()
{
  const auto &settings = CommonInterface::GetComputerSettings().weather.pcmet;
  if (settings.username.empty() || settings.password.empty()) {
    ShowMessageBox(_("No account was configured."),
                   _T("pc_met"), MB_OK);
    return;
  }

  const auto *image_type = PickImageType();
  if (image_type == nullptr)
    return;

  const auto *image_area = PickImageArea(*image_type);
  if (image_area == nullptr)
    return;

  DialogJobRunner runner(UIGlobals::GetMainWindow(),
                         UIGlobals::GetDialogLook(),
                         _("Download"), true);

  Bitmap bitmap = PCMet::DownloadLatestImage(image_type->uri, image_area->name,
                                             settings, runner);
  if (!bitmap.IsDefined())
    return;

  BitmapDialog(bitmap);
}

#else
void
ShowPCMetDialog()
{
  ShowMessageBox(_("This function is not available on your platform yet."),
                 _T("pc_met"), MB_OK);
}
#endif
