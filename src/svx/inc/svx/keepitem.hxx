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
#ifndef _SVX_KEEPITEM_HXX
#define _SVX_KEEPITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/eitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

namespace rtl
{
	class OUString;
}

// class SvxFmtKeepItem ------------------------------------------------

/*
[Beschreibung]
Dieses Item beschreibt eine logische Variable "Absaetze zusammenhalten?".
*/

class SVX_DLLPUBLIC SvxFmtKeepItem : public SfxBoolItem
{
public:
	TYPEINFO();

    inline SvxFmtKeepItem( const BOOL bKeep /*= FALSE*/,
                           const USHORT _nWhich  );
	inline SvxFmtKeepItem& operator=( const SvxFmtKeepItem& rSplit );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	Create( SvStream&, USHORT ) const;
	virtual SvStream&		Store( SvStream& , USHORT nItemVersion ) const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;
};

inline SvxFmtKeepItem::SvxFmtKeepItem( const BOOL bKeep, const USHORT _nWhich ) :
    SfxBoolItem( _nWhich, bKeep )
{}

inline SvxFmtKeepItem& SvxFmtKeepItem::operator=( const SvxFmtKeepItem& rKeep )
{
	SetValue( rKeep.GetValue() );
	return *this;
}

#endif

