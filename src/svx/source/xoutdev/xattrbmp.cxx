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
#include <com/sun/star/awt/XBitmap.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <tools/stream.hxx>
#include <vcl/window.hxx>
#include <vcl/virdev.hxx>
#include <vcl/bitmapex.hxx>
#include <toolkit/unohlp.hxx>
#include <svtools/style.hxx>

#include <svx/dialogs.hrc>
#include "xattr.hxx"
#include <svx/xtable.hxx>
#include <svx/xdef.hxx>
#include <svx/unomid.hxx>
#include <svx/unoprnms.hxx>

#include "unoapi.hxx"
#include <svx/svdmodel.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>

#define GLOBALOVERFLOW

using namespace ::com::sun::star;

// ---------------
// class XOBitmap
// ---------------

/*************************************************************************
|*
|*    XOBitmap::XOBitmap()
|*
|*    Beschreibung
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

XOBitmap::XOBitmap() :
	eType			( XBITMAP_NONE ),
	eStyle			( XBITMAP_STRETCH ),
	pPixelArray		( NULL ),
	bGraphicDirty	( FALSE )
{
}

/*************************************************************************
|*
|*    XOBitmap::XOBitmap( Bitmap aBitmap, XBitmapStyle eStyle = XBITMAP_TILE )
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

XOBitmap::XOBitmap( const Bitmap& rBmp, XBitmapStyle eInStyle ) :
	eType			( XBITMAP_IMPORT ),
	eStyle			( eInStyle ),
	aGraphicObject	( rBmp ),
	pPixelArray		( NULL ),
	bGraphicDirty	( FALSE )
{
}

/*************************************************************************
|*
|*    XOBitmap::XOBitmap( Bitmap aBitmap, XBitmapStyle eStyle = XBITMAP_TILE )
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

XOBitmap::XOBitmap( const GraphicObject& rGraphicObject, XBitmapStyle eInStyle ) :
	eType			( XBITMAP_IMPORT ),
	eStyle			( eInStyle ),
	aGraphicObject	( rGraphicObject ),
	pPixelArray		( NULL ),
	bGraphicDirty	( FALSE )
{
}

/*************************************************************************
|*
|*    XOBitmap::XOBitmap( USHORT* pArray, const Color& aPixelColor,
|*          const Color& aBckgrColor, const Size& rSize = Size( 8, 8 ),
|*          XBitmapStyle eStyle = XBITMAP_TILE )
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

XOBitmap::XOBitmap( const USHORT* pArray, const Color& rPixelColor,
			const Color& rBckgrColor, const Size& rSize,
			XBitmapStyle eInStyle ) :
	eStyle			( eInStyle ),
	pPixelArray		( NULL ),
	aArraySize		( rSize ),
	aPixelColor		( rPixelColor ),
	aBckgrColor		( rBckgrColor ),
	bGraphicDirty	( TRUE )

{
	if( aArraySize.Width() == 8 && aArraySize.Height() == 8 )
	{
		eType = XBITMAP_8X8;
		pPixelArray = new USHORT[ 64 ];

		for( USHORT i = 0; i < 64; i++ )
			*( pPixelArray + i ) = *( pArray + i );
	}
	else
	{
		DBG_ASSERT( 0, "Nicht unterstuetzte Bitmapgroesse" );
	}
}

/*************************************************************************
|*
|*    XOBitmap::XOBitmap( const XOBitmap& rXBmp )
|*
|*    Beschreibung
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

XOBitmap::XOBitmap( const XOBitmap& rXBmp ) :
	pPixelArray ( NULL )
{
	eType = rXBmp.eType;
	eStyle = rXBmp.eStyle;
	aGraphicObject = rXBmp.aGraphicObject;
	aArraySize = rXBmp.aArraySize;
	aPixelColor = rXBmp.aPixelColor;
	aBckgrColor = rXBmp.aBckgrColor;
	bGraphicDirty = rXBmp.bGraphicDirty;

	if( rXBmp.pPixelArray )
	{
		if( eType == XBITMAP_8X8 )
		{
			pPixelArray = new USHORT[ 64 ];

			for( USHORT i = 0; i < 64; i++ )
				*( pPixelArray + i ) = *( rXBmp.pPixelArray + i );
		}
	}
}

/*************************************************************************
|*
|*    XOBitmap::XOBitmap( Bitmap aBitmap, XBitmapStyle eStyle = XBITMAP_TILE )
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

XOBitmap::~XOBitmap()
{
	if( pPixelArray )
		delete []pPixelArray;
}

/*************************************************************************
|*
|*    XOBitmap& XOBitmap::operator=( const XOBitmap& rXBmp )
|*
|*    Beschreibung
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

XOBitmap& XOBitmap::operator=( const XOBitmap& rXBmp )
{
	eType = rXBmp.eType;
	eStyle = rXBmp.eStyle;
	aGraphicObject = rXBmp.aGraphicObject;
	aArraySize = rXBmp.aArraySize;
	aPixelColor = rXBmp.aPixelColor;
	aBckgrColor = rXBmp.aBckgrColor;
	bGraphicDirty = rXBmp.bGraphicDirty;

	if( rXBmp.pPixelArray )
	{
		if( eType == XBITMAP_8X8 )
		{
			pPixelArray = new USHORT[ 64 ];

			for( USHORT i = 0; i < 64; i++ )
				*( pPixelArray + i ) = *( rXBmp.pPixelArray + i );
		}
	}
	return( *this );
}

/*************************************************************************
|*
|*    int XOBitmap::operator==( const XOBitmap& rXOBitmap ) const
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

int XOBitmap::operator==( const XOBitmap& rXOBitmap ) const
{
	if( eType != rXOBitmap.eType      ||
		eStyle != rXOBitmap.eStyle         ||
		aGraphicObject != rXOBitmap.aGraphicObject ||
		aArraySize != rXOBitmap.aArraySize     ||
		aPixelColor != rXOBitmap.aPixelColor ||
		aBckgrColor != rXOBitmap.aBckgrColor ||
		bGraphicDirty != rXOBitmap.bGraphicDirty )
	{
		return( FALSE );
	}

	if( pPixelArray && rXOBitmap.pPixelArray )
	{
		USHORT nCount = (USHORT) ( aArraySize.Width() * aArraySize.Height() );
		for( USHORT i = 0; i < nCount; i++ )
		{
			if( *( pPixelArray + i ) != *( rXOBitmap.pPixelArray + i ) )
				return( FALSE );
		}
	}
	return( TRUE );
}

/*************************************************************************
|*
|*    void SetPixelArray( const USHORT* pArray )
|*
|*    Beschreibung
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

void XOBitmap::SetPixelArray( const USHORT* pArray )
{
	if( eType == XBITMAP_8X8 )
	{
		if( pPixelArray )
			delete []pPixelArray;

		pPixelArray = new USHORT[ 64 ];

		for( USHORT i = 0; i < 64; i++ )
			*( pPixelArray + i ) = *( pArray + i );

		bGraphicDirty = TRUE;
	}
	else
	{
		DBG_ASSERT( 0, "Nicht unterstuetzter Bitmaptyp" );
	}
}

/*************************************************************************
|*
|*    Bitmap XOBitmap::GetBitmap()
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

Bitmap XOBitmap::GetBitmap() const
{
	return GetGraphicObject().GetGraphic().GetBitmap();
}

/*************************************************************************
|*
|*    Bitmap XOBitmap::GetGraphicObject()
|*
|*    Beschreibung
|*    Ersterstellung
|*    Letzte Aenderung
|*
*************************************************************************/

