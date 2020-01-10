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
#ifndef _DATE_HXX
#define _DATE_HXX

#include "tools/toolsdllapi.h"
#include <tools/solar.h>

class ResId;

// --------------
// - Date-Types -
// --------------

enum DayOfWeek { MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY,
				 SATURDAY, SUNDAY };

// --------
// - Date -
// --------

class TOOLS_DLLPUBLIC Date
{
private:
	sal_uInt32		nDate;

public:
					Date();
					Date( const ResId & rResId );
					Date( sal_uInt32 _nDate ) { Date::nDate = _nDate; }
					Date( const Date& rDate )
						{ nDate = rDate.nDate; }
					Date( USHORT nDay, USHORT nMonth, USHORT nYear )
						{ nDate = (   sal_uInt32( nDay   % 100 ) ) +
								  ( ( sal_uInt32( nMonth % 100 ) ) * 100 ) +
								  ( ( sal_uInt32( nYear  % 10000 ) ) * 10000); }

	void			SetDate( sal_uInt32 nNewDate ) { nDate = nNewDate; }
	sal_uInt32		GetDate() const { return nDate; }

	void			SetDay( USHORT nNewDay );
	void			SetMonth( USHORT nNewMonth );
	void			SetYear( USHORT nNewYear );
	USHORT			GetDay() const { return (USHORT)(nDate % 100); }
	USHORT			GetMonth() const { return (USHORT)((nDate / 100) % 100); }
	USHORT			GetYear() const { return (USHORT)(nDate / 10000); }

	DayOfWeek		GetDayOfWeek() const;
	USHORT			GetDayOfYear() const;
    /** nMinimumNumberOfDaysInWeek: how many days of a week must reside in the
        first week of a year. */
	USHORT			GetWeekOfYear( DayOfWeek eStartDay = MONDAY,
								   sal_Int16 nMinimumNumberOfDaysInWeek = 4 ) const;

	USHORT			GetDaysInMonth() const;
	USHORT			GetDaysInYear() const { return (IsLeapYear()) ? 366 : 365; }
	BOOL			IsLeapYear() const;
	BOOL			IsValid() const;

	BOOL			IsBetween( const Date& rFrom, const Date& rTo ) const
						{ return ((nDate >= rFrom.nDate) &&
								 (nDate <= rTo.nDate)); }

	BOOL			operator ==( const Date& rDate ) const
						{ return (nDate == rDate.nDate); }
	BOOL			operator !=( const Date& rDate ) const
						{ return (nDate != rDate.nDate); }
	BOOL			operator  >( const Date& rDate ) const
						{ return (nDate > rDate.nDate); }
	BOOL			operator  <( const Date& rDate ) const
						{ return (nDate < rDate.nDate); }
	BOOL			operator >=( const Date& rDate ) const
						{ return (nDate >= rDate.nDate); }
	BOOL			operator <=( const Date& rDate ) const
						{ return (nDate <= rDate.nDate); }

	Date&			operator =( const Date& rDate )
						{ nDate = rDate.nDate; return *this; }
	Date&			operator +=( long nDays );
	Date&			operator -=( long nDays );
	Date&			operator ++();
	Date&			operator --();
#ifndef MPW33
	Date			operator ++( int );
	Date			operator --( int );
#endif

	TOOLS_DLLPUBLIC friend Date 	operator +( const Date& rDate, long nDays );
	TOOLS_DLLPUBLIC friend Date 	operator -( const Date& rDate, long nDays );
	TOOLS_DLLPUBLIC friend long 	operator -( const Date& rDate1, const Date& rDate2 );
};

#endif // _DATE_HXX
