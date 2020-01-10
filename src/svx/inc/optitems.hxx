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
#ifndef _SVX_OPTITEMS_HXX
#define _SVX_OPTITEMS_HXX

// include ---------------------------------------------------------------

#include <svtools/poolitem.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include "svx/svxdllapi.h"

// forward ---------------------------------------------------------------
namespace com{namespace sun{namespace star{
namespace beans{
//	class XPropertySet;
}
namespace linguistic2{
	class XSpellChecker1;
}}}}


// class SfxSpellCheckItem -----------------------------------------------



class SVX_DLLPUBLIC SfxSpellCheckItem: public SfxPoolItem
{
public:
	TYPEINFO();

	SfxSpellCheckItem( ::com::sun::star::uno::Reference<
							::com::sun::star::linguistic2::XSpellChecker1 >  &xChecker,
                       sal_uInt16 nWhich  );
	SfxSpellCheckItem( const SfxSpellCheckItem& rItem );

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
	virtual int 			operator==( const SfxPoolItem& ) const;

	::com::sun::star::uno::Reference<
		::com::sun::star::linguistic2::XSpellChecker1 >
			GetXSpellChecker() const { return xSpellCheck; }

private:
	::com::sun::star::uno::Reference<
		::com::sun::star::linguistic2::XSpellChecker1 > 		xSpellCheck;
};


// class SfxHyphenRegionItem ---------------------------------------------



class SVX_DLLPUBLIC SfxHyphenRegionItem: public SfxPoolItem
{
	sal_uInt8 nMinLead;
	sal_uInt8 nMinTrail;

public:
	TYPEINFO();

    SfxHyphenRegionItem( const sal_uInt16 nId  );
	SfxHyphenRegionItem( const SfxHyphenRegionItem& rItem );

	virtual int 			 operator==( const SfxPoolItem& ) const;
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*	 Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	 Create( SvStream& rStrm, sal_uInt16 nVer ) const;
	virtual SvStream&		 Store( SvStream& rStrm, sal_uInt16 ) const;

	inline sal_uInt8 &GetMinLead() { return nMinLead; }
	inline sal_uInt8 GetMinLead() const { return nMinLead; }

	inline sal_uInt8 &GetMinTrail() { return nMinTrail; }
	inline sal_uInt8 GetMinTrail() const { return nMinTrail; }

	inline SfxHyphenRegionItem& operator=( const SfxHyphenRegionItem& rNew )
	{
		nMinLead = rNew.GetMinLead();
		nMinTrail = rNew.GetMinTrail();
		return *this;
	}
};



#endif

