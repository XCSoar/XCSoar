/* Copyright_License {

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

#include "DataNode.hpp"

#include <stdio.h>

DataNode::~DataNode()
{
}

void
DataNode::set_attribute(const TCHAR *name, Angle value)
{
  set_attribute(name, value.Degrees());
}

void
DataNode::set_attribute(const TCHAR *name, fixed value)
{
  TCHAR buf[100];
  _stprintf(buf, _T("%g"), (double)value);
  set_attribute(name, buf);
}

void
DataNode::set_attribute(const TCHAR *name, int value)
{
  TCHAR buf[100];
  _stprintf(buf, _T("%d"), value);
  set_attribute(name, buf);
}

void
DataNode::set_attribute(const TCHAR *name, unsigned value)
{
  TCHAR buf[100];
  _stprintf(buf, _T("%d"), value);
  set_attribute(name, buf);
}

void
DataNode::set_attribute(const TCHAR *name, bool &value)
{
  TCHAR buf[100];
  _stprintf(buf, _T("%d"), (int)value);
  set_attribute(name, buf);
}

bool
DataNode::get_attribute(const TCHAR *name, Angle &value) const
{
  fixed v;
  if (get_attribute(name, v)) {
    value = Angle::Degrees(v);
    return true;
  } else
    return false;
}

bool
DataNode::get_attribute(const TCHAR *name, fixed &value) const
{
  tstring val;
  if (get_attribute(name, val)) {
    value = (fixed)_tcstod(val.c_str(), NULL);
    return true;
  } else {
    return false;
  }
}

bool
DataNode::get_attribute(const TCHAR *name, int &value) const
{
  tstring val;
  if (get_attribute(name, val)) {
    value = _tcstol(val.c_str(), NULL, 0);
    return true;
  } else {
    return false;
  }
}

bool
DataNode::get_attribute(const TCHAR *name, unsigned &value) const
{
  tstring val;
  if (get_attribute(name, val)) {
    value = _tcstol(val.c_str(), NULL, 0);
    return true;
  } else {
    return false;
  }
}

bool
DataNode::get_attribute(const TCHAR *name, bool &value) const
{
  tstring val;
  if (get_attribute(name, val)) {
    value = (_tcstol(val.c_str(), NULL, 0) > 0);
    return true;
  } else {
    return false;
  }
}

