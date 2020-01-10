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
#ifndef _SVX_WRITINGMODEITEM_HXX
#define _SVX_WRITINGMODEITEM_HXX

// include ---------------------------------------------------------------

#include <com/sun/star/text/WritingMode.hpp>
#include <svtools/intitem.hxx>
#include <svx/svddef.hxx>
#include "svx/svxdllapi.h"

// class SvxWritingModeItem ----------------------------------------------

class SVX_DLLPUBLIC SvxWritingModeItem : public SfxUInt16Item
{
public:
	TYPEINFO();

    SvxWritingModeItem( ::com::sun::star::text::WritingMode eValue /*= com::sun::star::text::WritingMode_LR_TB*/,
                USHORT nWhich /*= SDRATTR_TEXTDIRECTION*/ );
	virtual ~SvxWritingModeItem();

	SvxWritingModeItem& operator=( const SvxWritingModeItem& rItem );

	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	Create(SvStream &, USHORT) const;
	virtual SvStream& 		Store(SvStream & rStrm, USHORT nIVer) const;
	virtual USHORT			GetVersion( USHORT nFileVersion ) const;
	virtual int 			operator==( const SfxPoolItem& ) const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
									String &rText,
                                    const IntlWrapper * = 0 ) const;

	virtual sal_Bool PutValue( const com::sun::star::uno::Any& rVal,
									BYTE nMemberId );
	virtual sal_Bool QueryValue( com::sun::star::uno::Any& rVal,
								BYTE nMemberId ) const;
};

#endif // #ifndef _SVX_WRITINGMODEITEM_HXX