const GraphicObject& XOBitmap::GetGraphicObject() const
{
	if( bGraphicDirty )
		( (XOBitmap*) this )->Array2Bitmap();

	return aGraphicObject;
}

/*************************************************************************
|*
|*    void XOBitmap::Bitmap2Array()
|*
|*    Beschreibung      Umwandlung der Bitmap in Array, Hinter- u.
|*                      Vordergrundfarbe
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

void XOBitmap::Bitmap2Array()
{
	VirtualDevice   aVD;
	BOOL            bPixelColor = FALSE;
	const Bitmap	aBitmap( GetBitmap() );
	const USHORT	nLines = 8; // von Type abhaengig

	if( !pPixelArray )
		pPixelArray = new USHORT[ nLines * nLines ];

	aVD.SetOutputSizePixel( aBitmap.GetSizePixel() );
	aVD.DrawBitmap( Point(), aBitmap );
	aPixelColor = aBckgrColor = aVD.GetPixel( Point() );

	// Aufbau des Arrays und Ermittlung der Vorder-, bzw.
	// Hintergrundfarbe
	for( USHORT i = 0; i < nLines; i++ )
	{
		for( USHORT j = 0; j < nLines; j++ )
		{
			if ( aVD.GetPixel( Point( j, i ) ) == aBckgrColor )
				*( pPixelArray + j + i * nLines ) = 0;
			else
			{
				*( pPixelArray + j + i * nLines ) = 1;
				if( !bPixelColor )
				{
					aPixelColor = aVD.GetPixel( Point( j, i ) );
					bPixelColor = TRUE;
				}
			}
		}
	}
}

/*************************************************************************
|*
|*    void XOBitmap::Array2Bitmap()
|*
|*    Beschreibung      Umwandlung des Arrays, Hinter- u.
|*                      Vordergrundfarbe in eine Bitmap
|*    Ersterstellung    27.07.95
|*    Letzte Aenderung  27.07.95
|*
*************************************************************************/

