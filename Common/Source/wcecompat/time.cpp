/*  wcecompat: Windows CE C Runtime Library "compatibility" library.
 *
 *  Copyright (C) 2001-2002 Essemer Pty Ltd.  All rights reserved.
 *  http://www.essemer.com.au/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <errno.h>
#include "internal.h"


// Compare two 64-bit values.
// Returns:
//   <0 if a is less than b
//   0  if a is equal to b
//   >0 if a is greater than b
static int cmp64(const ULARGE_INTEGER* a, const ULARGE_INTEGER* b)
{
	if (a == NULL || b == NULL)
		return -1;	// error but no error return value

	if (a->HighPart < b->HighPart)
		return -1;
	else if (a->HighPart == b->HighPart)
	{
		if (a->LowPart < b->LowPart)
			return -1;
		else if (a->LowPart == b->LowPart)
			return 0;
		else	// if (a->LowPart > b->LowPart)
			return 1;
	}
	else	// if (a->HighPart < b->HighPart)
		return 1;
}


// Returns position of top non-zero bit,
// eg. -1 = none set, 0 = first bit, 31 = top bit
static int topBit(DWORD value)
{
	if (value == 0)
		return -1;
	else if (value & 0xffff0000)
	{	// bit in 0xffff0000 is set
		if (value & 0xff000000)
		{	// bit in 0xff000000 is set
			if (value & 0xf0000000)
			{	// bit in 0xf0000000 is set
				if (value & 0xc0000000)
				{	// bit in 0xc0000000 is set
					if (value & 0x80000000)
						return 31;
					else
						return 30;
				}
				else
				{	// bit in 0x30000000 is set
					if (value & 0x20000000)
						return 29;
					else
						return 28;
				}
			}
			else
			{	// bit in 0x0f000000 is set
				if (value & 0x0c000000)
				{	// bit in 0x0c000000 is set
					if (value & 0x08000000)
						return 27;
					else
						return 26;
				}
				else
				{	// bit in 0x03000000 is set
					if (value & 0x02000000)
						return 25;
					else
						return 24;
				}
			}
		}
		else
		{	// bit in 0x00ff0000 is set
			if (value & 0x00f00000)
			{	// bit in 0x00f00000 is set
				if (value & 0x00c00000)
				{	// bit in 0x00c00000 is set
					if (value & 0x00800000)
						return 23;
					else
						return 22;
				}
				else
				{	// bit in 0x00300000 is set
					if (value & 0x00200000)
						return 21;
					else
						return 20;
				}
			}
			else
			{	// bit in 0x000f0000 is set
				if (value & 0x000c0000)
				{	// bit in 0x000c0000 is set
					if (value & 0x00080000)
						return 19;
					else
						return 18;
				}
				else
				{	// bit in 0x00030000 is set
					if (value & 0x00020000)
						return 17;
					else
						return 16;
				}
			}
		}
	}
	else
	{	// bit in 0x0000ffff is set
		if (value & 0x0000ff00)
		{	// bit in 0x0000ff00 is set
			if (value & 0x0000f000)
			{	// bit in 0x0000f000 is set
				if (value & 0x0000c000)
				{	// bit in 0x0000c000 is set
					if (value & 0x00008000)
						return 15;
					else
						return 14;
				}
				else
				{	// bit in 0x00003000 is set
					if (value & 0x00002000)
						return 13;
					else
						return 12;
				}
			}
			else
			{	// bit in 0x00000f00 is set
				if (value & 0x00000c00)
				{	// bit in 0x00000c00 is set
					if (value & 0x00000800)
						return 11;
					else
						return 10;
				}
				else
				{	// bit in 0x00000300 is set
					if (value & 0x00000200)
						return 9;
					else
						return 8;
				}
			}
		}
		else
		{	// bit in 0x000000ff is set
			if (value & 0x000000f0)
			{	// bit in 0x000000f0 is set
				if (value & 0x000000c0)
				{	// bit in 0x000000c0 is set
					if (value & 0x00000080)
						return 7;
					else
						return 6;
				}
				else
				{	// bit in 0x00000030 is set
					if (value & 0x00000020)
						return 5;
					else
						return 4;
				}
			}
			else
			{	// bit in 0x0000000f is set
				if (value & 0x0000000c)
				{	// bit in 0x0000000c is set
					if (value & 0x00000008)
						return 3;
					else
						return 2;
				}
				else
				{	// bit in 0x00000003 is set
					if (value & 0x00000002)
						return 1;
					else
						return 0;
				}
			}
		}
	}
}


// Returns position of top non-zero bit,
// eg. -1 = none set, 0 = first bit, 63 = top bit
static int topBit64(ULARGE_INTEGER* value)
{
	int		result;

	if (value == NULL)
		return 0;

	result = topBit(value->HighPart);
	if (result != -1)
		return result+32;
	else
		return topBit(value->LowPart);
}


static void shl64(ULARGE_INTEGER* value, int shift)
{
	ULARGE_INTEGER	result;

	if (value == NULL || shift <= 0)
		return;

	result.HighPart = (value->HighPart << shift) | (value->LowPart >> (32-shift));
	result.LowPart = (value->LowPart << shift);

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


static void shr64(ULARGE_INTEGER* value, int shift)
{
	ULARGE_INTEGER	result;

	if (value == NULL || shift <= 0)
		return;

	result.HighPart = (value->HighPart >> shift);
	result.LowPart = (value->LowPart >> shift) | (value->HighPart << (32-shift));

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


// Add valueToAdd to value (doesn't handle overflow)
static void add64(ULARGE_INTEGER* value, ULARGE_INTEGER* valueToAdd)
{
	if (value == NULL || valueToAdd == NULL)
		return;

	value->LowPart += valueToAdd->LowPart;
	if (value->LowPart < valueToAdd->LowPart)
		value->HighPart++;	// carry to HighPart
	value->HighPart += valueToAdd->HighPart;
}


// Subtract valueToSubtract from value (doesn't handle underflow)
static void sub64(ULARGE_INTEGER* value, ULARGE_INTEGER* valueToSubtract)
{
	if (value == NULL || valueToSubtract == NULL)
		return;

	if (value->LowPart < valueToSubtract->LowPart)
		value->HighPart--;	// borrow from HighPart
	value->HighPart -= valueToSubtract->HighPart;
	value->LowPart -= valueToSubtract->LowPart;
}


static void mul64(ULARGE_INTEGER* value, DWORD multiplier)
{
	ULARGE_INTEGER	result = { 0, 0 };
	ULARGE_INTEGER	temp;
	WORD			multiplierHigh = (WORD)(multiplier >> 16);
	WORD			multiplierLow = (WORD)(multiplier & 0xffff);

	if (value == NULL)
		return;

	//
	// split the DWORD in two, and multiply seperately to avoid overflow
	//

	// firstly, the lower 16-bits of the multiplier

	temp.HighPart = 0;
	temp.LowPart = (value->LowPart & 0xffff) * multiplierLow;
	add64(&result, &temp);

	temp.LowPart = (value->LowPart >> 16) * multiplierLow;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.LowPart = 0;
	temp.HighPart = (value->HighPart & 0xffff) * multiplierLow;
	add64(&result, &temp);

	temp.HighPart = (value->HighPart >> 16) * multiplierLow;
	shl64(&temp, 16);
	add64(&result, &temp);

	// secondly, the higher 16-bits of the multiplier

	temp.HighPart = 0;
	temp.LowPart = (value->LowPart & 0xffff) * multiplierHigh;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.LowPart = (value->LowPart >> 16) * multiplierHigh;
	shl64(&temp, 32);
	add64(&result, &temp);

	temp.LowPart = 0;
	temp.HighPart = (value->HighPart & 0xffff) * multiplierHigh;
	shl64(&temp, 16);
	add64(&result, &temp);

	temp.HighPart = (value->HighPart >> 16) * multiplierHigh;
	shl64(&temp, 32);
	add64(&result, &temp);

	// return the result
	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


static void div64(ULARGE_INTEGER* value, DWORD divisor)
{
	ULARGE_INTEGER	result = { 0, 0 };
	ULARGE_INTEGER	shiftedDivisor;	// divisor shifted to left
	ULARGE_INTEGER	shiftedOne;		// '1' shifted to left by same number of bits as divisor
	int				shift;

	if (value == NULL)
		return;
	if (divisor == 0)
	{
		value->LowPart = 0;
		value->HighPart = 0;
		return;
	}
	if (value->HighPart == 0)
	{
		if (value->LowPart != 0)
			value->LowPart /= divisor;
		return;
	}

	// shift divisor up (into shifted) as far as it can go before it is greater than value
	shift = topBit64(value) - topBit(divisor);
	shiftedDivisor.LowPart = divisor;
	shiftedDivisor.HighPart = 0;
	shiftedOne.LowPart = 1;
	shiftedOne.HighPart = 0;
	shl64(&shiftedDivisor, shift);
	shl64(&shiftedOne, shift);
	while (shift >= 0)
	{
		if (cmp64(&shiftedDivisor, value) <= 0)
		{
			add64(&result, &shiftedOne);
			sub64(value, &shiftedDivisor);
		}
		shr64(&shiftedDivisor, 1);
		shr64(&shiftedOne, 1);
		shift--;
	}

	value->HighPart = result.HighPart;
	value->LowPart = result.LowPart;
}


static const FILETIME	ftJan1970 = {3577643008,27111902};


// Convert Win32 FILETIME into time_t
time_t w32_filetime_to_time_t(FILETIME* ft)
{
	// make sure ft is at least ftJan1970
	if (cmp64((ULARGE_INTEGER*)ft, (ULARGE_INTEGER*)&ftJan1970) < 0)
	{
		errno = -1;
		return -1;
	}

	// subtract ftJan1970 from ft
	sub64((ULARGE_INTEGER*)ft, (ULARGE_INTEGER*)&ftJan1970);

	// divide ft by 10,000,000 to convert from 100-nanosecond units to seconds
	div64((ULARGE_INTEGER*)ft, 10000000);

	// bound check result
	if (ft->dwHighDateTime != 0 || ft->dwLowDateTime >= 2147483648)
	{
		errno = -1;
		return -1;		// value is too big to return in time_t
	}

	return (time_t)ft->dwLowDateTime;
}


time_t time(time_t* t)
{
	SYSTEMTIME		stNow;
	FILETIME		ftNow;
	time_t			tt;

	// get system time
	GetSystemTime(&stNow);
	if (!SystemTimeToFileTime(&stNow, &ftNow))
	{
		errno = -1;
		return -1;
	}

	tt = w32_filetime_to_time_t(&ftNow);

	if (t != NULL)
		*t = tt;
	return tt;
}


clock_t clock(void)
{
	return (clock_t)GetTickCount();
}


// Returns to day of the year (0..365)
// Year: eg. 2001
// Month: 0.11
// Day: 1..31
int dayOfYear(int year, int month, int day)
{
	int		result;
	BOOL	isLeapYear = FALSE;

	if ((year % 4 == 0) && !(year % 100 == 0) && (year % 400 == 0))
		isLeapYear = TRUE;

	result = day-1;
	if (month > 0)
		result += 31;	// Jan
	if (month > 1)
		result += 28 + (isLeapYear ? 1 : 0);
	if (month > 2)
		result += 31;	// Mar
	if (month > 3)
		result += 30;	// Apr
	if (month > 4)
		result += 31;	// May
	if (month > 5)
		result += 30;	// Jun
	if (month > 6)
		result += 31;	// Jul
	if (month > 7)
		result += 31;	// Aug
	if (month > 8)
		result += 30;	// Sep
	if (month > 9)
		result += 31;	// Oct
	if (month > 10)
		result += 30;	// Nov

	return result;
}


static struct tm st_tm;


struct tm* localtime(const time_t* clock)
{
	FILETIME				ftUtc;
	FILETIME				ftLocal;
	SYSTEMTIME				stLocal;
	TIME_ZONE_INFORMATION	tzi;
	DWORD					tziResult;

	if (clock == NULL)
		return NULL;

	// convert time_t to FILETIME
	ftUtc.dwHighDateTime = 0;
	ftUtc.dwLowDateTime = *clock;
	mul64((ULARGE_INTEGER*)&ftUtc, 10000000);
	add64((ULARGE_INTEGER*)&ftUtc, (ULARGE_INTEGER*)&ftJan1970);

	// convert to local FILETIME
	if (!LocalFileTimeToFileTime(&ftUtc, &ftLocal))
		return NULL;

	// convert to SYSTEMTIME
	if (!FileTimeToSystemTime(&ftLocal, &stLocal))
		return NULL;

	// determine if we're operating in daylight savings time
	tziResult = GetTimeZoneInformation(&tzi);

	// fill return structure
	st_tm.tm_sec = stLocal.wSecond;
	st_tm.tm_min = stLocal.wMinute;
	st_tm.tm_hour = stLocal.wHour;
	st_tm.tm_mday = stLocal.wDay;
	st_tm.tm_mon = stLocal.wMonth-1;
	st_tm.tm_year = stLocal.wYear-1900;
	st_tm.tm_wday = stLocal.wDayOfWeek;
	st_tm.tm_yday = dayOfYear(stLocal.wYear, stLocal.wMonth-1, stLocal.wDay);
	if (tziResult == TIME_ZONE_ID_UNKNOWN)
		st_tm.tm_isdst = -1;
	else if (tziResult == TIME_ZONE_ID_STANDARD)
		st_tm.tm_isdst = 0;
	else if (tziResult == TIME_ZONE_ID_DAYLIGHT)
		st_tm.tm_isdst = 1;

	return &st_tm;
}


struct tm* gmtime(const time_t* clock)
{
	FILETIME	ftLocal;
	FILETIME	ftUtc;
	SYSTEMTIME	stUtc;

	if (clock == NULL)
		return NULL;

	// convert time_t to FILETIME
	ftLocal.dwHighDateTime = 0;
	ftLocal.dwLowDateTime = *clock;
	mul64((ULARGE_INTEGER*)&ftLocal, 10000000);
	add64((ULARGE_INTEGER*)&ftLocal, (ULARGE_INTEGER*)&ftJan1970);

	// convert to UTC FILETIME
	if (!LocalFileTimeToFileTime(&ftLocal, &ftUtc))
		return NULL;

	// convert to SYSTEMTIME
	if (!FileTimeToSystemTime(&ftUtc, &stUtc))
		return NULL;

	// fill return structure
	st_tm.tm_sec = stUtc.wSecond;
	st_tm.tm_min = stUtc.wMinute;
	st_tm.tm_hour = stUtc.wHour;
	st_tm.tm_mday = stUtc.wDay;
	st_tm.tm_mon = stUtc.wMonth-1;
	st_tm.tm_year = stUtc.wYear-1900;
	st_tm.tm_wday = stUtc.wDayOfWeek;
	st_tm.tm_yday = dayOfYear(stUtc.wYear, stUtc.wMonth-1, stUtc.wDay);
	st_tm.tm_isdst = 0;

	return &st_tm;
}
