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
#ifndef _SVX_PMDLITEM_HXX
#define _SVX_PMDLITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/stritem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

// class SvxPageModelItem ------------------------------------------------

/*
[Beschreibung]
Dieses Item enthaelt einen Namen einer Seitenvorlage.
*/

class SVX_DLLPUBLIC SvxPageModelItem : public SfxStringItem
{
private:
	BOOL bAuto;

public:
	TYPEINFO();

    inline SvxPageModelItem( USHORT nWh  );
    inline SvxPageModelItem( const String& rModel, BOOL bA /*= FALSE*/,
                             USHORT nWh  );
	inline SvxPageModelItem& operator=( const SvxPageModelItem& rModel );

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );
	BOOL IsAuto() const { return bAuto; }
};

inline SvxPageModelItem::SvxPageModelItem( USHORT nWh )
	: bAuto( FALSE )
{
    SetWhich( nWh );
}

inline SvxPageModelItem::SvxPageModelItem( const String& rModel, BOOL bA,
                                           USHORT nWh ) :
    SfxStringItem( nWh, rModel ),
	bAuto( bA )
{}

inline SvxPageModelItem& SvxPageModelItem::operator=( const SvxPageModelItem& rModel )
{
	SetValue( rModel.GetValue() );
	return *this;
}

#endif