void XOBitmap::Array2Bitmap()
{
	VirtualDevice   aVD;
	USHORT          nLines = 8; // von Type abhaengig

	if( !pPixelArray )
		return;

	aVD.SetOutputSizePixel( Size( nLines, nLines ) );

	// Aufbau der Bitmap
	for( USHORT i = 0; i < nLines; i++ )
	{
		for( USHORT j = 0; j < nLines; j++ )
		{
			if( *( pPixelArray + j + i * nLines ) == 0 )
				aVD.DrawPixel( Point( j, i ), aBckgrColor );
			else
				aVD.DrawPixel( Point( j, i ), aPixelColor );
		}
	}

	aGraphicObject = GraphicObject( aVD.GetBitmap( Point(), Size( nLines, nLines ) ) );
	bGraphicDirty = FALSE;
}

// -----------------------
// class XFillBitmapItem
// -----------------------
TYPEINIT1_AUTOFACTORY(XFillBitmapItem, NameOrIndex);

/*************************************************************************
|*
|*    XFillBitmapItem::XFillBitmapItem(long nIndex,
|*                                   const Bitmap& rTheBitmap)
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  17.11.94
|*
*************************************************************************/

XFillBitmapItem::XFillBitmapItem(long nIndex,
							   const XOBitmap& rTheBitmap) :
	NameOrIndex( XATTR_FILLBITMAP, nIndex ),
	aXOBitmap( rTheBitmap )
{
}

/*************************************************************************
|*
|*    XFillBitmapItem::XFillBitmapItem(const XubString& rName,
|*                                 const Bitmap& rTheBitmap)
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  17.11.94
|*
*************************************************************************/

XFillBitmapItem::XFillBitmapItem(const XubString& rName,
							   const XOBitmap& rTheBitmap) :
	NameOrIndex( XATTR_FILLBITMAP, rName ),
	aXOBitmap( rTheBitmap )
{
}

/*************************************************************************
|*
|*    XFillBitmapItem::XFillBitmapItem(const XFillBitmapItem& rItem)
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  17.11.94
|*
*************************************************************************/

XFillBitmapItem::XFillBitmapItem(const XFillBitmapItem& rItem) :
	NameOrIndex( rItem ),
	aXOBitmap( rItem.aXOBitmap )
{
}

/*************************************************************************
|*
|*    XFillBitmapItem::XFillBitmapItem(SvStream& rIn)
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  26.07.94
|*
*************************************************************************/

