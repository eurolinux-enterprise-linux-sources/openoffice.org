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
#ifndef _SVX_ORPHITEM_HXX
#define _SVX_ORPHITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/intitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif

class SvXMLUnitConverter;
namespace rtl
{
	class OUString;
}
#include "svx/svxdllapi.h"

// class SvxOrphansItem --------------------------------------------------

/*
[Beschreibung]
Dieses Item beschreibt die Anzahl der Zeilen fuer die Schusterjungenregelung.
*/

class SVX_DLLPUBLIC SvxOrphansItem: public SfxByteItem
{
	friend SvStream & operator<<( SvStream & aS, SvxOrphansItem & );
public:
	TYPEINFO();

    SvxOrphansItem( const BYTE nL /*= 0*/, const USHORT nId  );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create(SvStream &, USHORT) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion ) const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	inline SvxOrphansItem& operator=( const SvxOrphansItem& rOrphans )
	{
		SetValue( rOrphans.GetValue() );
		return *this;
	}
};

#endif

