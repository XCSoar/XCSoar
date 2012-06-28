/*
 * Copyright (C) 2012 Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "json.hpp"
#include "IO/TextWriter.hpp"
#include "Compiler.h"

namespace json {

void
Node::SerialiseIndent(TextWriter &writer, int indent)
{
  for (int i = 0; i < indent; ++i)
    writer.Write('\t');
}

void
None::Serialise(TextWriter &writer, gcc_unused int indent) const
{
  writer.Write("null");
}

void
Bool::Serialise(TextWriter &writer, gcc_unused int indent) const
{
  writer.Write(value ? "true": "false");
}

void
Integer::Serialise(TextWriter &writer, gcc_unused int indent) const
{
  writer.Format("%d", value);
}

void
Double::Serialise(TextWriter &writer, gcc_unused int indent) const
{
  writer.Format("%f", value);
}

void
String::Serialise(TextWriter &writer, gcc_unused int indent) const
{
  writer.Format("\"%s\"", value.c_str());
}

Array::~Array()
{
  for (auto it = children.begin(), end = children.end(); it != end; ++it)
    delete *it;
}

void
Array::Add()
{
  children.push_back(new None());
}

void
Array::Add(bool value)
{
  children.push_back(new Bool(value));
}

void
Array::Add(long value)
{
  children.push_back(new Integer(value));
}

void
Array::Add(double value)
{
  children.push_back(new Double(value));
}

void
Array::Add(const TCHAR *value)
{
  children.push_back(new String(value));
}

void
Array::Add(tstring value)
{
  children.push_back(new String(value));
}

void
Array::Add(Array* value)
{
  children.push_back(value);
}

void
Array::Add(Object* value)
{
  children.push_back(value);
}

void
Array::Serialise(TextWriter &writer, int indent) const
{
  writer.Write('[');
  if (indent >= 0)
    writer.NewLine();

  for (auto it = children.begin(), end = children.end(); it != end;) {
    if (indent >= 0) {
      SerialiseIndent(writer, indent + 1);
      (*it)->Serialise(writer, indent + 1);

      if (++it != end)
        writer.Write(',');

      writer.NewLine();
    } else {
      (*it)->Serialise(writer, indent);

      if (++it != end)
        writer.Write(',');
    }
  }

  if (indent >= 0)
    SerialiseIndent(writer, indent);

  writer.Write(']');
}

Object::~Object()
{
  for (auto it = children.begin(), end = children.end(); it != end; ++it)
    delete it->second;
}

void
Object::Add(tstring key)
{
  children[key] = new None();
}

void
Object::Add(tstring key, bool value)
{
  children[key] = new Bool(value);
}

void
Object::Add(tstring key, long value)
{
  children[key] = new Integer(value);
}

void
Object::Add(tstring key, double value)
{
  children[key] = new Double(value);
}

void
Object::Add(tstring key, const TCHAR *value)
{
  children[key] = new String(value);
}

void
Object::Add(tstring key, tstring value)
{
  children[key] = new String(value);
}

void
Object::Add(tstring key, Array* value)
{
  children[key] = value;
}

void
Object::Add(tstring key, Object* value)
{
  children[key] = value;
}

void
Object::Serialise(TextWriter &writer, int indent) const
{
  writer.Write('{');
  if (indent >= 0)
    writer.NewLine();

  for (auto it = children.begin(), end = children.end(); it != end;) {
    if (indent >= 0) {
      SerialiseIndent(writer, indent + 1);
      writer.Format("\"%s\": ", it->first.c_str());
      it->second->Serialise(writer, indent + 1);
      if (++it != end)
        writer.Write(',');
      writer.NewLine();
    } else {
      writer.Format("\"%s\":", it->first.c_str());
      it->second->Serialise(writer, indent);
      if (++it != end)
        writer.Write(',');
    }
  }

  if (indent >= 0)
    SerialiseIndent(writer, indent);

  writer.Write('}');
}

}
