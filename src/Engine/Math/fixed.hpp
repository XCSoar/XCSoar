#ifndef FIXED_HPP
#define FIXED_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
//
// Extensions and bug/compilation fixes by John Wharington 2009

#include <algorithm>
using std::max;
using std::min;

#ifndef FIXED_MATH
#include <math.h>
#define FIXED_DOUBLE(x) (x)
#define FIXED_INT(x) ((int)x)
typedef double fixed;
#define fixed_zero 0.0
#define fixed_half 0.5
#define fixed_one 1.0
#define fixed_two 2.0
#define fixed_four 4.0
#define fixed_deg_to_rad					0.0174532925199432958
#define fixed_rad_to_deg					57.2957795131
#define fixed_pi 3.1415926
#define fixed_two_pi 2.0*3.1415926
#define fixed_360 360
#define fixed_180 180

void sin_cos(const double&theta, double*s, double*c);
#define positive(x) (x>0)
#define negative(x) (x<0)

#else
#define FIXED_DOUBLE(x) x.as_double()
#define FIXED_INT(x) x.as_int()

#include <ostream>
#include <complex>
#include <climits>

#ifdef HAVE_BOOST
#include <boost/cstdint.hpp>
typedef boost::int64_t __int64;
typedef boost::uint64_t __uint64;
#else
#include <stdint.h>
typedef unsigned __int64    __uint64;
#endif

unsigned const fixed_resolution_shift=28;
__int64 const fixed_resolution=1<<fixed_resolution_shift; // JMW was LL

class fixed
{
private:
    __int64 m_nVal;

public:

    struct internal
    {};

    fixed():
        m_nVal(0)
    {}
    
    fixed(internal, __int64 nVal):
        m_nVal(nVal)
    {}
//    fixed(__int64 nVal):
//        m_nVal(nVal<<fixed_resolution_shift)
//    {}
    
    fixed(long nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}
    
    fixed(int nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}
    
    fixed(short nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}

/*    
    fixed(unsigned __int64 nVal):
        m_nVal(nVal<<fixed_resolution_shift)
    {}
*/  
    fixed(unsigned long nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}
    fixed(unsigned int nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}
    fixed(unsigned short nVal):
        m_nVal((__int64)(nVal)<<fixed_resolution_shift)
    {}
    fixed(double nVal):
        m_nVal(static_cast<__int64>(nVal*static_cast<double>(fixed_resolution)))
    {}
    fixed(float nVal):
        m_nVal(static_cast<__int64>(nVal*static_cast<float>(fixed_resolution)))
    {}

    template<typename T>
    fixed& operator=(T other)
    {
        m_nVal=fixed(other).m_nVal;
        return *this;
    }
    fixed& operator=(fixed const& other)
    {
        m_nVal=other.m_nVal;
        return *this;
    }
    friend bool operator==(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal==rhs.m_nVal;
    }
    friend bool operator!=(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal!=rhs.m_nVal;
    }
    friend bool operator<(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal<rhs.m_nVal;
    }
    friend bool operator>(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal>rhs.m_nVal;
    }
    friend bool operator<=(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal<=rhs.m_nVal;
    }
    friend bool operator>=(fixed const& lhs,fixed const& rhs)
    {
        return lhs.m_nVal>=rhs.m_nVal;
    }
    operator bool() const
    {
        return m_nVal?true:false;
    }
    inline operator double() const
    {
        return as_double();
    }
    inline operator short() const
    {
        return as_short();
    }
    inline operator int() const
    {
        return as_int();
    }
    inline operator unsigned short() const
    {
        return as_unsigned_short();
    }
    float as_float() const
    {
        return m_nVal/(float)fixed_resolution;
    }

    double as_double() const
    {
        return m_nVal/(double)fixed_resolution;
    }

    long as_long() const
    {
        return (long)(m_nVal/fixed_resolution);
    }
    __int64 as_int64() const
    {
        return m_nVal/fixed_resolution;
    }

    int as_int() const
    {
        return (int)(m_nVal/fixed_resolution);
    }

    unsigned long as_unsigned_long() const
    {
        return (unsigned long)(m_nVal/fixed_resolution);
    }
/*
    unsigned __int64 as_unsigned_int64() const
    {
        return (unsigned __int64)m_nVal/fixed_resolution;
    }
*/
    unsigned int as_unsigned_int() const
    {
        return (unsigned int)(m_nVal/fixed_resolution);
    }

    short as_short() const
    {
        return (short)(m_nVal/fixed_resolution);
    }

    unsigned short as_unsigned_short() const
    {
        return (unsigned short)(m_nVal/fixed_resolution);
    }

