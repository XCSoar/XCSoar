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

#include <algorithm>
#include "AirspaceLabelList.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"

class AirspaceLabelListCompare {
  const AirspaceWarningConfig &config;

public:
  AirspaceLabelListCompare(const AirspaceWarningConfig &_config)
    :config(_config) {}

  bool operator() (const AirspaceLabelList::Label &label1,
                   const AirspaceLabelList::Label &label2) {
    bool en1 = config.IsClassEnabled(label1.cls);
    bool en2 = config.IsClassEnabled(label2.cls);

    if(en1 == en2)
      return AirspaceAltitude::SortHighest(label2.base, label1.base);
    else if(en1)
      return false;
    else
      return true;
  }
};

void
AirspaceLabelList::Add(const GeoPoint &pos, AirspaceClass cls, 
                       const AirspaceAltitude &base, const AirspaceAltitude &top)
{
  if (labels.full())
    return;

  auto &label = labels.append();
  label.cls = cls;
  label.pos = pos;
  label.base = base;
  label.top = top;
}

void
AirspaceLabelList::Sort(const AirspaceWarningConfig &config)
{
  AirspaceLabelListCompare compare(config);
  std::sort(labels.begin(), labels.end(), compare);
}
