/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_tools.hxx"

#define _TOOLS_TIME_CXX

#if defined( OS2 )
#define INCL_DOSMISC
#define INCL_DOSDATETIME
#include <svpm.h>
#elif defined( WNT )
#ifdef _MSC_VER
#pragma warning (push,1)
#endif
#include <tools/svwin.h>
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#elif defined UNX
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#endif

#include <time.h>
#include <tools/time.hxx>

#if defined(SOLARIS) && defined(__GNUC__)
extern long altzone;
#endif

// =======================================================================

static sal_Int32 TimeToSec100( const Time& rTime )
{
	short  nSign   = (rTime.GetTime() >= 0) ? +1 : -1;
	sal_Int32   nHour   = rTime.GetHour();
	sal_Int32   nMin    = rTime.GetMin();
	sal_Int32   nSec    = rTime.GetSec();
	sal_Int32   n100Sec = rTime.Get100Sec();

//	Wegen Interal Compiler Error bei MSC, etwas komplizierter
//	return (n100Sec + (nSec*100) + (nMin*60*100) + (nHour*60*60*100) * nSign);

	sal_Int32 nRet = n100Sec;
	nRet	 += nSec*100;
	nRet	 += nMin*60*100;
	nRet	 += nHour*60*60*100;

	return (nRet * nSign);
}

// -----------------------------------------------------------------------

static Time Sec100ToTime( sal_Int32 nSec100 )
{
	short nSign;
	if ( nSec100 < 0 )
	{
		nSec100 *= -1;
		nSign = -1;
	}
	else
		nSign = 1;

	Time aTime( 0, 0, 0, nSec100 );
	aTime.SetTime( aTime.GetTime() * nSign );
	return aTime;
}

// =======================================================================

Time::Time()
{
#if defined( OS2 )
	DATETIME aDateTime;
	DosGetDateTime( &aDateTime );

	// Zeit zusammenbauen
	nTime = (((sal_Int32)aDateTime.hours)*1000000) +
			(((sal_Int32)aDateTime.minutes)*10000) +
			(((sal_Int32)aDateTime.seconds)*100) +
			((sal_Int32)aDateTime.hundredths);
#elif defined( WNT )
	SYSTEMTIME aDateTime;
	GetLocalTime( &aDateTime );

	// Zeit zusammenbauen
	nTime = (((sal_Int32)aDateTime.wHour)*1000000) +
			(((sal_Int32)aDateTime.wMinute)*10000) +
			(((sal_Int32)aDateTime.wSecond)*100) +
			((sal_Int32)aDateTime.wMilliseconds/10);
#else
	time_t	   nTmpTime;
	struct tm aTime;

	// Zeit ermitteln
	nTmpTime = time( 0 );

	// Zeit zusammenbauen
	if ( localtime_r( &nTmpTime, &aTime ) )
	{
		nTime = (((sal_Int32)aTime.tm_hour)*1000000) +
				(((sal_Int32)aTime.tm_min)*10000) +
				(((sal_Int32)aTime.tm_sec)*100);
	}
	else
		nTime = 0;
#endif
}

// -----------------------------------------------------------------------

Time::Time( const Time& rTime )
{
	nTime = rTime.nTime;
}

// -----------------------------------------------------------------------

Time::Time( ULONG nHour, ULONG nMin, ULONG nSec, ULONG n100Sec )
{
	// Zeit normalisieren
	nSec	+= n100Sec / 100;
	n100Sec  = n100Sec % 100;
	nMin	+= nSec / 60;
	nSec	 = nSec % 60;
	nHour	+= nMin / 60;
	nMin	 = nMin % 60;

	// Zeit zusammenbauen
	nTime = (sal_Int32)(n100Sec + (nSec*100) + (nMin*10000) + (nHour*1000000));
}

// -----------------------------------------------------------------------

void Time::SetHour( USHORT nNewHour )
{
	short  nSign	  = (nTime >= 0) ? +1 : -1;
	sal_Int32   nMin 	  = GetMin();
	sal_Int32   nSec 	  = GetSec();
	sal_Int32   n100Sec	  = Get100Sec();

	nTime = (n100Sec + (nSec*100) + (nMin*10000) +
			(((sal_Int32)nNewHour)*1000000)) * nSign;
}

// -----------------------------------------------------------------------