XFillBitmapItem::XFillBitmapItem( SvStream& rIn, USHORT nVer ) :
	NameOrIndex( XATTR_FILLBITMAP, rIn )
{
	if( nVer == 0 )
	{
		if (!IsIndex())
		{
			// Behandlung der alten Bitmaps
			Bitmap aBmp;

			rIn >> aBmp;

			aXOBitmap.SetBitmap( aBmp );
			aXOBitmap.SetBitmapStyle( XBITMAP_TILE );

			if( aBmp.GetSizePixel().Width() == 8 &&
				aBmp.GetSizePixel().Height() == 8 )
			{
				aXOBitmap.SetBitmapType( XBITMAP_8X8 );
				aXOBitmap.Bitmap2Array();
			}
			else
				aXOBitmap.SetBitmapType( XBITMAP_IMPORT );
		}
	}
	else if( nVer == 1 )
	{
		if (!IsIndex())
		{
			INT16 iTmp;
			rIn >> iTmp;
			aXOBitmap.SetBitmapStyle( (XBitmapStyle) iTmp );
			rIn >> iTmp;
			aXOBitmap.SetBitmapType( (XBitmapType) iTmp );

			if( aXOBitmap.GetBitmapType() == XBITMAP_IMPORT )
			{
				Bitmap aBmp;
				rIn >> aBmp;
				aXOBitmap.SetBitmap( aBmp );
			}
			else if( aXOBitmap.GetBitmapType() == XBITMAP_8X8 )
			{
				USHORT* pArray = new USHORT[ 64 ];
				Color   aColor;

				for( USHORT i = 0; i < 64; i++ )
					rIn >> *( pArray + i );
				aXOBitmap.SetPixelArray( pArray );

				rIn >> aColor;
				aXOBitmap.SetPixelColor( aColor );
				rIn >> aColor;
				aXOBitmap.SetBackgroundColor( aColor );

				delete []pArray;
			}
		}
	}

	// #81908# force bitmap to exist
	aXOBitmap.GetBitmap();
}

//*************************************************************************

XFillBitmapItem::XFillBitmapItem( SfxItemPool* /*pPool*/, const XOBitmap& rTheBitmap )
: 	NameOrIndex( XATTR_FILLBITMAP, -1 ),
	aXOBitmap( rTheBitmap )
{
}

//*************************************************************************

XFillBitmapItem::XFillBitmapItem( SfxItemPool* /*pPool*/)
: NameOrIndex(XATTR_FILLBITMAP, -1 )
{
}

/*************************************************************************
|*
|*    XFillBitmapItem::Clone(SfxItemPool* pPool) const
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  17.11.94
|*
*************************************************************************/

SfxPoolItem* XFillBitmapItem::Clone(SfxItemPool* /*pPool*/) const
{
	return new XFillBitmapItem(*this);
}

/*************************************************************************
|*
|*    int XFillBitmapItem::operator==(const SfxPoolItem& rItem) const
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

int XFillBitmapItem::operator==(const SfxPoolItem& rItem) const
{
	return ( NameOrIndex::operator==(rItem) &&
			 aXOBitmap == ((const XFillBitmapItem&) rItem).aXOBitmap );
}

/*************************************************************************
|*
|*    SfxPoolItem* XFillBitmapItem::Create(SvStream& rIn, USHORT nVer) const
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  17.11.94
|*
*************************************************************************/

SfxPoolItem* XFillBitmapItem::Create(SvStream& rIn, USHORT nVer) const
{
	return new XFillBitmapItem( rIn, nVer );
}

/*************************************************************************
|*
|*    SfxPoolItem* XFillBitmapItem::Store(SvStream& rOut) const
|*
|*    Beschreibung
|*    Ersterstellung    17.11.94
|*    Letzte Aenderung  26.07.94
|*
*************************************************************************/

SvStream& XFillBitmapItem::Store( SvStream& rOut, USHORT nItemVersion ) const
{
	NameOrIndex::Store( rOut, nItemVersion );

	if (!IsIndex())
	{
		rOut << (INT16) aXOBitmap.GetBitmapStyle();
		if( !aXOBitmap.GetBitmap() )
			rOut << (INT16) XBITMAP_NONE;
		else
		{
			rOut << (INT16) aXOBitmap.GetBitmapType();
			if( aXOBitmap.GetBitmapType() == XBITMAP_IMPORT )
			{
				const USHORT    nOldComprMode = rOut.GetCompressMode();
				USHORT          nNewComprMode = nOldComprMode;

				if( rOut.GetVersion() >= SOFFICE_FILEFORMAT_50 )
					nNewComprMode |= COMPRESSMODE_ZBITMAP;
				else
					nNewComprMode &= ~COMPRESSMODE_ZBITMAP;

				rOut.SetCompressMode( nNewComprMode );
				rOut << aXOBitmap.GetBitmap();
				rOut.SetCompressMode( nOldComprMode );
			}
			else if( aXOBitmap.GetBitmapType() == XBITMAP_8X8 )
			{
				USHORT* pArray = aXOBitmap.GetPixelArray();
				for( USHORT i = 0; i < 64; i++ )
					rOut << (USHORT) *( pArray + i );

				rOut << aXOBitmap.GetPixelColor();
				rOut << aXOBitmap.GetBackgroundColor();
			}
		}
	}

	return rOut;
}

