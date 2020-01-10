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
#include "precompiled_svx.hxx"

#define _SVSTDARR_ULONGS
#define _SVSTDARR_STRINGSDTOR

#include <svtools/svstdarr.hxx>
#include <clipfmtitem.hxx>
#include <com/sun/star/frame/status/ClipboardFormats.hpp>

struct SvxClipboardFmtItem_Impl
{
	SvStringsDtor aFmtNms;
	SvULongs aFmtIds;
	static String sEmptyStr;

	SvxClipboardFmtItem_Impl() : aFmtNms( 8, 8 ), aFmtIds( 8, 8 ) {}
	SvxClipboardFmtItem_Impl( const SvxClipboardFmtItem_Impl& );
};

String SvxClipboardFmtItem_Impl::sEmptyStr;

TYPEINIT1_FACTORY( SvxClipboardFmtItem, SfxPoolItem , new  SvxClipboardFmtItem(0));

SvxClipboardFmtItem_Impl::SvxClipboardFmtItem_Impl(
							const SvxClipboardFmtItem_Impl& rCpy )
{
	aFmtIds.Insert( &rCpy.aFmtIds, 0 );
	for( USHORT n = 0, nEnd = rCpy.aFmtNms.Count(); n < nEnd; ++n )
	{
		String* pStr = rCpy.aFmtNms[ n ];
		if( pStr )
			pStr = new String( *pStr );
		aFmtNms.Insert( pStr, n );
	}
}

SvxClipboardFmtItem::SvxClipboardFmtItem( USHORT nId )
	: SfxPoolItem( nId ), pImpl( new SvxClipboardFmtItem_Impl )
{
}

SvxClipboardFmtItem::SvxClipboardFmtItem( const SvxClipboardFmtItem& rCpy )
	: SfxPoolItem( rCpy.Which() ),
	pImpl( new SvxClipboardFmtItem_Impl( *rCpy.pImpl ) )
{
}

SvxClipboardFmtItem::~SvxClipboardFmtItem()
{
	delete pImpl;
}

BOOL SvxClipboardFmtItem::QueryValue( com::sun::star::uno::Any& rVal, BYTE /*nMemberId*/ ) const
{
    USHORT nCount = Count();

    ::com::sun::star::frame::status::ClipboardFormats aClipFormats;

    aClipFormats.Identifiers.realloc( nCount );
    aClipFormats.Names.realloc( nCount );
    for ( USHORT n=0; n < nCount; n++ )
    {
        aClipFormats.Identifiers[n] = (sal_Int64)GetClipbrdFormatId( n );
        aClipFormats.Names[n] = GetClipbrdFormatName( n );
    }

    rVal <<= aClipFormats;
    return TRUE;
}

sal_Bool SvxClipboardFmtItem::PutValue( const ::com::sun::star::uno::Any& rVal, BYTE /*nMemberId*/ )
{
    ::com::sun::star::frame::status::ClipboardFormats aClipFormats;
    if ( rVal >>= aClipFormats )
    {
        USHORT nCount = USHORT( aClipFormats.Identifiers.getLength() );

        pImpl->aFmtIds.Remove( 0, pImpl->aFmtIds.Count() );
        pImpl->aFmtNms.Remove( 0, pImpl->aFmtNms.Count() );
        for ( USHORT n=0; n < nCount; n++ )
            AddClipbrdFormat( ULONG( aClipFormats.Identifiers[n] ), aClipFormats.Names[n], n );

        return sal_True;
    }

    return sal_False;
}

int SvxClipboardFmtItem::operator==( const SfxPoolItem& rComp ) const
{
	int nRet = 0;
	const SvxClipboardFmtItem& rCmp = (SvxClipboardFmtItem&)rComp;
	if( rCmp.pImpl->aFmtNms.Count() == pImpl->aFmtNms.Count() )
	{
		nRet = 1;
		const String* pStr1, *pStr2;
		for( USHORT n = 0, nEnd = rCmp.pImpl->aFmtNms.Count(); n < nEnd; ++n )
		{
			if( pImpl->aFmtIds[ n ] != rCmp.pImpl->aFmtIds[ n ] ||
				( (0 == ( pStr1 = pImpl->aFmtNms[ n ] )) ^
				  (0 == ( pStr2 = rCmp.pImpl->aFmtNms[ n ] ) )) ||
				( pStr1 && *pStr1 != *pStr2 ))
			{
				nRet = 0;
				break;
			}
		}
	}
	return nRet;
}

SfxPoolItem* SvxClipboardFmtItem::Clone( SfxItemPool * /*pPool*/ ) const
{
	return new SvxClipboardFmtItem( *this );
}

void SvxClipboardFmtItem::AddClipbrdFormat( ULONG nId, USHORT nPos )
{
	if( nPos > pImpl->aFmtNms.Count() )
		nPos = pImpl->aFmtNms.Count();
	String* pStr = 0;
	pImpl->aFmtNms.Insert( pStr, nPos );
	pImpl->aFmtIds.Insert( nId, nPos );
}

void SvxClipboardFmtItem::AddClipbrdFormat( ULONG nId, const String& rName,
							USHORT nPos )
{
	if( nPos > pImpl->aFmtNms.Count() )
		nPos = pImpl->aFmtNms.Count();
	String* pStr = new String( rName );
	pImpl->aFmtNms.Insert( pStr, nPos );
	pImpl->aFmtIds.Insert( nId, nPos );
}

USHORT SvxClipboardFmtItem::Count() const
{
	return pImpl->aFmtIds.Count();
}

ULONG SvxClipboardFmtItem::GetClipbrdFormatId( USHORT nPos ) const
{
	return pImpl->aFmtIds[ nPos ];
}

const String& SvxClipboardFmtItem::GetClipbrdFormatName( USHORT nPos ) const
{
	const String* pS = pImpl->aFmtNms[ nPos ];
	return pS ? *pS : SvxClipboardFmtItem_Impl::sEmptyStr;
}


