/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Waypoint.hpp"
#include "Form/Edit.hpp"
#include "Form/Button.hpp"
#include "LocalPath.hpp"
#include "Protection.hpp"
#include "ConfigPanel.hpp"
#include "SiteConfigPanel.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/XML.hpp"


using namespace ConfigPanel;

class SiteConfigPanel : public XMLWidget {
private:
  WndButton *buttonWaypoints;

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
SiteConfigPanel::Show(const PixelRect &rc)
{
  buttonWaypoints->set_visible(true);
  XMLWidget::Show(rc);
}

void
SiteConfigPanel::Hide()
{
  buttonWaypoints->set_visible(false);
  XMLWidget::Hide();
}

static void
OnWaypoints(gcc_unused WndButton &button)
{
  dlgConfigWaypointsShowModal();
}

void
SiteConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_SITECONFIGPANEL") :
                               _T("IDR_XML_SITECONFIGPANEL_L"));

  buttonWaypoints = ((WndButton *)ConfigPanel::GetForm().
    FindByName(_T("cmdWaypoints")));
  assert (buttonWaypoints);
  buttonWaypoints->SetOnClickNotify(OnWaypoints);

  InitFileField(form, _T("prpAirspaceFile"),
                szProfileAirspaceFile, _T("*.txt\0*.air\0*.sua\0"));
  InitFileField(form, _T("prpAdditionalAirspaceFile"),
                szProfileAdditionalAirspaceFile, _T("*.txt\0*.air\0*.sua\0"));
  InitFileField(form, _T("prpWaypointFile"),
                szProfileWaypointFile, _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  InitFileField(form, _T("prpAdditionalWaypointFile"),
                szProfileAdditionalWaypointFile,
                _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));
  InitFileField(form, _T("prpWatchedWaypointFile"),
                szProfileWatchedWaypointFile,
                _T("*.dat\0*.xcw\0*.cup\0*.wpz\0*.wpt\0"));

  WndProperty *wp = (WndProperty *)form.FindByName(_T("prpDataPath"));
  wp->set_enabled(false);
  wp->SetText(GetPrimaryDataPath());

  InitFileField(form, _T("prpMapFile"),
                szProfileMapFile, _T("*.xcm\0*.lkm\0"));
  InitFileField(form, _T("prpTerrainFile"),
                szProfileTerrainFile, _T("*.jp2\0"));
  InitFileField(form, _T("prpTopographyFile"),
                szProfileTopographyFile, _T("*.tpl\0"));
  InitFileField(form, _T("prpAirfieldFile"),
                szProfileAirfieldFile, _T("*.txt\0"));
}

bool
SiteConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  WaypointFileChanged = WaypointFileChanged |
    FinishFileField(form, _T("prpWaypointFile"), szProfileWaypointFile) |
    FinishFileField(form, _T("prpAdditionalWaypointFile"),
                    szProfileAdditionalWaypointFile) |
    FinishFileField(form, _T("prpWatchedWaypointFile"),
                    szProfileWatchedWaypointFile);

  AirspaceFileChanged =
    FinishFileField(form, _T("prpAirspaceFile"), szProfileAirspaceFile) |
    FinishFileField(form, _T("prpAdditionalAirspaceFile"),
                    szProfileAdditionalAirspaceFile);

  MapFileChanged = FinishFileField(form, _T("prpMapFile"), szProfileMapFile);

  TerrainFileChanged = FinishFileField(form, _T("prpTerrainFile"),
                                       szProfileTerrainFile);

  TopographyFileChanged = FinishFileField(form, _T("prpTopographyFile"),
                                        szProfileTopographyFile);

  AirfieldFileChanged = FinishFileField(form, _T("prpAirfieldFile"),
                                        szProfileAirfieldFile);


  changed = WaypointFileChanged || AirfieldFileChanged || MapFileChanged ||
         TerrainFileChanged || TopographyFileChanged || AirfieldFileChanged;

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateSiteConfigPanel()
{
  return new SiteConfigPanel();
}
