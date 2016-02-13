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

#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "UIState.hpp"

namespace InfoBoxManager
{
  InfoBoxLayout::Layout layout;

  /**
   * Is this the initial DisplayInfoBox() call?  If yes, then all
   * content objects need to be created.
   */
  static bool first;

  static void DisplayInfoBox();
  static void InfoBoxDrawIfDirty();
}

static bool infoboxes_dirty = false;
static bool infoboxes_hidden = false;

static InfoBoxWindow *infoboxes[InfoBoxSettings::Panel::MAX_CONTENTS];

// TODO locking
void
InfoBoxManager::Hide()
{
  if (infoboxes_hidden)
    return;

  infoboxes_hidden = true;

  for (unsigned i = 0; i < layout.count; i++)
    infoboxes[i]->FastHide();
}

void
InfoBoxManager::Show()
{
  if (!infoboxes_hidden)
    return;

  infoboxes_hidden = false;

  for (unsigned i = 0; i < layout.count; i++)
    infoboxes[i]->Show();

  SetDirty();
}

void
InfoBoxManager::DisplayInfoBox()
{
  static int DisplayTypeLast[InfoBoxSettings::Panel::MAX_CONTENTS];

  // JMW note: this is updated every GPS time step

  const unsigned panel = CommonInterface::GetUIState().panel_index;

  const InfoBoxSettings::Panel &settings =
    CommonInterface::GetUISettings().info_boxes.panels[panel];

  for (unsigned i = 0; i < layout.count; i++) {
    // All calculations are made in a separate thread. Slow calculations
    // should apply to the function DoCalculationsSlow()
    // Do not put calculations here!

    InfoBoxFactory::Type DisplayType = settings.contents[i];
    if ((unsigned)DisplayType > (unsigned)InfoBoxFactory::MAX_TYPE_VAL)
      DisplayType = InfoBoxFactory::NavAltitude;

    bool needupdate = ((DisplayType != DisplayTypeLast[i]) || first);

    if (needupdate) {
      infoboxes[i]->SetTitle(gettext(InfoBoxFactory::GetCaption(DisplayType)));
      infoboxes[i]->SetContentProvider(InfoBoxFactory::Create(DisplayType));
      DisplayTypeLast[i] = DisplayType;
    }

    infoboxes[i]->UpdateContent();
  }

  first = false;
}

void
InfoBoxManager::InfoBoxDrawIfDirty()
{
  // No need to redraw map or infoboxes if screen is blanked.
  // This should save lots of battery power due to CPU usage
  // of drawing the screen

  if (infoboxes_dirty && !infoboxes_hidden &&
      !CommonInterface::GetUIState().screen_blanked) {
    DisplayInfoBox();
    infoboxes_dirty = false;
  }
}

void
InfoBoxManager::SetDirty()
{
  infoboxes_dirty = true;
}

void
InfoBoxManager::ProcessTimer()
{
  InfoBoxDrawIfDirty();
}

void
InfoBoxManager::Create(ContainerWindow &parent,
                       const InfoBoxLayout::Layout &_layout,
                       const InfoBoxLook &look)
{
  const InfoBoxSettings &settings =
    CommonInterface::GetUISettings().info_boxes;

  first = true;
  layout = _layout;

  WindowStyle style;
  style.Hide();

  // create infobox windows
  for (unsigned i = layout.count; i-- > 0;) {
    const PixelRect &rc = layout.positions[i];
    int Border =
      settings.border_style == InfoBoxSettings::BorderStyle::TAB
      ? 0
      /* layout.geometry is the effective layout, while
         settings.geometry is the configured layout */
      : InfoBoxLayout::GetBorder(layout.geometry, layout.landscape, i);

    infoboxes[i] = new InfoBoxWindow(parent, rc,
                                     Border, settings, look,
                                     i, style);
  }

  infoboxes_hidden = true;
}

void
InfoBoxManager::Destroy()
{
  for (unsigned i = 0; i < layout.count; i++) {
    delete infoboxes[i];
    infoboxes[i] = NULL;
  }
}

void
InfoBoxManager::ShowInfoBoxPicker(const int i)
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  InfoBoxSettings::Panel &panel = settings.panels[panel_index];

  const InfoBoxFactory::Type old_type = panel.contents[i];

  ComboList list;
  for (unsigned j = InfoBoxFactory::MIN_TYPE_VAL; j < InfoBoxFactory::NUM_TYPES; j++) {
    const TCHAR *desc = InfoBoxFactory::GetDescription((InfoBoxFactory::Type)j);
    list.Append(j, gettext(InfoBoxFactory::GetName((InfoBoxFactory::Type)j)),
                gettext(InfoBoxFactory::GetName((InfoBoxFactory::Type)j)),
                desc != NULL ? gettext(desc) : NULL);
  }

  list.Sort();
  list.current_index = list.LookUp(old_type);

  /* let the user select */

  StaticString<20> caption;
  caption.Format(_T("%s: %d"), _("InfoBox"), i + 1);
  int result = ComboPicker(caption, list, nullptr, true);
  if (result < 0)
    return;

  /* was there a modification? */

  InfoBoxFactory::Type new_type = (InfoBoxFactory::Type)list[result].int_value;
  if (new_type == old_type)
    return;

  /* yes: apply and save it */

  panel.contents[i] = new_type;
  DisplayInfoBox();

  Profile::Save(Profile::map, panel, panel_index);
}
