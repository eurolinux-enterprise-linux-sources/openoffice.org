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
#ifndef _SVX_KERNITEM_HXX
#define _SVX_KERNITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/intitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

class SvXMLUnitConverter;
namespace rtl
{
	class OUString;
}

// class SvxKerningItem --------------------------------------------------

// Achtung: Twips-Werte
// Twips: 0 = kein Kerning

/*	[Beschreibung]

	Dieses Item beschreibt die Schrift-Laufweite.
*/

class SVX_DLLPUBLIC SvxKerningItem : public SfxInt16Item
{
public:
	TYPEINFO();

    SvxKerningItem( const short nKern /*= 0*/, const USHORT nId  );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create(SvStream &, USHORT) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion) const;
	virtual int				ScaleMetrics( long nMult, long nDiv );
	virtual	int				HasMetrics() const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	inline SvxKerningItem& operator=(const SvxKerningItem& rKern) {
			SetValue( rKern.GetValue() );
			return *this;
		}

	virtual	BOOL        	QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	BOOL			PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );
};

#endif