void Time::SetMin( USHORT nNewMin )
{
	short  nSign	  = (nTime >= 0) ? +1 : -1;
	sal_Int32   nHour	  = GetHour();
	sal_Int32   nSec 	  = GetSec();
	sal_Int32   n100Sec	  = Get100Sec();

	// kein Ueberlauf
	nNewMin = nNewMin % 60;

	nTime = (n100Sec + (nSec*100) + (((sal_Int32)nNewMin)*10000) +
			(nHour*1000000)) * nSign;
}

// -----------------------------------------------------------------------

void Time::SetSec( USHORT nNewSec )
{
	short       nSign	  = (nTime >= 0) ? +1 : -1;
	sal_Int32   nHour	  = GetHour();
	sal_Int32   nMin 	  = GetMin();
	sal_Int32   n100Sec	  = Get100Sec();

	// kein Ueberlauf
	nNewSec = nNewSec % 60;

	nTime = (n100Sec + (((sal_Int32)nNewSec)*100) + (nMin*10000) +
			(nHour*1000000)) * nSign;
}

// -----------------------------------------------------------------------

void Time::Set100Sec( USHORT nNew100Sec )
{
	short       nSign	  = (nTime >= 0) ? +1 : -1;
	sal_Int32   nHour	  = GetHour();
	sal_Int32   nMin 	  = GetMin();
	sal_Int32   nSec 	  = GetSec();

	// kein Ueberlauf
	nNew100Sec = nNew100Sec % 100;

	nTime = (((sal_Int32)nNew100Sec) + (nSec*100) + (nMin*10000) +
			(nHour*1000000)) * nSign;
}

// -----------------------------------------------------------------------

sal_Int32 Time::GetMSFromTime() const
{
	short       nSign	  = (nTime >= 0) ? +1 : -1;
	sal_Int32   nHour	  = GetHour();
	sal_Int32   nMin 	  = GetMin();
	sal_Int32   nSec 	  = GetSec();
	sal_Int32   n100Sec	  = Get100Sec();

	return (((nHour*3600000)+(nMin*60000)+(nSec*1000)+(n100Sec*10))*nSign);
}

// -----------------------------------------------------------------------

void Time::MakeTimeFromMS( sal_Int32 nMS )
{
	short nSign;
	if ( nMS < 0 )
	{
		nMS *= -1;
		nSign = -1;
	}
	else
		nSign = 1;

	Time aTime( 0, 0, 0, nMS/10 );
	SetTime( aTime.GetTime() * nSign );
}

// -----------------------------------------------------------------------

double Time::GetTimeInDays() const
{
	short  nSign	  = (nTime >= 0) ? +1 : -1;
    double nHour      = GetHour();
    double nMin       = GetMin();
    double nSec       = GetSec();
    double n100Sec    = Get100Sec();

    return (nHour+(nMin/60)+(nSec/(60*60))+(n100Sec/(60*60*100))) / 24 * nSign;
}

// -----------------------------------------------------------------------

Time& Time::operator =( const Time& rTime )
{
	nTime = rTime.nTime;
	return *this;
}

// -----------------------------------------------------------------------

Time& Time::operator +=( const Time& rTime )
{
	nTime = Sec100ToTime( TimeToSec100( *this ) +
						  TimeToSec100( rTime ) ).GetTime();
	return *this;
}

// -----------------------------------------------------------------------

Time& Time::operator -=( const Time& rTime )
{
	nTime = Sec100ToTime( TimeToSec100( *this ) -
						  TimeToSec100( rTime ) ).GetTime();
	return *this;
}

// -----------------------------------------------------------------------

Time operator +( const Time& rTime1, const Time& rTime2 )
{
	return Sec100ToTime( TimeToSec100( rTime1 ) +
						 TimeToSec100( rTime2 ) );
}

// -----------------------------------------------------------------------

Time operator -( const Time& rTime1, const Time& rTime2 )
{
	return Sec100ToTime( TimeToSec100( rTime1 ) -
						 TimeToSec100( rTime2 ) );
}

// -----------------------------------------------------------------------

BOOL Time::IsEqualIgnore100Sec( const Time& rTime ) const
{
    sal_Int32 n1 = (nTime < 0 ? -Get100Sec() : Get100Sec() );
    sal_Int32 n2 = (rTime.nTime < 0 ? -rTime.Get100Sec() : rTime.Get100Sec() );
    return (nTime - n1) == (rTime.nTime - n2);
}

// -----------------------------------------------------------------------

