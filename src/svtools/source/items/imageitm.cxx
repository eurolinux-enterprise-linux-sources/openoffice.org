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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_svtools.hxx"

#include <svtools/imageitm.hxx>
#include <com/sun/star/uno/Sequence.hxx>

TYPEINIT1( SfxImageItem, SfxInt16Item );

struct SfxImageItem_Impl
{
    String  aURL;
    long    nAngle;
    BOOL    bMirrored;
    int     operator == ( const SfxImageItem_Impl& rOther ) const
            { return nAngle == rOther.nAngle && bMirrored == rOther.bMirrored; }
};

//---------------------------------------------------------

SfxImageItem::SfxImageItem( USHORT which, UINT16 nImage )
    : SfxInt16Item( which, nImage )
{
    pImp = new SfxImageItem_Impl;
    pImp->nAngle = 0;
    pImp->bMirrored = FALSE;
}

SfxImageItem::SfxImageItem( USHORT which, const String& rURL )
    : SfxInt16Item( which, 0 )
{
    pImp = new SfxImageItem_Impl;
    pImp->nAngle = 0;
    pImp->bMirrored = FALSE;
    pImp->aURL = rURL;
}

SfxImageItem::SfxImageItem( const SfxImageItem& rItem )
    : SfxInt16Item( rItem )
{
    pImp = new SfxImageItem_Impl( *(rItem.pImp) );
}

//---------------------------------------------------------
SfxImageItem::~SfxImageItem()
{
    delete pImp;
}

//---------------------------------------------------------

SfxPoolItem* SfxImageItem::Clone( SfxItemPool* ) const
{
    return new SfxImageItem( *this );
}

//---------------------------------------------------------

int SfxImageItem::operator==( const SfxPoolItem& rItem ) const
{
    return( ((SfxImageItem&) rItem).GetValue() == GetValue() && (*pImp == *(((SfxImageItem&)rItem).pImp) ) );
}

BOOL SfxImageItem::QueryValue( com::sun::star::uno::Any& rVal, BYTE ) const
{
    ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any > aSeq( 4 );
    aSeq[0] = ::com::sun::star::uno::makeAny( GetValue() );
    aSeq[1] = ::com::sun::star::uno::makeAny( pImp->nAngle );
    aSeq[2] = ::com::sun::star::uno::makeAny( pImp->bMirrored );
    aSeq[3] = ::com::sun::star::uno::makeAny( rtl::OUString( pImp->aURL ));

    rVal = ::com::sun::star::uno::makeAny( aSeq );
    return TRUE;
}

BOOL SfxImageItem::PutValue( const com::sun::star::uno::Any& rVal, BYTE )
{
    ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any > aSeq;
    if (( rVal >>= aSeq ) && ( aSeq.getLength() == 4 ))
    {
        sal_Int16     nVal = sal_Int16();
        rtl::OUString aURL;  
        if ( aSeq[0] >>= nVal )
            SetValue( nVal );
        aSeq[1] >>= pImp->nAngle;
        aSeq[2] >>= pImp->bMirrored;
        if ( aSeq[3] >>= aURL )
            pImp->aURL = aURL;
        return TRUE;
    }

    return FALSE;
}

void SfxImageItem::SetRotation( long nValue )
{
    pImp->nAngle = nValue;
}

long SfxImageItem::GetRotation() const
{
    return pImp->nAngle;
}

void SfxImageItem::SetMirrored( BOOL bSet )
{
    pImp->bMirrored = bSet;
}

BOOL SfxImageItem::IsMirrored() const
{
    return pImp->bMirrored;
}

String SfxImageItem::GetURL() const
{
    return pImp->aURL;
}

