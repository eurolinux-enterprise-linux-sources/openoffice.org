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
#ifndef _SVX_VIEWLAYOUTITEM_HXX
#define _SVX_VIEWLAYOUTITEM_HXX

#include <svtools/intitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

//-------------------------------------------------------------------------

class SVX_DLLPUBLIC SvxViewLayoutItem: public SfxUInt16Item
{
    bool                    mbBookMode;

public:
	TYPEINFO();

    SvxViewLayoutItem( USHORT nColumns = 0, bool bBookMode = false, USHORT nWhich = SID_ATTR_VIEWLAYOUT );
    SvxViewLayoutItem( const SvxViewLayoutItem& );
    ~SvxViewLayoutItem();

    void                    SetBookMode( bool bNew ) {mbBookMode = bNew; }
    bool                    IsBookMode() const {return mbBookMode; }

    // "pure virtual Methoden" vom SfxPoolItem
    virtual int             operator==( const SfxPoolItem& ) const;
    virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;
    virtual SfxPoolItem*    Create( SvStream& rStrm, USHORT nVersion ) const;                       // leer
    virtual SvStream&       Store( SvStream& rStrm , USHORT nItemVersion ) const;                   // leer
    virtual sal_Bool        QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const; // leer
    virtual sal_Bool        PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );   // leer
};

//------------------------------------------------------------------------

#endif