    fixed operator++()
    {
        m_nVal += fixed_resolution;
        return *this;
    }

    fixed operator--()
    {
        m_nVal -= fixed_resolution;
        return *this;
    }

  bool positive() const;
  bool negative() const;

    fixed floor() const;
    fixed ceil() const;
    fixed sqrt() const;
    fixed exp() const;
    fixed log() const;
    fixed& operator%=(fixed const& other);
    fixed& operator*=(fixed const& val);
    fixed& operator/=(fixed const& val);
    fixed& operator-=(fixed const& val)
    {
        m_nVal -= val.m_nVal;
        return *this;
    }

    fixed& operator+=(fixed const& val)
    {
        m_nVal += val.m_nVal;
        return *this;
    }
    fixed& operator*=(double val)
    {
        return (*this)*=fixed(val);
    }
    fixed& operator*=(float val)
    {
        return (*this)*=fixed(val);
    }
/*
    fixed& operator*=(__int64 val)
    {
        m_nVal*=val;
        return *this;
    }
*/
    fixed& operator*=(long val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(int val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(short val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(char val)
    {
        m_nVal*=val;
        return *this;
    }
/*
    fixed& operator*=(unsigned __int64 val)
    {
        m_nVal*=val;
        return *this;
    }
*/
    fixed& operator*=(unsigned long val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(unsigned int val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(unsigned short val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator*=(unsigned char val)
    {
        m_nVal*=val;
        return *this;
    }
    fixed& operator/=(double val)
    {
        return (*this)/=fixed(val);
    }
    fixed& operator/=(float val)
    {
        return (*this)/=fixed(val);
    }
/*
    fixed& operator/=(__int64 val)
    {
        m_nVal/=val;
        return *this;
    }
*/
    fixed& operator/=(long val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(int val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(short val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(char val)
    {
        m_nVal/=val;
        return *this;
    }
/*
    fixed& operator/=(unsigned __int64 val)
    {
        m_nVal/=val;
        return *this;
    }
*/
    fixed& operator/=(unsigned long val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(unsigned int val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(unsigned short val)
    {
        m_nVal/=val;
        return *this;
    }
    fixed& operator/=(unsigned char val)
    {
        m_nVal/=val;
        return *this;
    }
    

    bool operator!() const
    {
        return m_nVal==0;
    }
    
    fixed modf(fixed* integral_part) const;
    fixed atan() const;

    static void sin_cos(fixed const& theta,fixed* s,fixed*c);
    static void to_polar(fixed const& x,fixed const& y,fixed* r,fixed*theta);
    static fixed atan2(fixed const& y,fixed const& x);

    fixed sin() const;
    fixed cos() const;
    fixed tan() const;
    fixed operator-() const;
    fixed abs() const;
};

inline std::ostream& operator<<(std::ostream& os,fixed const& value)
{
    return os<<value.as_double();
}

inline bool fixed::positive() const
{
  return (m_nVal>0);
}

inline bool fixed::negative() const
{
  return (m_nVal<0);
}

inline fixed operator-(double a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}


inline fixed operator-(float a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(unsigned long a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(long a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(unsigned a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(int a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(unsigned short a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(short a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(unsigned char a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(char a, fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,double b)
{
    fixed temp(a);
    return temp-=b;
}


inline fixed operator-(fixed const& a,float b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,unsigned long b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,long b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,unsigned b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,int b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,unsigned short b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,short b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,unsigned char b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,char b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator-(fixed const& a,fixed const& b)
{
    fixed temp(a);
    return temp-=b;
}

inline fixed operator%(double a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}


inline fixed operator%(float a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(unsigned long a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(long a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(unsigned a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(int a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(unsigned short a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(short a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(unsigned char a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(char a, fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,double b)
{
    fixed temp(a);
    return temp%=b;
}


inline fixed operator%(fixed const& a,float b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,unsigned long b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,long b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,unsigned b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,int b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,unsigned short b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,short b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,unsigned char b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,char b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator%(fixed const& a,fixed const& b)
{
    fixed temp(a);
    return temp%=b;
}

inline fixed operator+(double a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}


inline fixed operator+(float a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(unsigned long a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(long a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(unsigned a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(int a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(unsigned short a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(short a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(unsigned char a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(char a, fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,double b)
{
    fixed temp(a);
    return temp+=b;
}


inline fixed operator+(fixed const& a,float b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,unsigned long b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,long b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,unsigned b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,int b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,unsigned short b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,short b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,unsigned char b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,char b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator+(fixed const& a,fixed const& b)
{
    fixed temp(a);
    return temp+=b;
}

inline fixed operator*(double a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}


inline fixed operator*(float a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(unsigned long a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(long a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(unsigned a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(int a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(unsigned short a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(short a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(unsigned char a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(char a, fixed const& b)
{
    fixed temp(b);
    return temp*=a;
}

inline fixed operator*(fixed const& a,double b)
{
    fixed temp(a);
    return temp*=b;
}


inline fixed operator*(fixed const& a,float b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,unsigned long b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,long b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,unsigned b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,int b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,unsigned short b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,short b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,unsigned char b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,char b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator*(fixed const& a,fixed const& b)
{
    fixed temp(a);
    return temp*=b;
}

inline fixed operator/(double a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}


inline fixed operator/(float a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(unsigned long a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(long a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(unsigned a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(int a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(unsigned short a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(short a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(unsigned char a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(char a, fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,double b)
{
    fixed temp(a);
    return temp/=b;
}


inline fixed operator/(fixed const& a,float b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,unsigned long b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,long b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,unsigned b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,int b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,unsigned short b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,short b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,unsigned char b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,char b)
{
    fixed temp(a);
    return temp/=b;
}

inline fixed operator/(fixed const& a,fixed const& b)
{
    fixed temp(a);
    return temp/=b;
}

inline bool operator==(double a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(float a, fixed const& b)
{
    return fixed(a)==b;
}

inline bool operator==(unsigned long a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(long a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(unsigned a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(int a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(unsigned short a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(short a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(unsigned char a, fixed const& b)
{
    return fixed(a)==b;
}
inline bool operator==(char a, fixed const& b)
{
    return fixed(a)==b;
}

inline bool operator==(fixed const& a,double b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,float b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,unsigned long b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,long b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,unsigned b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,int b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,unsigned short b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,short b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,unsigned char b)
{
    return a==fixed(b);
}
inline bool operator==(fixed const& a,char b)
{
    return a==fixed(b);
}

inline bool operator!=(double a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(float a, fixed const& b)
{
    return fixed(a)!=b;
}

inline bool operator!=(unsigned long a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(long a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(unsigned a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(int a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(unsigned short a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(short a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(unsigned char a, fixed const& b)
{
    return fixed(a)!=b;
}
inline bool operator!=(char a, fixed const& b)
{
    return fixed(a)!=b;
}

inline bool operator!=(fixed const& a,double b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,float b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,unsigned long b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,long b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,unsigned b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,int b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,unsigned short b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,short b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,unsigned char b)
{
    return a!=fixed(b);
}
inline bool operator!=(fixed const& a,char b)
{
    return a!=fixed(b);
}

inline bool operator<(double a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(float a, fixed const& b)
{
    return fixed(a)<b;
}

inline bool operator<(unsigned long a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(long a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(unsigned a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(int a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(unsigned short a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(short a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(unsigned char a, fixed const& b)
{
    return fixed(a)<b;
}
inline bool operator<(char a, fixed const& b)
{
    return fixed(a)<b;
}

inline bool operator<(fixed const& a,double b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,float b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,unsigned long b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,long b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,unsigned b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,int b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,unsigned short b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,short b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,unsigned char b)
{
    return a<fixed(b);
}
inline bool operator<(fixed const& a,char b)
{
    return a<fixed(b);
}

inline bool operator>(double a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(float a, fixed const& b)
{
    return fixed(a)>b;
}

inline bool operator>(unsigned long a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(long a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(unsigned a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(int a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(unsigned short a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(short a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(unsigned char a, fixed const& b)
{
    return fixed(a)>b;
}
inline bool operator>(char a, fixed const& b)
{
    return fixed(a)>b;
}

inline bool operator>(fixed const& a,double b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,float b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,unsigned long b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,long b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,unsigned b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,int b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,unsigned short b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,short b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,unsigned char b)
{
    return a>fixed(b);
}
inline bool operator>(fixed const& a,char b)
{
    return a>fixed(b);
}

inline bool operator<=(double a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(float a, fixed const& b)
{
    return fixed(a)<=b;
}

inline bool operator<=(unsigned long a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(long a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(unsigned a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(int a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(unsigned short a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(short a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(unsigned char a, fixed const& b)
{
    return fixed(a)<=b;
}
inline bool operator<=(char a, fixed const& b)
{
    return fixed(a)<=b;
}

inline bool operator<=(fixed const& a,double b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,float b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,unsigned long b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,long b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,unsigned b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,int b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,unsigned short b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,short b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,unsigned char b)
{
    return a<=fixed(b);
}
inline bool operator<=(fixed const& a,char b)
{
    return a<=fixed(b);
}

inline bool operator>=(double a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(float a, fixed const& b)
{
    return fixed(a)>=b;
}

inline bool operator>=(unsigned long a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(long a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(unsigned a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(int a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(unsigned short a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(short a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(unsigned char a, fixed const& b)
{
    return fixed(a)>=b;
}
inline bool operator>=(char a, fixed const& b)
{
    return fixed(a)>=b;
}

inline bool operator>=(fixed const& a,double b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,float b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,unsigned long b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,long b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,unsigned b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,int b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,unsigned short b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,short b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,unsigned char b)
{
    return a>=fixed(b);
}
inline bool operator>=(fixed const& a,char b)
{
    return a>=fixed(b);
}

inline fixed sin(fixed const& x)
{
    return x.sin();
}
inline fixed cos(fixed const& x)
{
    return x.cos();
}
inline fixed tan(fixed const& x)
{
    return x.tan();
}

inline fixed atan(fixed const& x)
{
    return x.atan();
}

inline fixed max(fixed const& x, fixed const& y)
{
  return (x>y? x:y);
}

inline fixed min(fixed const& x, fixed const& y)
{
  return (x<y? x:y);
}

inline fixed atan2(fixed const& y, fixed const& x)
{
  return fixed::atan2(y,x);
}

inline fixed sqrt(fixed const& x)
{
    return x.sqrt();
}

inline fixed exp(fixed const& x)
{
    return x.exp();
}

inline fixed log(fixed const& x)
{
    return x.log();
}

inline fixed floor(fixed const& x)
{
    return x.floor();
}

inline fixed ceil(fixed const& x)
{
    return x.ceil();
}

inline fixed abs(fixed const& x)
{
    return x.abs();
}

inline fixed fabs(fixed const& x)
{
    return x.abs();
}

inline fixed modf(fixed const& x,fixed*integral_part)
{
    return x.modf(integral_part);
}

inline fixed fixed::ceil() const
{
    if(m_nVal%fixed_resolution)
    {
        return floor()+1;
    }
    else
    {
        return *this;
    }
}

inline fixed fixed::floor() const
{
    fixed res(*this);
    __int64 const remainder=m_nVal%fixed_resolution;
    if(remainder)
    {
        res.m_nVal-=remainder;
        if(m_nVal<0)
        {
            res-=1;
        }
    }
    return res;
}


inline fixed fixed::sin() const
{
    fixed res;
    sin_cos(*this,&res,0);
    return res;
}

inline fixed fixed::cos() const
{
    fixed res;
    sin_cos(*this,0,&res);
    return res;
}

inline fixed fixed::tan() const
{
    fixed s,c;
    sin_cos(*this,&s,&c);
    return s/c;
}

inline fixed fixed::operator-() const
{
    return fixed(internal(),-m_nVal);
}

inline fixed fixed::abs() const
{
    return fixed(internal(),m_nVal<0?-m_nVal:m_nVal);
}

inline fixed fixed::modf(fixed*integral_part) const
{
    __int64 fractional_part=m_nVal%fixed_resolution;
    if(m_nVal<0 && fractional_part>0)
    {
        fractional_part-=fixed_resolution;
    }
    integral_part->m_nVal=m_nVal-fractional_part;
    return fixed(internal(),fractional_part);
}

inline void sin_cos(fixed const& theta,fixed* s,fixed*c)
{
  ::fixed::sin_cos(theta, s, c);
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
        ::fixed s,c;
        ::fixed::sin_cos(theta,&s,&c);
        return complex< ::fixed>(rho * c, rho * s);
    }
}

fixed const fixed_max(fixed::internal(),0x7fffffffffffffffLL);
fixed const fixed_one(fixed::internal(),1<<(fixed_resolution_shift));
fixed const fixed_two(fixed::internal(),1<<(fixed_resolution_shift+1));
fixed const fixed_four(fixed::internal(),1<<(fixed_resolution_shift+2));
fixed const fixed_zero(fixed::internal(),0);
fixed const fixed_half(fixed::internal(),1<<(fixed_resolution_shift-1));
extern fixed const fixed_pi;
extern fixed const fixed_two_pi;
extern fixed const fixed_half_pi;
extern fixed const fixed_quarter_pi;
extern fixed const fixed_deg_to_rad;
extern fixed const fixed_rad_to_deg;
extern fixed const fixed_360;
extern fixed const fixed_180;

inline bool positive(const fixed&f) {
  return f.positive();
};

inline bool negative(const fixed&f) {
  return f.negative();
};

#endif

inline void limit_tolerance(fixed& f, const fixed tol_act) {
  if (fabs(f)<tol_act) {
    f = positive(f)? tol_act:-tol_act;
  }
}

#endif
