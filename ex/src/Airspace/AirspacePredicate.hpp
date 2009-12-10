#ifndef AIRSPACE_PREDICATE_HPP
#define AIRSPACE_PREDICATE_HPP

class AbstractAirspace;
class AirspacePredicateTrue;
class AIRCRAFT_STATE;

/**
 *  Functor class for conditions to be applied to airspace queries
 */
class AirspacePredicate
{
public:
/** 
 * Test condition
 * @param t Airspace to test
 * @return True if condition met
 */
  virtual bool operator()( const AbstractAirspace& t ) const = 0;

/** 
 * Test condition (calls operator() method)
 * @param t Airspace to test
 * @return True if condition met
 */
  bool condition(const AbstractAirspace&t) const {
    return (*this)(t);
  }

  static AirspacePredicateTrue always_true; /**< Convenience condition, useful for default conditions */
};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateTrue: public AirspacePredicate
{
public:
  bool operator()( const AbstractAirspace& t ) const { return true; }

};

/**
 * Convenience predicate for conditions always true
 */
class AirspacePredicateAircraftInside: public AirspacePredicate
{
public:
  AirspacePredicateAircraftInside(const AIRCRAFT_STATE& state);
  bool operator()( const AbstractAirspace& t ) const;
private:
  const AIRCRAFT_STATE& m_state;
};

#endif
