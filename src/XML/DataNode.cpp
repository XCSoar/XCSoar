/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Math/Angle.hpp"
#include "Util/StaticString.hpp"
#include "Util/NumberParser.hpp"

#include <stdio.h>

DataNode::~DataNode()
{
}

void
DataNode::SetAttribute(const TCHAR *name, Angle value)
{
  SetAttribute(name, value.Degrees());
}

void
DataNode::SetAttribute(const TCHAR *name, fixed value)
{
  StaticString<48> buf;
  buf.UnsafeFormat(_T("%g"), (double)value);
  SetAttribute(name, buf);
}

void
DataNode::SetAttribute(const TCHAR *name, int value)
{
  StaticString<24> buf;
  buf.UnsafeFormat(_T("%d"), value);
  SetAttribute(name, buf);
}

void
DataNode::SetAttribute(const TCHAR *name, unsigned value)
{
  StaticString<24> buf;
  buf.UnsafeFormat(_T("%d"), value);
  SetAttribute(name, buf);
}

void
DataNode::SetAttribute(const TCHAR *name, bool &value)
{
  StaticString<4> buf;
  buf.UnsafeFormat(_T("%d"), (int)value);
  SetAttribute(name, buf);
}

bool
DataNode::GetAttribute(const TCHAR *name, Angle &value) const
{
  fixed v;
  if (GetAttribute(name, v)) {
    value = Angle::Degrees(v);
    return true;
  } else
    return false;
}

bool
DataNode::GetAttribute(const TCHAR *name, fixed &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == NULL)
    return false;

  value = (fixed)_tcstod(val, NULL);
  return true;
}

bool
DataNode::GetAttribute(const TCHAR *name, int &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == NULL)
    return false;

  value = ParseInt(val);
  return true;
}

bool
DataNode::GetAttribute(const TCHAR *name, unsigned &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == NULL)
    return false;

  value = ParseInt(val);
  return true;
}

bool
DataNode::GetAttribute(const TCHAR *name, bool &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == NULL)
    return false;

  value = ParseInt(val) > 0;
  return true;
}

