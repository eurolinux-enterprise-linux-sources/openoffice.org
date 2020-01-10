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
#ifndef _SVX_UDLNITEM_HXX
#define _SVX_UDLNITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/eitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include <vcl/vclenum.hxx>
#include <tools/color.hxx>
#include "svx/svxdllapi.h"

class SvXMLUnitConverter;
namespace rtl
{
	class OUString;
}

// class SvxTextLineItem ------------------------------------------------

/* Value container for underline and overline font effects */

class SVX_DLLPUBLIC SvxTextLineItem : public SfxEnumItem
{
	Color mColor;
public:
	TYPEINFO();

    SvxTextLineItem( const FontUnderline eSt,
                     const USHORT nId );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
							SfxMapUnit eCoreMetric,
							SfxMapUnit ePresMetric,
                            String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create(SvStream &, USHORT) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion) const;
	virtual String			GetValueTextByPos( USHORT nPos ) const;
	virtual USHORT          GetValueCount() const;

	virtual	sal_Bool        QueryValue( com::sun::star::uno::Any& rVal,
										BYTE nMemberId = 0 ) const;
	virtual	sal_Bool		PutValue( const com::sun::star::uno::Any& rVal,
										BYTE nMemberId = 0 );

	// MS VC4.0 kommt durcheinander
	void			 		SetValue( USHORT nNewVal )
								{SfxEnumItem::SetValue(nNewVal); }
	virtual int 			HasBoolValue() const;
	virtual BOOL			GetBoolValue() const;
	virtual void			SetBoolValue( BOOL bVal );

	virtual int 			operator==( const SfxPoolItem& ) const;

	inline SvxTextLineItem& operator=(const SvxTextLineItem& rTextLine)
		{
			SetValue( rTextLine.GetValue() );
			SetColor( rTextLine.GetColor() );
			return *this;
		}

	// enum cast
	FontUnderline 			GetLineStyle() const
								{ return (FontUnderline)GetValue(); }
	void 					SetLineStyle( FontUnderline eNew )
								{ SetValue((USHORT) eNew); }

	const Color&			GetColor() const				{ return mColor; }
	void 					SetColor( const Color& rCol )	{ mColor = rCol; }
};

// class SvxUnderlineItem ------------------------------------------------

/* Value container for underline font effects */

class SVX_DLLPUBLIC SvxUnderlineItem : public SvxTextLineItem
{
public:
	TYPEINFO();

    SvxUnderlineItem( const FontUnderline eSt,
                      const USHORT nId );

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create(SvStream &, USHORT) const;
	virtual String			GetValueTextByPos( USHORT nPos ) const;
};

// class SvxOverlineItem ------------------------------------------------

/* Value container for overline font effects */

class SVX_DLLPUBLIC SvxOverlineItem : public SvxTextLineItem
{
public:
	TYPEINFO();

    SvxOverlineItem( const FontUnderline eSt,
                     const USHORT nId );

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*    Create(SvStream &, USHORT) const;
	virtual String			GetValueTextByPos( USHORT nPos ) const;
};

#endif // #ifndef _SVX_UDLNITEM_HXX
