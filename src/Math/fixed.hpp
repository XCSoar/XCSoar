#ifndef FIXED_HPP
#define FIXED_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
//
// Extensions and bug/compilation fixes by John Wharington 2009

#include "Compiler.h"
#include "Constants.h"

#include <utility>
#include <complex>

#include <assert.h>

#define fixed_third fixed(1./3.)
#define fixed_two_thirds fixed(2./3.)

#define fixed_pi fixed(M_PI)
#define fixed_two_pi fixed(M_2PI)
#define fixed_half_pi fixed(M_HALFPI)

#define fixed_sqrt_two fixed(1.4142135623730951)
#define fixed_sqrt_half fixed(0.70710678118654757)

#ifndef FIXED_MATH
#include <math.h>
#define FIXED_DOUBLE(x) (x)
#define FIXED_INT(x) ((int)x)
typedef double fixed;

gcc_const
static inline std::pair<fixed, fixed>
sin_cos(const fixed thetha)
{
#if defined(__GLIBC__) && defined(_GNU_SOURCE)
  double s, c;
  sincos(thetha, &s, &c);
  return std::make_pair(s, c);
#else
  return std::make_pair(sin(thetha), cos(thetha));
#endif
}

#define positive(x) (x > 0)
#define negative(x) (x < 0)
#define sigmoid(x) (2.0 / (1.0 + exp(-x)) - 1.0)

constexpr
static inline fixed
Half(fixed a)
{
  return a * 0.5;
}

constexpr
static inline fixed
Quarter(fixed a)
{
  return a * 0.25;
}

constexpr
static inline fixed
Double(fixed a)
{
  return a * 2;
}

gcc_const
inline fixed rsqrt(fixed a) {
  // not fast
  return 1.0/sqrt(a);
}

gcc_const
inline fixed fast_sqrt(fixed a) {
  // not fast
  return sqrt(a);
}

constexpr
inline fixed sqr(fixed a) {
  return a*a;
}

constexpr
inline fixed fast_mult(fixed a, int a_bits, fixed b, int b_bits)
{
  return a * b;
}

constexpr
inline fixed fast_mult(fixed a, fixed b, int b_bits)
{
  return a * b;
}

gcc_const
inline fixed accurate_half_sin(fixed a) {
  return sin(a/2);
}

#else
#define FIXED_DOUBLE(x) x.as_double()
#define FIXED_INT(x) x.as_int()

#include <type_traits>
#include <complex>
#include <climits>

#ifdef HAVE_BOOST
#include <boost/cstdint.hpp>
#else
#include <stdint.h>
#endif

class fixed
{
#ifdef HAVE_BOOST
  typedef boost::int64_t int64_t;
  typedef boost::uint64_t uint64_t;
#endif
  typedef uint64_t uvalue_t;

public:
  typedef int64_t value_t;

  static constexpr unsigned resolution_shift = 28;
  static constexpr value_t resolution = 1 << resolution_shift;
  static constexpr unsigned accurate_cordic_shift = 11;

private:
    value_t m_nVal;

public:

  struct internal {};

  fixed() = default;

  constexpr
  fixed(internal, value_t nVal)
    :m_nVal(nVal) {}

//    fixed(value_t nVal):
//        m_nVal(nVal<<resolution_shift)
//    {}

