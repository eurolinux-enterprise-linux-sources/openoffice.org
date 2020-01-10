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
#ifndef _SVX_NUMINF_HXX
#define _SVX_NUMINF_HXX

// include ---------------------------------------------------------------

#include <svtools/itempool.hxx>
#include <svx/numfmtsh.hxx>
#include "svx/svxdllapi.h"

// class SvxNumberInfoItem -----------------------------------------------



/*
[Beschreibung]
Dieses Item dient als Transport-Medium fuer einen Number-Formatter.
*/

class SVX_DLLPUBLIC SvxNumberInfoItem : public SfxPoolItem
{
public:
	TYPEINFO();

    SvxNumberInfoItem( const USHORT nId  );
	SvxNumberInfoItem( SvNumberFormatter* pNumFormatter,
                       const USHORT nId  );
	SvxNumberInfoItem( SvNumberFormatter* pNumFormatter, const String& rVal,
                       const USHORT nId  );
	SvxNumberInfoItem( SvNumberFormatter* pNumFormatter, const double& rVal,
                       const USHORT nId  );
	// if both double and String are supplied, String is used for text formats
	SvxNumberInfoItem( SvNumberFormatter* pNumFormatter, const double& rVal,
					   const String& rValueStr, const USHORT nId );
	SvxNumberInfoItem( const SvxNumberInfoItem& );
	~SvxNumberInfoItem();

	virtual int              operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*     Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*     Create( SvStream& rStream, USHORT nVer ) const;
	virtual SvStream&		 Store( SvStream& , USHORT nItemVersion ) const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	SvNumberFormatter*		GetNumberFormatter() const { return pFormatter; }
	void					SetNumberFormatter( SvNumberFormatter* pNumFrmt );
	const String&			GetValueString() const { return aStringVal; }
	void					SetStringValue( const String& rNewVal );
	double					GetValueDouble() const 	{ return nDoubleVal; }
	void					SetDoubleValue( const double& rNewVal );

	const sal_uInt32*		GetDelArray() const { return pDelFormatArr; }
	void					SetDelFormatArray( const sal_uInt32* pData,
											   const sal_uInt32  nCount );

	SvxNumberValueType		GetValueType() const { return eValueType; }
	sal_uInt32				GetDelCount() const  { return nDelCount; }

private:
	SvNumberFormatter*	pFormatter;
	SvxNumberValueType	eValueType;
	String				aStringVal;
	double				nDoubleVal;

	sal_uInt32*			pDelFormatArr;
	sal_uInt32			nDelCount;
};



#endif

