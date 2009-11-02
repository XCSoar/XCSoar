#ifndef GENERIC_VISITOR_HPP
#define GENERIC_VISITOR_HPP

#include <vector>

// Adapted from
// "Modern C++ design: generic programming and design patterns applied"
// By Andrei Alexandrescu
//
// Adapted to handle access via libkdtree methods,
//  and for const-ness of visitable items.
// Also added utility functions

class BaseVisitor
{
public:
  virtual ~BaseVisitor() {}
};

template <class T, typename R = void>
class Visitor
{
public:
  typedef R ReturnType; // Available for clients
  virtual ReturnType Visit(const T&) = 0;
};

template <class T>
class TreeVisitor: public BaseVisitor
{
public:
  typedef std::vector< T > TVector;
  typedef typename TVector::const_iterator TVectorIterator;

  void for_each(const TVector &v) {
    for (TVectorIterator i= v.begin(); i!= v.end(); i++) {
      i->Accept(*this);
    }
  }
  void operator()(const T& as) {
    as.Accept(*this);
  }
};

template <typename R = void>
class BaseVisitable
{
public:
  typedef R ReturnType;
  virtual ~BaseVisitable() {}
  virtual R Accept(BaseVisitor&) const = 0;
protected:
  template <class T>
  static ReturnType AcceptImpl(const T& visited, BaseVisitor& guest)
    {
      // Apply the acyclic visitor
      if (Visitor<T>* p = 
          dynamic_cast<Visitor<T>*>(&guest)) {
        return p->Visit(visited);
      }
      return ReturnType();
    }
};

#define DEFINE_VISITABLE() \
  virtual ReturnType Accept(BaseVisitor& guest) const \
  { return AcceptImpl(*this, guest); }

#endif
