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
#ifndef _SVX_WGHTITEM_HXX
#define _SVX_WGHTITEM_HXX

// include ---------------------------------------------------------------

#include <vcl/vclenum.hxx>
#include <svtools/eitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

class SvXMLUnitConverter;
namespace rtl
{
	class OUString;
}

// class SvxWeightItem ---------------------------------------------------

/* [Beschreibung]

	Dieses Item beschreibt die Font-Staerke.
*/

class SVX_DLLPUBLIC SvxWeightItem : public SfxEnumItem
{
public:
	TYPEINFO();

    SvxWeightItem(  const FontWeight eWght /*= WEIGHT_NORMAL*/,
                    const USHORT nId  );

	// "pure virtual Methoden" vom SfxPoolItem + SfxEnumItem
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	Create(SvStream &, USHORT) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion) const;
	virtual String			GetValueTextByPos( USHORT nPos ) const;
	virtual USHORT			GetValueCount() const;

	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	virtual int 			HasBoolValue() const;
	virtual BOOL			GetBoolValue() const;
	virtual void			SetBoolValue( BOOL bVal );

	inline SvxWeightItem& operator=(const SvxWeightItem& rWeight) {
			SetValue( rWeight.GetValue() );
			return *this;
		}

	// enum cast
	FontWeight				GetWeight() const
								{ return (FontWeight)GetValue(); }
	void					SetWeight( FontWeight eNew )
								{ SetValue( (USHORT)eNew ); }
};

#endif // #ifndef _SVX_WGHTITEM_HXX

