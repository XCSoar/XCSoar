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

#ifndef JSON_HPP
#define JSON_HPP

#include "Util/tstring.hpp"

#include <stdint.h>
#include <map>
#include <list>

class TextWriter;

namespace json {

class Node
{
public:
  enum class Type: uint8_t {
    NONE,
    BOOL,
    INTEGER,
    DOUBLE,
    STRING,
    ARRAY,
    OBJECT,
  };

private:
  Type type;

protected:
  Node(Type _type): type(_type) {}

public:
  virtual ~Node() {}

  Type GetType() const {
    return type;
  }

  virtual void Serialise(TextWriter &writer, int indent = 0) const = 0;

protected:
  static void SerialiseIndent(TextWriter &writer, int indent);
};


class None;
class Bool;
class Double;
class String;
class Array;
class Object;


class None: public Node
{
public:
  None():Node(Type::NONE) {}

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class Bool: public Node
{
  bool value;

public:
  Bool(bool _value):Node(Type::BOOL), value(_value) {}

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class Integer: public Node
{
  long value;

public:
  Integer(long _value):Node(Type::INTEGER), value(_value) {}

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class Double: public Node
{
  double value;

public:
  Double(double _value):Node(Type::DOUBLE), value(_value) {}

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class String: public Node
{
  tstring value;

public:
  String(tstring _value):Node(Type::STRING), value(_value) {}

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class Array: public Node
{
  typedef std::list<Node *> List;
  List children;

public:
  Array():Node(Type::ARRAY) {}
  ~Array();

  void Add();
  void Add(bool value);
  void Add(long value);
  void Add(double value);
  void Add(const TCHAR *value);
  void Add(tstring value);
  void Add(Array *value);
  void Add(Object *value);

  const List &GetChildren() const {
    return children;
  }

  void Serialise(TextWriter &writer, int indent = 0) const;
};


class Object: public Node
{
  typedef std::map<tstring, Node *> Map;
  Map children;

public:
  Object():Node(Type::OBJECT) {}
  ~Object();

  void Add(tstring key);
  void Add(tstring key, bool value);
  void Add(tstring key, long value);
  void Add(tstring key, double value);
  void Add(tstring key, const TCHAR *value);
  void Add(tstring key, tstring value);
  void Add(tstring key, Array *value);
  void Add(tstring key, Object *value);

  const Map &GetChildren() const {
    return children;
  }

  void Serialise(TextWriter &writer, int indent = 0) const;
};

}

#endif
