/* Copyright_License {

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

#include "DataNode.hpp"
#include "Math/Angle.hpp"
#include "Util/StaticString.hxx"
#include "Util/NumberParser.hpp"
#include "Time/RoughTime.hpp"

ConstDataNode::~ConstDataNode()
{
}

bool
ConstDataNode::GetAttribute(const TCHAR *name, Angle &value) const
{
  double v;
  if (GetAttribute(name, v)) {
    value = Angle::Degrees(v);
    return true;
  } else
    return false;
}

bool
ConstDataNode::GetAttribute(const TCHAR *name, double &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == nullptr)
    return false;

  value = ParseDouble(val);
  return true;
}

bool
ConstDataNode::GetAttribute(const TCHAR *name, int &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == nullptr)
    return false;

  value = ParseInt(val);
  return true;
}

bool
ConstDataNode::GetAttribute(const TCHAR *name, unsigned &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == nullptr)
    return false;

  value = ParseInt(val);
  return true;
}

bool
ConstDataNode::GetAttribute(const TCHAR *name, bool &value) const
{
  const TCHAR *val = GetAttribute(name);
  if (val == nullptr)
    return false;

  value = ParseInt(val) > 0;
  return true;
}

RoughTime
ConstDataNode::GetAttributeRoughTime(const TCHAR *name) const
{
  const TCHAR *p = GetAttribute(name);
  if (p == nullptr)
    return RoughTime::Invalid();

  TCHAR *endptr;
  unsigned hours = ParseUnsigned(p, &endptr, 10);
  if (endptr == p || *endptr != ':' || hours >= 24)
    return RoughTime::Invalid();

  p = endptr + 1;
  unsigned minutes = ParseUnsigned(p, &endptr, 10);
  if (endptr == p || *endptr != 0 || minutes >= 60)
    return RoughTime::Invalid();

  return RoughTime(hours, minutes);
}

RoughTimeSpan
ConstDataNode::GetAttributeRoughTimeSpan(const TCHAR *start_name,
                                    const TCHAR *end_name) const
{
  return RoughTimeSpan(GetAttributeRoughTime(start_name),
                       GetAttributeRoughTime(end_name));
}

WritableDataNode::~WritableDataNode()
{
}

void
WritableDataNode::SetAttribute(const TCHAR *name, Angle value)
{
  SetAttribute(name, value.Degrees());
}

void
WritableDataNode::SetAttribute(const TCHAR *name, double value)
{
  StaticString<48> buf;
  buf.UnsafeFormat(_T("%g"), (double)value);
  SetAttribute(name, buf);
}

void
WritableDataNode::SetAttribute(const TCHAR *name, int value)
{
  StaticString<24> buf;
  buf.UnsafeFormat(_T("%d"), value);
  SetAttribute(name, buf);
}

void
WritableDataNode::SetAttribute(const TCHAR *name, unsigned value)
{
  StaticString<24> buf;
  buf.UnsafeFormat(_T("%d"), value);
  SetAttribute(name, buf);
}

void
WritableDataNode::SetAttribute(const TCHAR *name, bool value)
{
  StaticString<4> buf;
  buf.UnsafeFormat(_T("%d"), (int)value);
  SetAttribute(name, buf);
}

void
WritableDataNode::SetAttribute(const TCHAR *name, RoughTime value)
{
  if (!value.IsValid())
    /* no-op */
    return;

  StaticString<8> buffer;
  buffer.UnsafeFormat(_T("%02u:%02u"), value.GetHour(), value.GetMinute());
  SetAttribute(name, buffer);
}