  constexpr explicit
  fixed(long nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

  constexpr explicit
  fixed(int nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

  constexpr explicit
  fixed(short nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}

/*    
    fixed(unsigned value_t nVal):
        m_nVal(nVal<<resolution_shift)
    {}
*/  
  constexpr explicit
  fixed(unsigned long nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  constexpr explicit
  fixed(unsigned int nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  constexpr explicit
  fixed(unsigned short nVal)
    :m_nVal((value_t)(nVal)<<resolution_shift) {}
  constexpr explicit
  fixed(double nVal)
    :m_nVal(static_cast<value_t>(nVal*static_cast<double>(resolution))) {}
  constexpr explicit
  fixed(float nVal)
    :m_nVal(static_cast<value_t>(nVal*static_cast<float>(resolution))) {}

  constexpr
  friend bool operator==(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal==rhs.m_nVal;
  }

  constexpr
  friend bool operator!=(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal!=rhs.m_nVal;
  }

  constexpr
  friend bool operator<(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal<rhs.m_nVal;
  }

  constexpr
  friend bool operator>(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal>rhs.m_nVal;
  }

  constexpr
  friend bool operator<=(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal<=rhs.m_nVal;
  }

  constexpr
  friend bool operator>=(const fixed lhs, const fixed rhs) {
    return lhs.m_nVal>=rhs.m_nVal;
  }

  constexpr explicit
  operator bool() const {
    return m_nVal != 0;
  }

  constexpr explicit
  inline operator double() const {
    return as_double();
  }

  constexpr explicit
  inline operator float() const {
    return as_float();
  }

  constexpr explicit
  inline operator short() const {
    return as_short();
  }

  constexpr explicit
  inline operator int() const {
    return as_int();
  }

  constexpr explicit
  inline operator unsigned() const {
    return (unsigned)as_int();
  }

  constexpr explicit
  inline operator unsigned short() const {
    return as_unsigned_short();
  }

  constexpr explicit
  inline operator long() const {
    return as_long();
  }

  constexpr
  float as_float() const {
    return m_nVal/(float)resolution;
  }

  constexpr
  double as_double() const {
    return m_nVal/(double)resolution;
  }

  constexpr
  long as_long() const {
    return (long)(m_nVal >> resolution_shift);
  }

  constexpr
  int64_t as_int64() const {
    return m_nVal >> resolution_shift;
  }

  constexpr
  int as_int() const {
    return (int)(m_nVal >> resolution_shift);
  }

  constexpr
  unsigned long as_unsigned_long() const {
    return (unsigned long)(m_nVal >> resolution_shift);
  }

  /*
  uint64_t as_unsigned_int64() const {
    return (uint64_t)(m_nVal >> resolution_shift);
  }
  */

  constexpr
  unsigned int as_unsigned_int() const {
    return (unsigned int)(m_nVal >> resolution_shift);
  }

  constexpr
  short as_short() const {
    return (short)(m_nVal >> resolution_shift);
  }

  constexpr
  unsigned short as_unsigned_short() const {
    return (unsigned short)(m_nVal >> resolution_shift);
  }

  // TODO: be more generic
  constexpr
  long as_glfixed() const {
    //assert(resolution_shift >= 16);
    return m_nVal >> (resolution_shift - 16);
  }

  // TODO: be more generic
  constexpr
  long as_glfixed_scale() const {
    //assert(resolution_shift <= 32);
    return m_nVal << (32 - resolution_shift);
  }

  fixed operator++() {
    m_nVal += resolution;
    return *this;
  }

  fixed operator--() {
    m_nVal -= resolution;
    return *this;
  }

  constexpr
  bool positive() const;

  constexpr
  bool negative() const;

  constexpr
  fixed Half() const {
    return fixed(internal(), m_nVal >> 1);
  }

  constexpr
  fixed Quarter() const {
    return fixed(internal(), m_nVal >> 2);
  }

  constexpr
  fixed Double() const {
    return fixed(internal(), m_nVal << 1);
  }

  gcc_pure
  fixed trunc() const {
    value_t x = m_nVal;
    if (x < 0)
      x += resolution - 1;
    return fixed(fixed::internal(),
                 x & ((int64_t)-1 << resolution_shift));
  }

  constexpr fixed floor() const {
    return fixed(fixed::internal(),
                 m_nVal & ~(resolution - 1));
  }

  constexpr fixed ceil() const {
    return fixed(fixed::internal(),
                 ((m_nVal - 1) | (resolution - 1)) + 1);
  }

  gcc_pure
  fixed sqrt() const;

  gcc_pure
  fixed fast_sqrt() const;

  gcc_pure
  fixed sqr() const;

  gcc_pure
  fixed rsqrt() const;

  gcc_pure
  fixed exp() const;

  gcc_pure
  fixed log() const;

  constexpr
  friend fixed operator+(const fixed a, const fixed b) {
    return fixed(fixed::internal(), a.m_nVal + b.m_nVal);
  }

  constexpr
  friend fixed operator-(const fixed a, const fixed b) {
    return fixed(fixed::internal(), a.m_nVal - b.m_nVal);
  }

  constexpr
  friend fixed operator*(const fixed a, const long b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const unsigned long b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const int b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const unsigned int b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const short b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const unsigned short b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const char b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator*(const fixed a, const unsigned char b) {
    return fixed(fixed::internal(), a.m_nVal * b);
  }

  constexpr
  friend fixed operator/(const fixed a, const long b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const unsigned long b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const int b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const unsigned int b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const short b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const unsigned short b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const char b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  constexpr
  friend fixed operator/(const fixed a, const unsigned char b) {
    return fixed(fixed::internal(), a.m_nVal / b);
  }

  fixed& operator%=(const fixed other);
  fixed& operator*=(const fixed val);
  fixed& operator/=(fixed const divisor);
  fixed& operator-=(const fixed val) {
    m_nVal -= val.m_nVal;
    return *this;
  }

  fixed& operator+=(const fixed val) {
    m_nVal += val.m_nVal;
    return *this;
  }

/*
    fixed& operator*=(value_t val)
    {
        m_nVal*=val;
        return *this;
    }
*/

  fixed& operator*=(long val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(int val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(short val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(char val) {
    m_nVal*=val;
    return *this;
  }

/*
    fixed& operator*=(unsigned value_t val)
    {
        m_nVal*=val;
        return *this;
    }
*/

  fixed& operator*=(unsigned long val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned int val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned short val) {
    m_nVal*=val;
    return *this;
  }

  fixed& operator*=(unsigned char val) {
    m_nVal*=val;
    return *this;
  }

  constexpr
  static fixed fast_mult(fixed a, int a_shift, fixed b, int b_shift) {
    return fixed(internal(),
                 ((a.m_nVal >> a_shift) * (b.m_nVal >> b_shift))
                 >> (resolution_shift - a_shift - b_shift));
  }

/*
    fixed& operator/=(value_t val)
    {
        m_nVal/=val;
        return *this;
    }
*/

  fixed& operator/=(long val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(int val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(short val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(char val) {
    m_nVal/=val;
    return *this;
  }

/*
    fixed& operator/=(unsigned value_t val)
    {
        m_nVal/=val;
        return *this;
    }
*/

  fixed& operator/=(unsigned long val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned int val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned short val) {
    m_nVal/=val;
    return *this;
  }

  fixed& operator/=(unsigned char val) {
    m_nVal/=val;
    return *this;
  }

  constexpr
  fixed operator>>(int bits) const {
    return fixed(fixed::internal(), m_nVal >> bits);
  }

  constexpr
  fixed operator<<(int bits) const {
    return fixed(fixed::internal(), m_nVal << bits);
  }

  fixed &operator>>=(int bits) {
    m_nVal >>= bits;
    return *this;
  }

  fixed &operator<<=(int bits) {
    m_nVal <<= bits;
    return *this;
  }

  constexpr bool operator!() const {
    return m_nVal==0;
  }

  fixed modf(fixed* integral_part) const;

  gcc_pure
  fixed atan() const;

  gcc_const
  static std::pair<fixed, fixed> sin_cos(fixed theta);

  static void to_polar(const fixed x, const fixed y, fixed* r, fixed*theta);

  gcc_pure
  static fixed atan2(const fixed y, const fixed x);

  gcc_pure
  static fixed sigmoid(const fixed x);

  gcc_pure
  fixed sin() const {
    return sin_cos(*this).first;
  }

  gcc_pure
  fixed cos() const {
    return sin_cos(*this).second;
  }

  gcc_pure
  fixed tan() const {
    const auto sc = sin_cos(*this);
    fixed result = sc.first;
    result /= sc.second;
    return result;
  }

  gcc_pure
  fixed accurate_half_sin() const;

  constexpr
  fixed operator-() const;

  constexpr
  fixed abs() const;
};

static_assert(std::is_trivial<fixed>::value, "type is not trivial");

constexpr
inline bool fixed::positive() const
{
  return (m_nVal>0);
}

constexpr
inline bool fixed::negative() const
{
  return (m_nVal<0);
}

constexpr
inline fixed operator*(unsigned long a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(long a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(unsigned a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(int a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(unsigned short a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(short a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(unsigned char a, const fixed b)
{
  return b * a;
}

constexpr
inline fixed operator*(char a, const fixed b)
{
  return b * a;
}

gcc_pure
inline fixed operator*(const fixed a, const fixed b)
{
  fixed temp(a);
  return temp*=b;
}

/**
 * Simplified and faster multiplication when you know the range of
 * the coefficients at compile time.
 *
 * @param a first coefficient
 * @param a_shift number of bits to discard from the first coefficient
 * @param b second coefficient
 * @param b_shift number of bits to discard from the second coefficient
 */
constexpr
inline fixed
fast_mult(fixed a, int a_shift, fixed b, int b_shift)
{
  return fixed::fast_mult(a, a_shift, b, b_shift);
}

constexpr
inline fixed fast_mult(fixed a, fixed b, int b_bits)
{
  return fixed::fast_mult(a, 0, b, b_bits);
}

gcc_pure
inline fixed operator/(const fixed a, const fixed b)
{
  fixed temp(a);
  return temp/=b;
}

gcc_pure
static inline fixed pow(fixed x, fixed y)
{
  return fixed(pow((double)x, (double)y));
}

inline fixed sin(const fixed x)
{
  return x.sin();
}
inline fixed cos(const fixed x)
{
  return x.cos();
}
inline fixed tan(const fixed x)
{
  return x.tan();
}
inline fixed atan(const fixed x)
{
    return x.atan();
}
inline fixed accurate_half_sin(const fixed x)
{
  return x.accurate_half_sin();
}

gcc_pure
inline fixed atan2(const fixed y, const fixed x)
{
  return fixed::atan2(y,x);
}

static inline fixed asin(fixed x)
{
  return atan2(x, (fixed(1)-x*x).sqrt());
}

static inline fixed acos(fixed x)
{
  return atan2((fixed(1)-x*x).sqrt(), x);
}

gcc_pure
inline fixed sqr(const fixed x)
{
  return x.sqr();
}

gcc_pure
inline fixed sqrt(const fixed x)
{
  return x.sqrt();
}

gcc_pure
inline fixed fast_sqrt(const fixed x)
{
  assert(!x.negative());
  if (!x.positive())
    return fixed(0);
  return x.rsqrt()*x;
}

gcc_pure
inline fixed rsqrt(const fixed x)
{
  return x.rsqrt();
}

gcc_pure
inline fixed exp(const fixed x)
{
  return x.exp();
}

gcc_pure
inline fixed log(const fixed x)
{
  return x.log();
}

inline fixed trunc(fixed x)
{
  return x.trunc();
}

constexpr
static inline fixed
floor(fixed x)
{
  return x.floor();
}

constexpr
static inline fixed
ceil(fixed x)
{
  return x.ceil();
}

constexpr
inline fixed fabs(fixed const x)
{
  return x.abs();
}

inline fixed modf(const fixed x, fixed *integral_part)
{
  return x.modf(integral_part);
}

gcc_pure
static inline fixed fmod(fixed x, fixed y)
{
  return fixed(fmod((double)x, (double)y));
}

constexpr
inline fixed fixed::operator-() const
{
  return fixed(internal(),-m_nVal);
}

constexpr
inline fixed fixed::abs() const
{
  return fixed(internal(),m_nVal<0?-m_nVal:m_nVal);
}

inline fixed fixed::modf(fixed*integral_part) const
{
  value_t fractional_part=m_nVal%resolution;
  if(m_nVal<0 && fractional_part>0)
    {
      fractional_part-=resolution;
    }
  integral_part->m_nVal=m_nVal-fractional_part;
  return fixed(internal(),fractional_part);
}

gcc_const
static inline std::pair<fixed, fixed>
sin_cos(const fixed theta)
{
  return ::fixed::sin_cos(theta);
}

gcc_pure
inline fixed sigmoid(const fixed x)
{
  return ::fixed::sigmoid(x);
}

 namespace std
 {
   template<>
   inline ::fixed arg(const std::complex< ::fixed>& val)
   {
     ::fixed r,theta;
     ::fixed::to_polar(val.real(),val.imag(),&r,&theta);
     return theta;
   }

   template<>
   inline complex< ::fixed> polar(::fixed const& rho,::fixed const& theta)
   {
     const auto sc = ::fixed::sin_cos(theta);
     return complex< ::fixed>(rho * sc.second, rho * sc.first);
   }
 }

#define fixed_max fixed(fixed::internal(), 0x7fffffffffffffffLL)

inline fixed fixed::sigmoid(const fixed x) {
  return fixed(2) / (fixed(1) + (-x).exp()) - fixed(1);
}

constexpr
inline bool positive(const fixed f) {
  return f.positive();
}

constexpr
inline bool negative(const fixed f) {
  return f.negative();
}

constexpr
static inline fixed
Half(fixed a)
{
  return a.Half();
}

constexpr
static inline fixed
Quarter(fixed a)
{
  return a.Quarter();
}

constexpr
static inline fixed
Double(fixed a)
{
  return a.Double();
}

#endif

/**
 * Calculate the euclidian distance for "tiny" parameter values,
 * i.e. all values below 3.
 *
 * This function was calibrated for small delta angles,
 * e.g. reasonable distances on earth's surface.
 */
gcc_const
inline fixed
TinyHypot(fixed x, fixed y)
{
#ifdef FIXED_MATH
  /* shift 15 bits left to avoid underflow and precision loss in
     sqr() */
  return sqrt(sqr(x << 15) + sqr(y << 15)) >> 15;
#else
  return hypot(x, y);
#endif
}

/**
 * Calculate the euclidian distance for "small" parameter values,
 * i.e. values below 100,000.
 */
gcc_const
inline fixed
SmallHypot(fixed x, fixed y)
{
#ifdef FIXED_MATH
  return sqrt(sqr(x) + sqr(y));
#else
  return hypot(x, y);
#endif
}

/**
 * Calculate the euclidian distance for "medium" parameter values,
 * i.e. values below 1,000,000.
 */
gcc_const
inline fixed
MediumHypot(fixed x, fixed y)
{
#ifdef FIXED_MATH
  /* discarding the lower 3 bits to avoid integer overflow in sqr() */
  return sqrt(sqr(x >> 3) + sqr(y >> 3)) << 3;
#else
  return hypot(x, y);
#endif
}

/**
 * Calculate the euclidian distance for "large" parameter values,
 * i.e. values below 8,000,000,000.
 */
gcc_const
inline fixed
LargeHypot(fixed x, fixed y)
{
#ifdef FIXED_MATH
  /* discarding the lower 16 bits to avoid integer overflow in sqr() */
  return sqrt(sqr(x >> 16) + sqr(y >> 16)) << 16;
#else
  return hypot(x, y);
#endif
}

inline void limit_tolerance(fixed& f, const fixed tol_act) {
  if (fabs(f)<tol_act) {
    f = positive(f)? tol_act:-tol_act;
  }
}

/**
 * Convert this number to a signed integer, with rounding.
 */
gcc_const static inline int
iround(fixed x)
{
#ifdef FIXED_MATH
  return (int)(x + fixed(0.5));
#else
  return (int)lround(x);
#endif
}

/**
 * Convert this number to an unsigned integer, with rounding.
 */
gcc_const static inline unsigned
uround(const fixed x)
{
  return (unsigned)(x + fixed(0.5));
}

/**
 * The sign function.
 *
 * Returns 1 for positive values, -1 for negative values and 0 for zero.
 */
constexpr
static inline int
sgn(const fixed x)
{
  return positive(x) ? 1 : (negative(x) ? -1 : 0);
}

#endif
