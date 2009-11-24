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

/**
 * Generic visitor
 */
template <class T, typename R = void>
class Visitor
{
public:
  typedef R ReturnType; // Available for clients

/** 
 * Abstract visit method; this prototype method is called
 * on accepting instances.
 * 
 * @return Return value of visitor
 */
  virtual ReturnType Visit(const T&) = 0;
};

/**
 * Special visitor that can visit items within a kd-tree
 */
template <class T>
class TreeVisitor: public BaseVisitor
{
public:
  typedef std::vector< T > TVector;
  typedef typename TVector::const_iterator TVectorIterator;

/** 
 * Utility function to call visitor on all items in a vector
 * 
 * @param v Vector of items to be visited
 */
  void for_each(const TVector &v) {
    for (TVectorIterator i= v.begin(); i!= v.end(); i++) {
      i->Accept(*this);
    }
  }

/** 
 * Utility accessor to visit an item by calling the visitor with () operator
 * as used by libkdtree++
 */
  void operator()(const T& as) {
    as.Accept(*this);
  }
};

/**
 * Class from which to inherit for a class to be visitable
 */
template <typename R = void>
class BaseVisitable
{
public:
  typedef R ReturnType;
  virtual ~BaseVisitable() {}

/** 
 * Double-dispatch abstract accept method for items that
 * can be visited.
 * 
 * @return Return value of Visitor
 */
  virtual R Accept(BaseVisitor&) const = 0;
protected:

/** 
 * Dispatcher for visitor-visitable double dispatch system
 * 
 * @param visited Item to be visited
 * @param guest Guest visitor to be called on visited item
 * 
 * @return Return value of guest
 */
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