Time Time::GetUTCOffset()
{
#if defined( OS2 )
#undef timezone
	DATETIME aDateTime;
	DosGetDateTime( &aDateTime );

	// Zeit zusammenbauen
	if ( aDateTime.timezone != -1  )
	{
		short nTempTime = (short)Abs( aDateTime.timezone );
		Time aTime( 0, (USHORT)nTempTime );
		if ( aDateTime.timezone > 0 )
			aTime = -aTime;
		return aTime;
	}
	else
		return Time( 0 );
#elif defined( WNT )
	TIME_ZONE_INFORMATION	aTimeZone;
	aTimeZone.Bias = 0;
	DWORD nTimeZoneRet = GetTimeZoneInformation( &aTimeZone );
	sal_Int32 nTempTime = aTimeZone.Bias;
	if ( nTimeZoneRet == TIME_ZONE_ID_STANDARD )
		nTempTime += aTimeZone.StandardBias;
	else if ( nTimeZoneRet == TIME_ZONE_ID_DAYLIGHT )
		nTempTime += aTimeZone.DaylightBias;
	Time aTime( 0, (USHORT)Abs( nTempTime ) );
	if ( nTempTime > 0 )
		aTime = -aTime;
	return aTime;
#else
	static ULONG	nCacheTicks = 0;
	static sal_Int32 	nCacheSecOffset = -1;
	ULONG			nTicks = Time::GetSystemTicks();
	time_t			nTime;
	tm 			    aTM;
	sal_Int32			nLocalTime;
	sal_Int32			nUTC;
	short			nTempTime;

	// Evt. Wert neu ermitteln
	if ( (nCacheSecOffset == -1)			||
		 ((nTicks - nCacheTicks) > 360000)	||
		 ( nTicks < nCacheTicks ) // handle overflow
		 )
	{
		nTime = time( 0 );
		localtime_r( &nTime, &aTM );
		nLocalTime = mktime( &aTM );
#if defined( SOLARIS )
		// Solaris gmtime_r() seems not to handle daylight saving time
		// flags correctly
		nUTC = nLocalTime + ( aTM.tm_isdst == 0 ? timezone : altzone );
#elif defined( LINUX )
		// Linux mktime() seems not to handle tm_isdst correctly
		nUTC = nLocalTime - aTM.tm_gmtoff;
#else
 		gmtime_r( &nTime, &aTM );
 		nUTC = mktime( &aTM );
#endif
 		nCacheTicks = nTicks;
 		nCacheSecOffset = (nLocalTime-nUTC) / 60;
	}

	nTempTime = (short)Abs( nCacheSecOffset );
	Time aTime( 0, (USHORT)nTempTime );
	if ( nCacheSecOffset < 0 )
		aTime = -aTime;
	return aTime;
#endif
}


// -----------------------------------------------------------------------

ULONG Time::GetSystemTicks()
{
#if defined WNT
	return (ULONG)GetTickCount();
#elif defined( OS2 )
	ULONG nClock;
	DosQuerySysInfo( QSV_MS_COUNT, QSV_MS_COUNT, &nClock, sizeof( nClock ) );
	return (ULONG)nClock;
#else
	timeval tv;
	gettimeofday (&tv, 0);

	double fTicks = tv.tv_sec;
	fTicks *= 1000;
	fTicks += ((tv.tv_usec + 500) / 1000);

	fTicks = fmod (fTicks, double(ULONG_MAX));
	return ULONG(fTicks);
#endif
}

// -----------------------------------------------------------------------

ULONG Time::GetProcessTicks()
{
#if defined WNT
	return (ULONG)GetTickCount();
#elif defined( OS2 )
	ULONG nClock;
	DosQuerySysInfo( QSV_MS_COUNT, QSV_MS_COUNT, &nClock, sizeof( nClock ) );
	return (ULONG)nClock;
#else
	static ULONG	nImplTicksPerSecond = 0;
	static double	dImplTicksPerSecond;
	static double	dImplTicksULONGMAX;
	ULONG			nTicks = (ULONG)clock();

	if ( !nImplTicksPerSecond )
	{
		nImplTicksPerSecond = CLOCKS_PER_SEC;
		dImplTicksPerSecond = nImplTicksPerSecond;
		dImplTicksULONGMAX	= (double)(ULONG)ULONG_MAX;
	}

	double fTicks = nTicks;
	fTicks *= 1000;
	fTicks /= dImplTicksPerSecond;
	fTicks = fmod (fTicks, dImplTicksULONGMAX);
	return (ULONG)fTicks;
#endif
}
