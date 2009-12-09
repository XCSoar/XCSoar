#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

class AbstractAirspace;
class AirspacePredicateTrue;

class AirspacePredicate
{
public:
  virtual bool operator()( const AbstractAirspace& t ) const = 0;
  bool condition(const AbstractAirspace&t) const {
    return (*this)(t);
  }
  static AirspacePredicateTrue always_true;
};

class AirspacePredicateTrue: public AirspacePredicate
{
public:
  bool operator()( const AbstractAirspace& t ) const { return true; }

};

#endif
