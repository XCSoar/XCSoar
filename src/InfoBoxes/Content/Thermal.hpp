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

#ifndef XCSOAR_INFOBOX_CONTENT_THERMAL_HPP
#define XCSOAR_INFOBOX_CONTENT_THERMAL_HPP

#include "InfoBoxes/Content/Base.hpp"

#include "Form/TabBar.hpp"
#include "Form/Button.hpp"

class InfoBoxContentMacCready : public InfoBoxContent
{
public:
  virtual InfoBoxDlgContent* GetInfoBoxDlgContent();

  static Window* PnlEditLoad(SingleWindow &parent, TabBarControl* wTabBar, WndForm* wf, const int id);
  static void PnlEditOnCloseClicked(WndButton &Sender);
  static bool PnlEditOnTabPreShow(TabBarControl::EventType EventType);

  static void PnlEditOnPlusBig(WndButton &Sender);
  static void PnlEditOnPlusSmall(WndButton &Sender);
  static void PnlEditOnMinusSmall(WndButton &Sender);
  static void PnlEditOnMinusBig(WndButton &Sender);

  static CallBackTableEntry CallBackTable[];
  static InfoBoxPanelContent pnlEdit;
  static InfoBoxPanelContent pnlInfo;
  static InfoBoxPanelContent pnlSetup;
  static InfoBoxDlgContent dlgContent;

  static Window* PnlInfoLoad(SingleWindow &parent, TabBarControl* wTabBar, WndForm* wf, const int id);

  static Window* PnlSetupLoad(SingleWindow &parent, TabBarControl* wTabBar, WndForm* wf, const int id);
  static void PnlSetupOnSetup(WndButton &Sender);

  virtual void Update(InfoBoxWindow &infobox);
  virtual bool HandleKey(const InfoBoxKeyCodes keycode);
  virtual bool HandleQuickAccess(const TCHAR *misc);
};

class InfoBoxContentVario : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentVarioNetto : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermal30s : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalLastAvg : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalLastGain : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalLastTime : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalAllAvg : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalAvg : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalGain : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentThermalRatio : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

class InfoBoxContentVarioDistance : public InfoBoxContent
{
public:
  virtual void Update(InfoBoxWindow &infobox);
};

#endif