/*************************************************************************
|*
|*    const Bitmap& XFillBitmapItem::GetValue(const XBitmapTable* pTable) const
|*
|*    Beschreibung
|*    Ersterstellung    15.11.94
|*    Letzte Aenderung  26.07.94
|*
*************************************************************************/

const XOBitmap& XFillBitmapItem::GetBitmapValue(const XBitmapTable* pTable) const // GetValue -> GetBitmapValue
{
	if (!IsIndex())
		return aXOBitmap;
	else
		return pTable->GetBitmap(GetIndex())->GetXBitmap();
}


/*************************************************************************
|*
|*    USHORT XFillBitmapItem::GetVersion() const
|*
|*    Beschreibung
|*    Ersterstellung    26.07.95
|*    Letzte Aenderung  26.07.95
|*
*************************************************************************/

USHORT XFillBitmapItem::GetVersion( USHORT /*nFileFormatVersion*/) const
{
	// 2. Version
	return( 1 );
}

//------------------------------------------------------------------------

SfxItemPresentation XFillBitmapItem::GetPresentation
(
	SfxItemPresentation ePres,
	SfxMapUnit          /*eCoreUnit*/,
	SfxMapUnit          /*ePresUnit*/,
    XubString&           rText, const IntlWrapper *
)   const
{
	switch ( ePres )
	{
		case SFX_ITEM_PRESENTATION_NONE:
			rText.Erase();
			return ePres;
		case SFX_ITEM_PRESENTATION_NAMELESS:
		case SFX_ITEM_PRESENTATION_COMPLETE:
			rText += GetName();
			return ePres;
		default:
			return SFX_ITEM_PRESENTATION_NONE;
	}
}

//------------------------------------------------------------------------

sal_Bool XFillBitmapItem::QueryValue( ::com::sun::star::uno::Any& rVal, BYTE nMemberId ) const
{
//    sal_Bool bConvert = 0!=(nMemberId&CONVERT_TWIPS);
    nMemberId &= ~CONVERT_TWIPS;

    // needed for MID_NAME
    ::rtl::OUString aApiName;
    // needed for complete item (MID 0)
    ::rtl::OUString aInternalName;

    ::rtl::OUString aURL;
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XBitmap > xBmp;

	if( nMemberId == MID_NAME )
	{
 		SvxUnogetApiNameForItem( Which(), GetName(), aApiName );
	}
    else if( nMemberId == 0  )
    {
        aInternalName = GetName();
    }

    if( nMemberId == MID_GRAFURL ||
        nMemberId == 0 )
	{
		XOBitmap aLocalXOBitmap( GetBitmapValue() );
		aURL = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(UNO_NAME_GRAPHOBJ_URLPREFIX));
		aURL += ::rtl::OUString::createFromAscii( aLocalXOBitmap.GetGraphicObject().GetUniqueID().GetBuffer() );
	}
    if( nMemberId == MID_BITMAP ||
        nMemberId == 0  )
	{
		XOBitmap aLocalXOBitmap( GetBitmapValue() );
		Bitmap aBmp( aLocalXOBitmap.GetBitmap() );
		BitmapEx aBmpEx( aBmp );

		xBmp.set( VCLUnoHelper::CreateBitmap( aBmpEx ) );
	}

    if( nMemberId == MID_NAME )
		rVal <<= aApiName;
	else if( nMemberId == MID_GRAFURL )
		rVal <<= aURL;
	else if( nMemberId == MID_BITMAP )
		rVal <<= xBmp;
    else
    {
        // member-id 0 => complete item (e.g. for toolbars)
        DBG_ASSERT( nMemberId == 0, "invalid member-id" );
        uno::Sequence< beans::PropertyValue > aPropSeq( 3 );

        aPropSeq[0].Name  = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Name" ));
        aPropSeq[0].Value = uno::makeAny( aInternalName );
        aPropSeq[1].Name  = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "FillBitmapURL" ));
        aPropSeq[1].Value = uno::makeAny( aURL );
        aPropSeq[2].Name  = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Bitmap" ));
        aPropSeq[2].Value = uno::makeAny( xBmp );

        rVal <<= aPropSeq;
    }

	return sal_True;
}

//------------------------------------------------------------------------

sal_Bool XFillBitmapItem::PutValue( const ::com::sun::star::uno::Any& rVal, BYTE nMemberId )
{
//    sal_Bool bConvert = 0!=(nMemberId&CONVERT_TWIPS);
    nMemberId &= ~CONVERT_TWIPS;

    ::rtl::OUString aName;
    ::rtl::OUString aURL;
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XBitmap > xBmp;
    ::com::sun::star::uno::Reference< ::com::sun::star::graphic::XGraphic > xGraphic;

    bool bSetName   = false;
    bool bSetURL    = false;
    bool bSetBitmap = false;

    if( nMemberId == MID_NAME )
		bSetName = (rVal >>= aName);
    else if( nMemberId == MID_GRAFURL )
		bSetURL = (rVal >>= aURL);
	else if( nMemberId == MID_BITMAP )
	{
        bSetBitmap = (rVal >>= xBmp);
		if ( !bSetBitmap )
			bSetBitmap = (rVal >>= xGraphic );
	}
    else
    {
        DBG_ASSERT( nMemberId == 0, "invalid member-id" );
        uno::Sequence< beans::PropertyValue >   aPropSeq;
        if( rVal >>= aPropSeq )
        {
            for ( sal_Int32 n = 0; n < aPropSeq.getLength(); n++ )
            {
                if( aPropSeq[n].Name.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "Name" )))
                    bSetName = (aPropSeq[n].Value >>= aName);
                else if( aPropSeq[n].Name.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "FillBitmapURL" )))
                    bSetURL = (aPropSeq[n].Value >>= aURL);
                else if( aPropSeq[n].Name.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "Bitmap" )))
                    bSetBitmap = (aPropSeq[n].Value >>= xBmp);
            }
        }
    }

    if( bSetName )
    {
        SetName( aName );
    }
    if( bSetURL )
    {
        GraphicObject aGrafObj( CreateGraphicObjectFromURL( aURL ) );
        XOBitmap aBMP( aGrafObj );
        SetBitmapValue( aBMP );
    }
    if( bSetBitmap )
    {
		Bitmap aInput;
		if ( xBmp.is() )
		{
	        BitmapEx aInputEx( VCLUnoHelper::GetBitmap( xBmp ) );
		    aInput = aInputEx.GetBitmap();
		}
		else if ( xGraphic.is() )
		{
			Graphic aGraphic( xGraphic );
		    aInput = aGraphic.GetBitmap();
		}

        // note: aXOBitmap is the member bitmap
        aXOBitmap.SetBitmap( aInput );
        aXOBitmap.SetBitmapType(XBITMAP_IMPORT);

        if(aInput.GetSizePixel().Width() == 8
           && aInput.GetSizePixel().Height() == 8
           && aInput.GetColorCount() == 2)
        {
            aXOBitmap.Bitmap2Array();
            aXOBitmap.SetBitmapType(XBITMAP_8X8);
            aXOBitmap.SetPixelSize(aInput.GetSizePixel());
        }
    }

	return (bSetName || bSetURL || bSetBitmap);
}

BOOL XFillBitmapItem::CompareValueFunc( const NameOrIndex* p1, const NameOrIndex* p2 )
{
	return ((XFillBitmapItem*)p1)->GetBitmapValue().GetGraphicObject().GetUniqueID() ==
		   ((XFillBitmapItem*)p2)->GetBitmapValue().GetGraphicObject().GetUniqueID();
}

XFillBitmapItem* XFillBitmapItem::checkForUniqueItem( SdrModel* pModel ) const
{
	if( pModel )
	{
		const String aUniqueName = NameOrIndex::CheckNamedItem(	this,
																XATTR_FILLBITMAP,
																&pModel->GetItemPool(),
																pModel->GetStyleSheetPool() ? &pModel->GetStyleSheetPool()->GetPool() : NULL,
																XFillBitmapItem::CompareValueFunc,
																RID_SVXSTR_BMP21,
																pModel->GetBitmapList() );

		// if the given name is not valid, replace it!
		if( aUniqueName != GetName() )
		{
			return new XFillBitmapItem( aUniqueName, aXOBitmap );
		}
	}

	return (XFillBitmapItem*)this;
}
