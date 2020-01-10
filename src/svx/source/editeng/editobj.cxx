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


#include <eeng_pch.hxx>

#define ENABLE_STRING_STREAM_OPERATORS
#include <tools/stream.hxx>

#include <editobj2.hxx>
#include <svx/editdata.hxx>
#include <editattr.hxx>
#include <svx/editeng.hxx>
#include <fontitem.hxx>
#include <svx/cscoitem.hxx>
#include <svx/flditem.hxx>
#include <svx/lrspitem.hxx>
#include <svx/tstpitem.hxx>
#include <bulitem.hxx>
#include <svx/numitem.hxx>
#include <svx/brshitem.hxx>
#include <vcl/graph.hxx>
#include <svtools/intitem.hxx>
#include <vcl/fontcvt.hxx>
#include <tools/tenccvt.hxx>

DBG_NAME( EE_EditTextObject )
DBG_NAME( XEditAttribute )

//--------------------------------------------------------------

BOOL lcl_CreateBulletItem( const SvxNumBulletItem& rNumBullet, USHORT nLevel, SvxBulletItem& rBullet )
{
	const SvxNumberFormat* pFmt = rNumBullet.GetNumRule()->Get( nLevel );
	if ( pFmt )
	{
		rBullet.SetWidth( (-pFmt->GetFirstLineOffset()) + pFmt->GetCharTextDistance() );
		rBullet.SetSymbol( pFmt->GetBulletChar() );
		rBullet.SetPrevText( pFmt->GetPrefix() );
		rBullet.SetFollowText( pFmt->GetSuffix() );
		rBullet.SetStart( pFmt->GetStart() );
		rBullet.SetScale( pFmt->GetBulletRelSize() );

		Font aBulletFont( rBullet.GetFont() );
		if ( pFmt->GetBulletFont() )
			aBulletFont = *pFmt->GetBulletFont();
		aBulletFont.SetColor( pFmt->GetBulletColor() );
		rBullet.SetFont( aBulletFont );

		if ( pFmt->GetBrush() && pFmt->GetBrush()->GetGraphic() )
		{
			Bitmap aBmp( pFmt->GetBrush()->GetGraphic()->GetBitmap() );
			aBmp.SetPrefSize( pFmt->GetGraphicSize() );
			aBmp.SetPrefMapMode( MAP_100TH_MM );
			rBullet.SetBitmap( aBmp );
		}

		switch ( pFmt->GetNumberingType() )
		{
			case SVX_NUM_CHARS_UPPER_LETTER:
			case SVX_NUM_CHARS_UPPER_LETTER_N:
				rBullet.SetStyle( BS_ABC_BIG );
				break;
			case SVX_NUM_CHARS_LOWER_LETTER:
			case SVX_NUM_CHARS_LOWER_LETTER_N:
				rBullet.SetStyle( BS_ABC_SMALL );
				break;
			case SVX_NUM_ROMAN_UPPER:
				rBullet.SetStyle( BS_ROMAN_BIG );
				break;
			case SVX_NUM_ROMAN_LOWER:
				rBullet.SetStyle( BS_ROMAN_SMALL );
				break;
			case SVX_NUM_ARABIC:
				rBullet.SetStyle( BS_123 );
				break;
			case SVX_NUM_NUMBER_NONE:
				rBullet.SetStyle( BS_NONE );
				break;
			case SVX_NUM_CHAR_SPECIAL:
				rBullet.SetStyle( BS_BULLET );
				break;
			case SVX_NUM_PAGEDESC:
				DBG_ERROR( "Unknown: SVX_NUM_PAGEDESC" );
				rBullet.SetStyle( BS_BULLET );
				break;
			case SVX_NUM_BITMAP:
				rBullet.SetStyle( BS_BMP );
				break;
			default:
				DBG_ERROR( "Unknown NumType" );
		}

		switch ( pFmt->GetNumAdjust() )
		{
			case SVX_ADJUST_LEFT:
				rBullet.SetJustification( BJ_VCENTER|BJ_HLEFT );
				break;
			case SVX_ADJUST_RIGHT:
				rBullet.SetJustification( BJ_VCENTER|BJ_HRIGHT );
				break;
			case SVX_ADJUST_CENTER:
				rBullet.SetJustification( BJ_VCENTER|BJ_HCENTER );
				break;
			default:
				DBG_ERROR( "Unknown or invalid NumAdjust" );
		}
	}
	return pFmt ? TRUE : FALSE;
}


XEditAttribute* MakeXEditAttribute( SfxItemPool& rPool, const SfxPoolItem& rItem, USHORT nStart, USHORT nEnd )
{
	// das neue Attribut im Pool anlegen
	const SfxPoolItem& rNew = rPool.Put( rItem );

	XEditAttribute* pNew = new XEditAttribute( rNew, nStart, nEnd );
	return pNew;
}


XEditAttribute::XEditAttribute( const SfxPoolItem& rAttr )
{
	DBG_CTOR( XEditAttribute, 0 );
	pItem = &rAttr;
	nStart = 0;
	nEnd = 0;
}

XEditAttribute::XEditAttribute( const SfxPoolItem& rAttr, USHORT nS, USHORT nE )
{
	DBG_CTOR( XEditAttribute, 0 );
	pItem = &rAttr;
	nStart = nS;
	nEnd = nE;
}

XEditAttribute::~XEditAttribute()
{
	DBG_DTOR( XEditAttribute, 0 );
	pItem = 0;	// Gehoert dem Pool.
}

XEditAttribute* XEditAttributeList::FindAttrib( USHORT _nWhich, USHORT nChar ) const
{
    for ( USHORT n = Count(); n; )
    {
        XEditAttribute* pAttr = GetObject( --n );
        if( ( pAttr->GetItem()->Which() == _nWhich ) && ( pAttr->GetStart() <= nChar ) && ( pAttr->GetEnd() > nChar ) )
            return pAttr;
    }
    return NULL;
}

ContentInfo::ContentInfo( SfxItemPool& rPool ) : aParaAttribs( rPool, EE_PARA_START, EE_CHAR_END )
{
	eFamily = SFX_STYLE_FAMILY_PARA;
	pWrongs = NULL;
/* cl removed because not needed anymore since binfilter
    pTempLoadStoreInfos = NULL;
*/
}

// Richtiger CopyCTOR unsinning, weil ich mit einem anderen Pool arbeiten muss!
ContentInfo::ContentInfo( const ContentInfo& rCopyFrom, SfxItemPool& rPoolToUse )
	: aParaAttribs( rPoolToUse, EE_PARA_START, EE_CHAR_END )
{
	pWrongs = NULL;
/* cl removed because not needed anymore since binfilter
    pTempLoadStoreInfos = NULL;
*/
	if ( rCopyFrom.GetWrongList() )
		pWrongs = rCopyFrom.GetWrongList()->Clone();
	// So sollten die Items im richtigen Pool landen!
	aParaAttribs.Set( rCopyFrom.GetParaAttribs() );
	aText = rCopyFrom.GetText();
	aStyle = rCopyFrom.GetStyle();
	eFamily = rCopyFrom.GetFamily();

	// Attribute kopieren...
	for ( USHORT n = 0; n < rCopyFrom.GetAttribs().Count(); n++  )
	{
		XEditAttribute* pAttr = rCopyFrom.GetAttribs().GetObject( n );
		XEditAttribute* pMyAttr = MakeXEditAttribute( rPoolToUse, *pAttr->GetItem(), pAttr->GetStart(), pAttr->GetEnd() );
		aAttribs.Insert( pMyAttr, aAttribs.Count()  );
	}

	// Wrongs
	pWrongs = NULL;
#ifndef SVX_LIGHT
	if ( rCopyFrom.GetWrongList() )
		pWrongs = rCopyFrom.GetWrongList()->Clone();
#endif // !SVX_LIGHT
}

ContentInfo::~ContentInfo()
{
	for ( USHORT nAttr = 0; nAttr < aAttribs.Count(); nAttr++ )
	{
		XEditAttribute* pAttr = aAttribs.GetObject(nAttr);
		// Item aus Pool entfernen!
		aParaAttribs.GetPool()->Remove( *pAttr->GetItem() );
		delete pAttr;
	}
	aAttribs.Remove( 0, aAttribs.Count() );
#ifndef SVX_LIGHT
	delete pWrongs;
#endif
}

/* cl removed because not needed anymore since binfilter
void ContentInfo::CreateLoadStoreTempInfos()
{
    delete pTempLoadStoreInfos; 
    pTempLoadStoreInfos = new LoadStoreTempInfos;
}

void ContentInfo::DestroyLoadStoreTempInfos()
{
    delete pTempLoadStoreInfos; 
    pTempLoadStoreInfos = NULL; 
}
*/

// #i102062#
bool ContentInfo::isWrongListEqual(const ContentInfo& rCompare) const
{
    if(GetWrongList() == rCompare.GetWrongList())
        return true;

    if(!GetWrongList() || !rCompare.GetWrongList())
        return false;

    return (*GetWrongList() == *rCompare.GetWrongList());
}

bool ContentInfo::operator==( const ContentInfo& rCompare ) const
{
	if( (aText == rCompare.aText) && 
			(aStyle == rCompare.aStyle ) &&
			(aAttribs.Count() == rCompare.aAttribs.Count() ) &&
			(eFamily == rCompare.eFamily ) &&
			(aParaAttribs == rCompare.aParaAttribs ) )
	{
		const USHORT nCount = aAttribs.Count();
		if( nCount == rCompare.aAttribs.Count() )
		{
			USHORT n;
			for( n = 0; n < nCount; n++ )
			{
				if( !(*aAttribs.GetObject(n) == *rCompare.aAttribs.GetObject(n)) )
					return false;
			}

			return true;
		}
	}

	return false;
}

EditTextObject::EditTextObject( USHORT n)
{
	DBG_CTOR( EE_EditTextObject, 0 );
	nWhich = n;
}

EditTextObject::EditTextObject( const EditTextObject& r )
{
	DBG_CTOR( EE_EditTextObject, 0 );
	nWhich = r.nWhich;
}

__EXPORT EditTextObject::~EditTextObject()
{
	DBG_DTOR( EE_EditTextObject, 0 );
}

USHORT EditTextObject::GetParagraphCount() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

XubString EditTextObject::GetText( USHORT /* nParagraph */ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return XubString();
}

void EditTextObject::Insert( const EditTextObject& /* rObj */, USHORT /* nPara */)
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

EditTextObject* EditTextObject::CreateTextObject( USHORT /*nPara*/, USHORT /*nParas*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

void EditTextObject::RemoveParagraph( USHORT /*nPara*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL EditTextObject::HasPortionInfo() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

void EditTextObject::ClearPortionInfo()
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL EditTextObject::HasOnlineSpellErrors() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

BOOL EditTextObject::HasCharAttribs( USHORT ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

void EditTextObject::GetCharAttribs( USHORT /*nPara*/, EECharAttribArray& /*rLst*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

void EditTextObject::MergeParaAttribs( const SfxItemSet& /*rAttribs*/, USHORT /*nStart*/, USHORT /*nEnd*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL EditTextObject::IsFieldObject() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

const SvxFieldItem* EditTextObject::GetField() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

BOOL EditTextObject::HasField( TypeId /*aType*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

SfxItemSet EditTextObject::GetParaAttribs( USHORT /*nPara*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return SfxItemSet( *(SfxItemPool*)NULL );
}

void EditTextObject::SetParaAttribs( USHORT /*nPara*/, const SfxItemSet& /*rAttribs*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL EditTextObject::RemoveCharAttribs( USHORT /*nWhich*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

BOOL EditTextObject::RemoveParaAttribs( USHORT /*nWhich*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

BOOL EditTextObject::HasStyleSheet( const XubString& /*rName*/, SfxStyleFamily /*eFamily*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

void EditTextObject::GetStyleSheet( USHORT /*nPara*/, XubString& /*rName*/, SfxStyleFamily& /*eFamily*/ ) const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

void EditTextObject::SetStyleSheet( USHORT /*nPara*/, const XubString& /*rName*/, const SfxStyleFamily& /*eFamily*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL __EXPORT EditTextObject::ChangeStyleSheets( const XubString&, SfxStyleFamily,
											const XubString&, SfxStyleFamily )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

void __EXPORT EditTextObject::ChangeStyleSheetName( SfxStyleFamily /*eFamily*/,
				const XubString& /*rOldName*/, const XubString& /*rNewName*/ )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

USHORT EditTextObject::GetUserType() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

void EditTextObject::SetUserType( USHORT )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

ULONG EditTextObject::GetObjectSettings() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

void EditTextObject::SetObjectSettings( ULONG )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
}

BOOL EditTextObject::IsVertical() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return FALSE;
}

void EditTextObject::SetVertical( BOOL bVertical )
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	((BinTextObject*)this)->SetVertical( bVertical );
}

USHORT EditTextObject::GetScriptType() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return ((const BinTextObject*)this)->GetScriptType();
}


BOOL EditTextObject::Store( SvStream& rOStream ) const
{
	if ( rOStream.GetError() )
		return FALSE;

	// Vorspann:
	sal_Size nStartPos = rOStream.Tell();

	rOStream << (USHORT)Which();

	sal_uInt32 nStructSz = 0;
	rOStream << nStructSz;

	// Eigene Daten:
	StoreData( rOStream );

	// Nachspann:
	sal_Size nEndPos = rOStream.Tell();
	nStructSz = nEndPos - nStartPos - sizeof( nWhich ) - sizeof( nStructSz );
	rOStream.Seek( nStartPos + sizeof( nWhich ) );
	rOStream << nStructSz;
	rOStream.Seek( nEndPos );

	return rOStream.GetError() ? FALSE : TRUE;
}

EditTextObject*	EditTextObject::Create( SvStream& rIStream, SfxItemPool* pGlobalTextObjectPool )
{
	ULONG nStartPos = rIStream.Tell();

	// Ertmal sehen, was fuer ein Object...
	USHORT nWhich;
	rIStream >> nWhich;

	sal_uInt32 nStructSz;
	rIStream >> nStructSz;

	DBG_ASSERT( ( nWhich == 0x22 /*EE_FORMAT_BIN300*/ ) || ( nWhich == EE_FORMAT_BIN ), "CreateTextObject: Unbekanntes Objekt!" );

	if ( rIStream.GetError() )
		return NULL;

	EditTextObject* pTxtObj = NULL;
	switch ( nWhich )
	{
		case 0x22 /*BIN300*/:		pTxtObj = new BinTextObject( 0 );
									((BinTextObject*)pTxtObj)->CreateData300( rIStream );
									break;
		case EE_FORMAT_BIN:			pTxtObj = new BinTextObject( pGlobalTextObjectPool );
									pTxtObj->CreateData( rIStream );
									break;
		default:
		{
			// Wenn ich das Format nicht kenne, ueberlese ich den Inhalt:
			rIStream.SetError( EE_READWRITE_WRONGFORMAT );
		}
	}

	// Sicherstellen, dass der Stream an der richtigen Stelle hinterlassen wird.
	sal_Size nFullSz = sizeof( nWhich ) + sizeof( nStructSz ) + nStructSz;
	rIStream.Seek( nStartPos + nFullSz );
	return pTxtObj;
}

void EditTextObject::Skip( SvStream& rIStream )
{
	sal_Size nStartPos = rIStream.Tell();

	USHORT _nWhich;
	rIStream >> _nWhich;

	sal_uInt32 nStructSz;
	rIStream >> nStructSz;

	sal_Size nFullSz = sizeof( _nWhich ) + sizeof( nStructSz ) + nStructSz;
	rIStream.Seek( nStartPos + nFullSz );
}

void __EXPORT EditTextObject::StoreData( SvStream& ) const
{
	DBG_ERROR( "StoreData: Basisklasse!" );
}

void __EXPORT EditTextObject::CreateData( SvStream& )
{
	DBG_ERROR( "CreateData: Basisklasse!" );
}

USHORT EditTextObject::GetVersion() const
{
	DBG_ERROR( "V-Methode direkt vom EditTextObject!" );
	return 0;
}

bool EditTextObject::operator==( const EditTextObject& rCompare ) const
{
	return static_cast< const BinTextObject* >( this )->operator==( static_cast< const BinTextObject& >( rCompare ) );
}

// #i102062#
bool EditTextObject::isWrongListEqual(const EditTextObject& rCompare) const
{
	return static_cast< const BinTextObject* >(this)->isWrongListEqual(static_cast< const BinTextObject& >(rCompare));
}

// from SfxItemPoolUser
void BinTextObject::ObjectInDestruction(const SfxItemPool& rSfxItemPool)
{
    if(!bOwnerOfPool && pPool && pPool == &rSfxItemPool)
    {
        // The pool we are based on gets destructed; get owner of pool by creating own one.
        // No need to call RemoveSfxItemPoolUser(), this is done from the pool's destructor
        // Base new pool on EditEnginePool; it would also be possible to clone the used
        // pool if needed, but only text attributes should be used.
		SfxItemPool* pNewPool = EditEngine::CreatePool();

	    if(pPool)
        {
		    pNewPool->SetDefaultMetric(pPool->GetMetric(DEF_METRIC));
        }

	    for(sal_uInt16 n(0); n < aContents.Count(); n++)
	    {
            // clone ContentInfos for new pool
		    ContentInfo* pOrg = aContents.GetObject(n);
		    DBG_ASSERT(pOrg, "NULL-Pointer in ContentList!");

            ContentInfo* pNew = new ContentInfo(*pOrg, *pNewPool);
		    aContents.Replace(pNew, n);
            delete pOrg;
	    }
        
        // set local variables
        pPool = pNewPool;
        bOwnerOfPool = TRUE;
    }
}

EditEngineItemPool* getEditEngineItemPool(SfxItemPool* pPool)
{
	EditEngineItemPool* pRetval = dynamic_cast< EditEngineItemPool* >(pPool);

	while(!pRetval && pPool && pPool->GetSecondaryPool())
	{
		pPool = pPool->GetSecondaryPool();

		if(pPool)
		{
			pRetval = dynamic_cast< EditEngineItemPool* >(pPool);
		}
	}

	return pRetval;
}

BinTextObject::BinTextObject( SfxItemPool* pP ) :
	EditTextObject( EE_FORMAT_BIN ),
    SfxItemPoolUser()
{
	nVersion = 0;
	nMetric = 0xFFFF;
	nUserType = 0;
	nObjSettings = 0;
	pPortionInfo = 0;
	
	// #i101239# ensure target is a EditEngineItemPool, else
	// fallback to pool ownership. This is needed to ensure that at
	// pool destruction time of an alien pool, the pool is still alive.
	// When registering would happen at an alien pool which just uses an
	// EditEngineItemPool as some sub-pool, that pool could already
	// be decoupled and deleted whcih would lead to crashes.
	pPool =	getEditEngineItemPool(pP);
	
	if ( pPool )
	{
		bOwnerOfPool = FALSE;
	}
	else
	{
		pPool =	EditEngine::CreatePool();
		bOwnerOfPool =	TRUE;
	}

    if(!bOwnerOfPool && pPool)
    {
		// it is sure now that the pool is an EditEngineItemPool
        pPool->AddSfxItemPoolUser(*this);
    }

	bVertical = FALSE;
	bStoreUnicodeStrings = FALSE;
	nScriptType = 0;
}

BinTextObject::BinTextObject( const BinTextObject& r ) :
	EditTextObject( r ),
    SfxItemPoolUser()
{
	nVersion = r.nVersion;
	nMetric = r.nMetric;
	nUserType = r.nUserType;
	nObjSettings = r.nObjSettings;
	bVertical = r.bVertical;
	nScriptType = r.nScriptType;
	pPortionInfo = NULL;	// PortionInfo nicht kopieren
	bStoreUnicodeStrings = FALSE;

	if ( !r.bOwnerOfPool )
	{
		// reuse alien pool; this must be a EditEngineItemPool
		// since there is no other way to construct a BinTextObject
		// than it's regular constructor where that is ensured
		pPool =	r.pPool;
		bOwnerOfPool = FALSE;
	}
	else
	{
		pPool =	EditEngine::CreatePool();
		bOwnerOfPool =	TRUE;

	}

    if(!bOwnerOfPool && pPool)
    {
		// it is sure now that the pool is an EditEngineItemPool
        pPool->AddSfxItemPoolUser(*this);
    }

    if ( bOwnerOfPool && pPool && r.pPool )
		pPool->SetDefaultMetric( r.pPool->GetMetric( DEF_METRIC ) );

	for ( USHORT n = 0; n < r.aContents.Count(); n++ )
	{
		ContentInfo* pOrg = r.aContents.GetObject( n );
		DBG_ASSERT( pOrg, "NULL-Pointer in ContentList!" );
		ContentInfo* pNew = new ContentInfo( *pOrg, *pPool );
		aContents.Insert( pNew, aContents.Count() );
	}
}

__EXPORT BinTextObject::~BinTextObject()
{
    if(!bOwnerOfPool && pPool)
    {
        pPool->RemoveSfxItemPoolUser(*this);
    }

    ClearPortionInfo();
	DeleteContents();
	if ( bOwnerOfPool )
	{
		// Nicht mehr, wegen 1xDefItems.
		// siehe auch ~EditDoc().
//		pPool->ReleaseDefaults( TRUE /* bDelete */ );
        SfxItemPool::Free(pPool);
	}
}

USHORT BinTextObject::GetUserType() const
{
	return nUserType;
}

void BinTextObject::SetUserType( USHORT n )
{
	nUserType = n;
}

ULONG BinTextObject::GetObjectSettings() const
{
	return nObjSettings;
}

void BinTextObject::SetObjectSettings( ULONG n )
{
	nObjSettings = n;
}

BOOL BinTextObject::IsVertical() const
{
	return bVertical;
}

void BinTextObject::SetVertical( BOOL b )
{
	if ( b != bVertical )
	{
		bVertical = b;
		ClearPortionInfo();
	}
}

USHORT BinTextObject::GetScriptType() const
{
	return nScriptType;
}

void BinTextObject::SetScriptType( USHORT nType )
{
	nScriptType = nType;
}


void BinTextObject::DeleteContents()
{
	for ( USHORT n = 0; n < aContents.Count(); n++ )
	{
		ContentInfo* p = aContents.GetObject( n );
		DBG_ASSERT( p, "NULL-Pointer in ContentList!" );
		delete p;
	}
	aContents.Remove( 0, aContents.Count() );
}

EditTextObject*	__EXPORT BinTextObject::Clone() const
{
	return new BinTextObject( *this );
}

XEditAttribute* BinTextObject::CreateAttrib( const SfxPoolItem& rItem, USHORT nStart, USHORT nEnd )
{
	return MakeXEditAttribute( *pPool, rItem, nStart, nEnd );
}

void BinTextObject::DestroyAttrib( XEditAttribute* pAttr )
{
	pPool->Remove( *pAttr->GetItem() );
	delete pAttr;
}

ContentInfo* BinTextObject::CreateAndInsertContent()
{
	ContentInfo* pC = new ContentInfo( *pPool );
	aContents.Insert( pC, aContents.Count() );
	return pC;
}

USHORT BinTextObject::GetParagraphCount() const
{
	return aContents.Count();
}

XubString BinTextObject::GetText( USHORT nPara ) const
{
	DBG_ASSERT( nPara < aContents.Count(), "BinTextObject::GetText: Absatz existiert nicht!" );
	if ( nPara < aContents.Count() )
	{
		ContentInfo* pC = aContents[ nPara ];
		return pC->GetText();
	}
	return XubString();
}

void BinTextObject::Insert( const EditTextObject& rObj, USHORT nDestPara )
{
	DBG_ASSERT( rObj.Which() == EE_FORMAT_BIN, "UTO: Unbekanntes Textobjekt" );

	const BinTextObject& rBinObj = (const BinTextObject&)rObj;

	if ( nDestPara > aContents.Count() )
		nDestPara = aContents.Count();

	const USHORT nParas = rBinObj.GetContents().Count();
	for ( USHORT nP = 0; nP < nParas; nP++ )
	{
		ContentInfo* pC = rBinObj.GetContents()[ nP ];
		ContentInfo* pNew = new ContentInfo( *pC, *GetPool() );
		aContents.Insert( pNew, nDestPara+nP );
	}
	ClearPortionInfo();
}

EditTextObject* BinTextObject::CreateTextObject( USHORT nPara, USHORT nParas ) const
{
	if ( ( nPara >= aContents.Count() ) || !nParas )
		return NULL;

	// Pool nur teilen, wenn von aussen eingestellter Pool.
	BinTextObject* pObj = new BinTextObject( bOwnerOfPool ? 0 : pPool );
	if ( bOwnerOfPool && pPool )
		pObj->GetPool()->SetDefaultMetric( pPool->GetMetric( DEF_METRIC ) );

	// If complete text is only one ScriptType, this is valid.
	// If text contains different ScriptTypes, this shouldn't be a problem...
	pObj->nScriptType = nScriptType;

	const USHORT nEndPara = nPara+nParas-1;
	for ( USHORT nP = nPara; nP <= nEndPara; nP++ )
	{
		ContentInfo* pC = aContents[ nP ];
		ContentInfo* pNew = new ContentInfo( *pC, *pObj->GetPool() );
		pObj->GetContents().Insert( pNew, pObj->GetContents().Count() );
	}
	return pObj;
}

void BinTextObject::RemoveParagraph( USHORT nPara )
{
	DBG_ASSERT( nPara < aContents.Count(), "BinTextObject::GetText: Absatz existiert nicht!" );
	if ( nPara < aContents.Count() )
	{
		ContentInfo* pC = aContents[ nPara ];
		aContents.Remove( nPara );
		delete pC;
		ClearPortionInfo();
	}
}

BOOL BinTextObject::HasPortionInfo() const
{
	return pPortionInfo ? TRUE : FALSE;
}

void BinTextObject::ClearPortionInfo()
{
	if ( pPortionInfo )
	{
		for ( USHORT n = pPortionInfo->Count(); n; )
			delete pPortionInfo->GetObject( --n );
		delete pPortionInfo;
		pPortionInfo = NULL;
	}
}

BOOL BinTextObject::HasOnlineSpellErrors() const
{
#ifndef SVX_LIGHT
	for ( USHORT n = 0; n < aContents.Count(); n++ )
	{
		ContentInfo* p = aContents.GetObject( n );
		if ( p->GetWrongList() && p->GetWrongList()->Count() )
			return TRUE;
	}
#endif // !SVX_LIGHT
	return FALSE;

}

BOOL BinTextObject::HasCharAttribs( USHORT _nWhich ) const
{
	for ( USHORT nPara = GetContents().Count(); nPara; )
	{
		ContentInfo* pC = GetContents().GetObject( --nPara );

		USHORT nAttribs = pC->GetAttribs().Count();
		if ( nAttribs && !_nWhich )
			return TRUE;

		for ( USHORT nAttr = nAttribs; nAttr; )
		{
			XEditAttribute* pX = pC->GetAttribs().GetObject( --nAttr );
			if ( pX->GetItem()->Which() == _nWhich )
				return TRUE;
		}
	}
	return FALSE;
}

void BinTextObject::GetCharAttribs( USHORT nPara, EECharAttribArray& rLst ) const
{
	rLst.Remove( 0, rLst.Count() );
	ContentInfo* pC = GetContents().GetObject( nPara );
	if ( pC )
	{
		for ( USHORT nAttr = 0; nAttr < pC->GetAttribs().Count(); nAttr++ )
		{
			XEditAttribute* pAttr = pC->GetAttribs().GetObject( nAttr );
			EECharAttrib aEEAttr;
			aEEAttr.pAttr = pAttr->GetItem();
			aEEAttr.nPara = nPara;
			aEEAttr.nStart = pAttr->GetStart();
			aEEAttr.nEnd = pAttr->GetEnd();
			rLst.Insert( aEEAttr, rLst.Count() );
		}
	}
}

void BinTextObject::MergeParaAttribs( const SfxItemSet& rAttribs, USHORT nStart, USHORT nEnd )
{
	BOOL bChanged = FALSE;

	for ( USHORT nPara = GetContents().Count(); nPara; )
	{
		ContentInfo* pC = GetContents().GetObject( --nPara );

		for ( USHORT nW = nStart; nW <= nEnd; nW++ )
		{
			if ( ( pC->GetParaAttribs().GetItemState( nW, FALSE ) != SFX_ITEM_ON )
					&& ( rAttribs.GetItemState( nW, FALSE ) == SFX_ITEM_ON ) )
			{
				pC->GetParaAttribs().Put( rAttribs.Get( nW ) );
				bChanged = TRUE;
			}
		}
	}

	if ( bChanged )
		ClearPortionInfo();
}

BOOL BinTextObject::IsFieldObject() const
{
	return BinTextObject::GetField() ? TRUE : FALSE;
}

const SvxFieldItem* BinTextObject::GetField() const
{
	if ( GetContents().Count() == 1 )
	{
		ContentInfo* pC = GetContents()[0];
		if ( pC->GetText().Len() == 1 )
		{
			USHORT nAttribs = pC->GetAttribs().Count();
			for ( USHORT nAttr = nAttribs; nAttr; )
			{
				XEditAttribute* pX = pC->GetAttribs().GetObject( --nAttr );
				if ( pX->GetItem()->Which() == EE_FEATURE_FIELD )
					return (const SvxFieldItem*)pX->GetItem();
			}
		}
	}
	return 0;
}

BOOL BinTextObject::HasField( TypeId aType ) const
{
	USHORT nParagraphs = GetContents().Count();
	for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = GetContents().GetObject( nPara );
		USHORT nAttrs = pC->GetAttribs().Count();
		for ( USHORT nAttr = 0; nAttr < nAttrs; nAttr++ )
		{
			XEditAttribute* pAttr = pC->GetAttribs()[nAttr];
			if ( pAttr->GetItem()->Which() == EE_FEATURE_FIELD )
			{
				if ( !aType )
					return TRUE;

				const SvxFieldData* pFldData = ((const SvxFieldItem*)pAttr->GetItem())->GetField();
				if ( pFldData && pFldData->IsA( aType ) )
					return TRUE;
			}
		}
	}
	return FALSE;
}

SfxItemSet BinTextObject::GetParaAttribs( USHORT nPara ) const
{
	ContentInfo* pC = GetContents().GetObject( nPara );
	return pC->GetParaAttribs();
}

void BinTextObject::SetParaAttribs( USHORT nPara, const SfxItemSet& rAttribs )
{
	ContentInfo* pC = GetContents().GetObject( nPara );
	pC->GetParaAttribs().Set( rAttribs );
	ClearPortionInfo();
}

BOOL BinTextObject::RemoveCharAttribs( USHORT _nWhich )
{
	BOOL bChanged = FALSE;

	for ( USHORT nPara = GetContents().Count(); nPara; )
	{
		ContentInfo* pC = GetContents().GetObject( --nPara );

		for ( USHORT nAttr = pC->GetAttribs().Count(); nAttr; )
		{
			XEditAttribute* pAttr = pC->GetAttribs().GetObject( --nAttr );
			if ( !_nWhich || ( pAttr->GetItem()->Which() == _nWhich ) )
			{
				pC->GetAttribs().Remove( nAttr );
				DestroyAttrib( pAttr );
				bChanged = TRUE;
			}
		}
	}

	if ( bChanged )
		ClearPortionInfo();

	return bChanged;
}

BOOL BinTextObject::RemoveParaAttribs( USHORT _nWhich )
{
	BOOL bChanged = FALSE;

	for ( USHORT nPara = GetContents().Count(); nPara; )
	{
		ContentInfo* pC = GetContents().GetObject( --nPara );

		if ( !_nWhich )
		{
			if( pC->GetParaAttribs().Count() )
				bChanged = TRUE;
			pC->GetParaAttribs().ClearItem();
		}
		else
		{
			if ( pC->GetParaAttribs().GetItemState( _nWhich ) == SFX_ITEM_ON )
			{
				pC->GetParaAttribs().ClearItem( _nWhich );
				bChanged = TRUE;
			}
		}
	}

	if ( bChanged )
		ClearPortionInfo();

	return bChanged;
}

BOOL BinTextObject::HasStyleSheet( const XubString& rName, SfxStyleFamily eFamily ) const
{
	USHORT nParagraphs = GetContents().Count();
	for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = GetContents().GetObject( nPara );
		if ( ( pC->GetFamily() == eFamily ) && ( pC->GetStyle() == rName ) )
			return TRUE;
	}
	return FALSE;
}

void BinTextObject::GetStyleSheet( USHORT nPara, XubString& rName, SfxStyleFamily& rFamily ) const
{
	if ( nPara < aContents.Count() )
	{
		ContentInfo* pC = aContents[ nPara ];
		rName = pC->GetStyle();
		rFamily = pC->GetFamily();
	}
}

void BinTextObject::SetStyleSheet( USHORT nPara, const XubString& rName, const SfxStyleFamily& rFamily )
{
	if ( nPara < aContents.Count() )
	{
		ContentInfo* pC = aContents[ nPara ];
		pC->GetStyle() = rName;
		pC->GetFamily() = rFamily;
	}
}

BOOL BinTextObject::ImpChangeStyleSheets(
					const XubString& rOldName, SfxStyleFamily eOldFamily,
					const XubString& rNewName, SfxStyleFamily eNewFamily )
{
	const USHORT nParagraphs = GetContents().Count();
	BOOL bChanges = FALSE;

	for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = GetContents().GetObject( nPara );
		if ( pC->GetFamily() == eOldFamily )
		{
			if ( pC->GetStyle() == rOldName )
			{
				pC->GetStyle() = rNewName;
				pC->GetFamily() = eNewFamily;
				bChanges = TRUE;
			}
		}
	}
	return bChanges;
}

BOOL __EXPORT BinTextObject::ChangeStyleSheets(
					const XubString& rOldName, SfxStyleFamily eOldFamily,
					const XubString& rNewName, SfxStyleFamily eNewFamily )
{
	BOOL bChanges = ImpChangeStyleSheets( rOldName, eOldFamily, rNewName, eNewFamily );
	if ( bChanges )
		ClearPortionInfo();

	return bChanges;
}

void __EXPORT BinTextObject::ChangeStyleSheetName( SfxStyleFamily eFamily,
				const XubString& rOldName, const XubString& rNewName )
{
	ImpChangeStyleSheets( rOldName, eFamily, rNewName, eFamily );
}

void __EXPORT BinTextObject::StoreData( SvStream& rOStream ) const
{
	USHORT nVer = 602;
	rOStream << nVer;

	rOStream << bOwnerOfPool;

	// Erst den Pool speichern, spaeter nur noch Surregate
	if ( bOwnerOfPool )
	{
		GetPool()->SetFileFormatVersion( SOFFICE_FILEFORMAT_50 );
		GetPool()->Store( rOStream );
	}

	// Aktuelle Zeichensatz speichern...
	// #90477# GetSOStoreTextEncoding: Bug in 5.2, when default char set is multi byte text encoding
	rtl_TextEncoding eEncoding = GetSOStoreTextEncoding( gsl_getSystemTextEncoding(), (USHORT) rOStream.GetVersion() );
	rOStream << (USHORT) eEncoding;

	// Die Anzahl der Absaetze...
	USHORT nParagraphs = GetContents().Count();
	rOStream << nParagraphs;

	char cFeatureConverted = ByteString( CH_FEATURE, eEncoding ).GetChar(0);

	// Die einzelnen Absaetze...
	for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = GetContents().GetObject( nPara );

		// Text...
		ByteString aText( pC->GetText(), eEncoding );

        // Symbols?
        BOOL bSymbolPara = FALSE;
		if ( pC->GetParaAttribs().GetItemState( EE_CHAR_FONTINFO ) == SFX_ITEM_ON )
        {
            const SvxFontItem& rFontItem = (const SvxFontItem&)pC->GetParaAttribs().Get( EE_CHAR_FONTINFO );
            if ( rFontItem.GetCharSet() == RTL_TEXTENCODING_SYMBOL )
            {
		        aText = ByteString( pC->GetText(), RTL_TEXTENCODING_SYMBOL );
                bSymbolPara = TRUE;
            }
        }
		for ( USHORT nA = 0; nA < pC->GetAttribs().Count(); nA++ )
		{
			XEditAttribute* pAttr = pC->GetAttribs().GetObject( nA );

			if ( pAttr->GetItem()->Which() == EE_CHAR_FONTINFO )
			{
                const SvxFontItem& rFontItem = (const SvxFontItem&)*pAttr->GetItem();
                if ( ( !bSymbolPara && ( rFontItem.GetCharSet() == RTL_TEXTENCODING_SYMBOL ) )
                      || ( bSymbolPara && ( rFontItem.GetCharSet() != RTL_TEXTENCODING_SYMBOL ) ) )
                {
                    // Not correctly converted
                    String aPart( pC->GetText(), pAttr->GetStart(), pAttr->GetEnd() - pAttr->GetStart() );
                    ByteString aNew( aPart, rFontItem.GetCharSet() );
                    aText.Erase( pAttr->GetStart(), pAttr->GetEnd() - pAttr->GetStart() );
                    aText.Insert( aNew, pAttr->GetStart() );
                }

                // #88414# Convert StarSymbol back to StarBats
                FontToSubsFontConverter hConv = CreateFontToSubsFontConverter( rFontItem.GetFamilyName(), FONTTOSUBSFONT_EXPORT | FONTTOSUBSFONT_ONLYOLDSOSYMBOLFONTS );
                if ( hConv )
                {
                    // Don't create a new Attrib with StarBats font, MBR changed the
                    // SvxFontItem::Store() to store StarBats instead of StarSymbol!
                    for ( USHORT nChar = pAttr->GetStart(); nChar < pAttr->GetEnd(); nChar++ )
                    {
                        sal_Unicode cOld = pC->GetText().GetChar( nChar );
                        char cConv = ByteString::ConvertFromUnicode( ConvertFontToSubsFontChar( hConv, cOld ), RTL_TEXTENCODING_SYMBOL );
                        if ( cConv )
                            aText.SetChar( nChar, cConv );
                    }

                    DestroyFontToSubsFontConverter( hConv );
                }
			}
		}

        // #88414# Convert StarSymbol back to StarBats
        // StarSymbol as paragraph attribute or in StyleSheet?

        FontToSubsFontConverter hConv = NULL;
        if ( pC->GetParaAttribs().GetItemState( EE_CHAR_FONTINFO ) == SFX_ITEM_ON )
        {
            hConv = CreateFontToSubsFontConverter( ((const SvxFontItem&)pC->GetParaAttribs().Get( EE_CHAR_FONTINFO )).GetFamilyName(), FONTTOSUBSFONT_EXPORT | FONTTOSUBSFONT_ONLYOLDSOSYMBOLFONTS );
        }
/* cl removed because not needed anymore since binfilter

        else if ( pC->GetStyle().Len() && pC->GetLoadStoreTempInfos() )
        {
            hConv = pC->GetLoadStoreTempInfos()->hOldSymbolConv_Store;
        }
*/
        if ( hConv )
        {
            for ( USHORT nChar = 0; nChar < pC->GetText().Len(); nChar++ )
            {
                if ( !pC->GetAttribs().FindAttrib( EE_CHAR_FONTINFO, nChar ) )
                {
                    sal_Unicode cOld = pC->GetText().GetChar( nChar );
                    char cConv = ByteString::ConvertFromUnicode( ConvertFontToSubsFontChar( hConv, cOld ), RTL_TEXTENCODING_SYMBOL );
                    if ( cConv )
                        aText.SetChar( nChar, cConv );
                }
            }

            DestroyFontToSubsFontConverter( hConv );

        }


		// Convert CH_FEATURE to CH_FEATURE_OLD
		aText.SearchAndReplaceAll( cFeatureConverted, CH_FEATURE_OLD );
		rOStream.WriteByteString( aText );

		// StyleName und Family...
		rOStream.WriteByteString( ByteString( pC->GetStyle(), eEncoding ) );
		rOStream << (USHORT)pC->GetFamily();

		// Absatzattribute...
		pC->GetParaAttribs().Store( rOStream );

		// Die Anzahl der Attribute...
		USHORT nAttribs = pC->GetAttribs().Count();
		rOStream << nAttribs;

		// Und die einzelnen Attribute
		// Items als Surregate => immer 8 Byte pro Attrib
		// Which = 2; Surregat = 2; Start = 2; End = 2;
		for ( USHORT nAttr = 0; nAttr < nAttribs; nAttr++ )
		{
			XEditAttribute* pX = pC->GetAttribs().GetObject( nAttr );

			rOStream << pX->GetItem()->Which();
			GetPool()->StoreSurrogate( rOStream, pX->GetItem() );
			rOStream << pX->GetStart();
			rOStream << pX->GetEnd();
		}
	}

	// Ab 400:
	rOStream << nMetric;

	// Ab 600
	rOStream << nUserType;
	rOStream << nObjSettings;

	// Ab 601
	rOStream << bVertical;

	// Ab 602
	rOStream << nScriptType;

	rOStream << bStoreUnicodeStrings;
	if ( bStoreUnicodeStrings )
	{
		for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
		{
			ContentInfo* pC = GetContents().GetObject( nPara );
			USHORT nL = pC->GetText().Len();
			rOStream << nL;
			rOStream.Write( pC->GetText().GetBuffer(), nL*sizeof(sal_Unicode) );

            // #91575# StyleSheetName must be Unicode too!
            // Copy/Paste from EA3 to BETA or from BETA to EA3 not possible, not needed...
            // If needed, change nL back to ULONG and increase version...
            nL = pC->GetStyle().Len();
			rOStream << nL;
			rOStream.Write( pC->GetStyle().GetBuffer(), nL*sizeof(sal_Unicode) );
		}
	}
}

void __EXPORT BinTextObject::CreateData( SvStream& rIStream )
{
	rIStream >> nVersion;

	// Das Textobject wurde erstmal mit der aktuellen Einstellung
	// von pTextObjectPool erzeugt.
	BOOL bOwnerOfCurrent = bOwnerOfPool;
	rIStream >> bOwnerOfPool;

	if ( bOwnerOfCurrent && !bOwnerOfPool )
	{
		// Es wurde ein globaler Pool verwendet, mir jetzt nicht uebergeben,
		// aber ich brauche ihn!
		DBG_ERROR( "Man gebe mir den globalen TextObjectPool!" );
		return;
	}
	else if ( !bOwnerOfCurrent && bOwnerOfPool )
	{
		// Es soll ein globaler Pool verwendet werden, aber dieses
		// Textobject hat einen eigenen.
		pPool =	EditEngine::CreatePool();
	}

	if ( bOwnerOfPool )
		GetPool()->Load( rIStream );

	// CharSet, in dem gespeichert wurde:
	USHORT nCharSet;
	rIStream >> nCharSet;

	rtl_TextEncoding eSrcEncoding = GetSOLoadTextEncoding( (rtl_TextEncoding)nCharSet, (USHORT)rIStream.GetVersion() );

	// Die Anzahl der Absaetze...
	USHORT nParagraphs;
	rIStream >> nParagraphs;

	// Die einzelnen Absaetze...
	for ( ULONG nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = CreateAndInsertContent();

		// Der Text...
		ByteString aByteString;
		rIStream.ReadByteString( aByteString );
		pC->GetText() = String( aByteString, eSrcEncoding );

		// StyleName und Family...
		rIStream.ReadByteString( pC->GetStyle(), eSrcEncoding );
		USHORT nStyleFamily;
		rIStream >> nStyleFamily;
		pC->GetFamily() = (SfxStyleFamily)nStyleFamily;

		// Absatzattribute...
		pC->GetParaAttribs().Load( rIStream );

		// Die Anzahl der Attribute...
		USHORT nAttribs;
		rIStream >> nAttribs;

		// Und die einzelnen Attribute
		// Items als Surregate => immer 8 Byte pro Attrib
		// Which = 2; Surregat = 2; Start = 2; End = 2;
        USHORT nAttr;
		for ( nAttr = 0; nAttr < nAttribs; nAttr++ )
		{
			USHORT _nWhich, nStart, nEnd;
			const SfxPoolItem* pItem;

			rIStream >> _nWhich;
			_nWhich = pPool->GetNewWhich( _nWhich );
			pItem = pPool->LoadSurrogate( rIStream, _nWhich, 0 );
			rIStream >> nStart;
			rIStream >> nEnd;
			if ( pItem )
			{
				if ( pItem->Which() == EE_FEATURE_NOTCONV )
				{
					pC->GetText().SetChar( nStart, ByteString::ConvertToUnicode( aByteString.GetChar( nStart ), ((SvxCharSetColorItem*)pItem)->GetCharSet() ) );
				}
				else
				{
					XEditAttribute* pAttr = new XEditAttribute( *pItem, nStart, nEnd );
					pC->GetAttribs().Insert( pAttr, pC->GetAttribs().Count() );

					if ( ( _nWhich >= EE_FEATURE_START ) && ( _nWhich <= EE_FEATURE_END ) )
					{
						// Convert CH_FEATURE to CH_FEATURE_OLD
						DBG_ASSERT( (BYTE) aByteString.GetChar( nStart ) == CH_FEATURE_OLD, "CreateData: CH_FEATURE expected!" );
						if ( (BYTE) aByteString.GetChar( nStart ) == CH_FEATURE_OLD )
							pC->GetText().SetChar( nStart, CH_FEATURE );
					}
				}
			}
		}

        // But check for paragraph and character symbol attribs here,
        // FinishLoad will not be called in OpenOffice Calc, no StyleSheets...

        BOOL bSymbolPara = FALSE;
        if ( pC->GetParaAttribs().GetItemState( EE_CHAR_FONTINFO ) == SFX_ITEM_ON )
        {
            const SvxFontItem& rFontItem = (const SvxFontItem&)pC->GetParaAttribs().Get( EE_CHAR_FONTINFO );
            if ( rFontItem.GetCharSet() == RTL_TEXTENCODING_SYMBOL )
            {
		        pC->GetText() = String( aByteString, RTL_TEXTENCODING_SYMBOL );
                bSymbolPara = TRUE;
            }
        }

		for ( nAttr = pC->GetAttribs().Count(); nAttr; )
		{
			XEditAttribute* pAttr = pC->GetAttribs().GetObject( --nAttr );
            if ( pAttr->GetItem()->Which() == EE_CHAR_FONTINFO )
            {
                const SvxFontItem& rFontItem = (const SvxFontItem&)*pAttr->GetItem();
                if ( ( !bSymbolPara && ( rFontItem.GetCharSet() == RTL_TEXTENCODING_SYMBOL ) )
                      || ( bSymbolPara && ( rFontItem.GetCharSet() != RTL_TEXTENCODING_SYMBOL ) ) )
                {
                    // Not correctly converted
                    ByteString aPart( aByteString, pAttr->GetStart(), pAttr->GetEnd()-pAttr->GetStart() );
                    String aNew( aPart, rFontItem.GetCharSet() );
                    pC->GetText().Erase( pAttr->GetStart(), pAttr->GetEnd()-pAttr->GetStart() );
                    pC->GetText().Insert( aNew, pAttr->GetStart() );
                }

                // #88414# Convert StarMath and StarBats to StarSymbol
                FontToSubsFontConverter hConv = CreateFontToSubsFontConverter( rFontItem.GetFamilyName(), FONTTOSUBSFONT_IMPORT | FONTTOSUBSFONT_ONLYOLDSOSYMBOLFONTS );
                if ( hConv )
                {
                    SvxFontItem aNewFontItem( rFontItem );
		            aNewFontItem.GetFamilyName() = GetFontToSubsFontName( hConv );

                    pC->GetAttribs().Remove( nAttr );
                    XEditAttribute* pNewAttr = CreateAttrib( aNewFontItem, pAttr->GetStart(), pAttr->GetEnd() );
                    pC->GetAttribs().Insert( pNewAttr, nAttr );
                    DestroyAttrib( pAttr );

                    for ( USHORT nChar = pNewAttr->GetStart(); nChar < pNewAttr->GetEnd(); nChar++ )
                    {
                        sal_Unicode cOld = pC->GetText().GetChar( nChar );
                        DBG_ASSERT( cOld >= 0xF000, "cOld not converted?!" );
                        sal_Unicode cConv = ConvertFontToSubsFontChar( hConv, cOld );
                        if ( cConv )
                            pC->GetText().SetChar( nChar, cConv );
                    }

                    DestroyFontToSubsFontConverter( hConv );
                }
            }
        }


        // #88414# Convert StarMath and StarBats to StarSymbol
        // Maybe old symbol font as paragraph attribute?
        if ( pC->GetParaAttribs().GetItemState( EE_CHAR_FONTINFO ) == SFX_ITEM_ON )
        {
            const SvxFontItem& rFontItem = (const SvxFontItem&)pC->GetParaAttribs().Get( EE_CHAR_FONTINFO );
            FontToSubsFontConverter hConv = CreateFontToSubsFontConverter( rFontItem.GetFamilyName(), FONTTOSUBSFONT_IMPORT | FONTTOSUBSFONT_ONLYOLDSOSYMBOLFONTS );
            if ( hConv )
            {
                SvxFontItem aNewFontItem( rFontItem );
		        aNewFontItem.GetFamilyName() = GetFontToSubsFontName( hConv );
                pC->GetParaAttribs().Put( aNewFontItem );

                for ( USHORT nChar = 0; nChar < pC->GetText().Len(); nChar++ )
                {
                    if ( !pC->GetAttribs().FindAttrib( EE_CHAR_FONTINFO, nChar ) )
                    {
                        sal_Unicode cOld = pC->GetText().GetChar( nChar );
                        DBG_ASSERT( cOld >= 0xF000, "cOld not converted?!" );
                        sal_Unicode cConv = ConvertFontToSubsFontChar( hConv, cOld );
                        if ( cConv )
                            pC->GetText().SetChar( nChar, cConv );
                    }
                }

                DestroyFontToSubsFontConverter( hConv );
            }
        }
    }

	// Ab 400 auch die DefMetric:
	if ( nVersion >= 400 )
	{
		USHORT nTmpMetric;
		rIStream >> nTmpMetric;
		if ( nVersion >= 401 )
		{
			// In der 400 gab es noch einen Bug bei Textobjekten mit eigenem
			// Pool, deshalb erst ab 401 auswerten.
			nMetric = nTmpMetric;
			if ( bOwnerOfPool && pPool && ( nMetric != 0xFFFF ) )
				pPool->SetDefaultMetric( (SfxMapUnit)nMetric );
		}
	}

	if ( nVersion >= 600 )
	{
		rIStream >> nUserType;
		rIStream >> nObjSettings;
	}

	if ( nVersion >= 601 )
	{
		rIStream >> bVertical;
	}

	if ( nVersion >= 602 )
	{
		rIStream >> nScriptType;

		BOOL bUnicodeStrings;
		rIStream >> bUnicodeStrings;
		if ( bUnicodeStrings )
		{
			for ( USHORT nPara = 0; nPara < nParagraphs; nPara++ )
			{
				ContentInfo* pC = GetContents().GetObject( nPara );
				USHORT nL;

                // Text
				rIStream >> nL;
                if ( nL )
                {
				    pC->GetText().AllocBuffer( nL );
				    rIStream.Read( pC->GetText().GetBufferAccess(), nL*sizeof(sal_Unicode) );
				    pC->GetText().ReleaseBufferAccess( (USHORT)nL );
                }

                // StyleSheetName
                rIStream >> nL;
                if ( nL )
                {
				    pC->GetStyle().AllocBuffer( nL );
				    rIStream.Read( pC->GetStyle().GetBufferAccess(), nL*sizeof(sal_Unicode) );
				    pC->GetStyle().ReleaseBufferAccess( (USHORT)nL );
                }
			}
		}
	}


	// Ab 500 werden die Tabs anders interpretiert: TabPos + LI, vorher nur TabPos.
	// Wirkt nur wenn auch Tab-Positionen eingestellt wurden, nicht beim DefTab.
	if ( nVersion < 500 )
	{
		for ( USHORT n = 0; n < aContents.Count(); n++ )
		{
			ContentInfo* pC = aContents.GetObject( n );
			const SvxLRSpaceItem& rLRSpace = (const SvxLRSpaceItem&) pC->GetParaAttribs().Get( EE_PARA_LRSPACE );
			if ( rLRSpace.GetTxtLeft() && ( pC->GetParaAttribs().GetItemState( EE_PARA_TABS ) == SFX_ITEM_ON ) )
			{
				const SvxTabStopItem& rTabs = (const SvxTabStopItem&) pC->GetParaAttribs().Get( EE_PARA_TABS );
				SvxTabStopItem aNewTabs( 0, 0, SVX_TAB_ADJUST_LEFT, EE_PARA_TABS );
				for ( USHORT t = 0; t < rTabs.Count(); t++ )
				{
					const SvxTabStop& rT = rTabs[ t ];
					aNewTabs.Insert( SvxTabStop( rT.GetTabPos() - rLRSpace.GetTxtLeft(),
								rT.GetAdjustment(), rT.GetDecimal(), rT.GetFill() ) );
				}
				pC->GetParaAttribs().Put( aNewTabs );
			}
		}
	}
}

USHORT BinTextObject::GetVersion() const
{
	return nVersion;
}

bool BinTextObject::operator==( const BinTextObject& rCompare ) const
{
	if( this == &rCompare )
		return true;

	if( ( aContents.Count() != rCompare.aContents.Count() ) ||
			( pPool != rCompare.pPool ) ||
			( nMetric != rCompare.nMetric ) ||
			( nUserType!= rCompare.nUserType ) ||
			( nScriptType != rCompare.nScriptType ) ||
			( bVertical != rCompare.bVertical ) )
		return false;

	USHORT n;
	for( n = 0; n < aContents.Count(); n++ )
	{
		if( !( *aContents.GetObject( n ) == *rCompare.aContents.GetObject( n ) ) )
			return false;
	}

	return true;
}

// #i102062#
bool BinTextObject::isWrongListEqual(const BinTextObject& rCompare) const
{
    if(GetContents().Count() != rCompare.GetContents().Count())
    {
        return false;
    }

    for(USHORT a(0); a < GetContents().Count(); a++)
    {
        const ContentInfo& rCandA(*GetContents().GetObject(a));
        const ContentInfo& rCandB(*rCompare.GetContents().GetObject(a));

        if(!rCandA.isWrongListEqual(rCandB))
        {
            return false;
        }
    }

    return true;
}

#define CHARSETMARKER	0x9999

void __EXPORT BinTextObject::CreateData300( SvStream& rIStream )
{
	// Fuer Aufwaertskompatibilitaet.

	// Erst den Pool laden...
	// Ist in der 300 immer gespeichert worden!
	GetPool()->Load( rIStream );

	// Die Anzahl der Absaetze...
	sal_uInt32 nParagraphs;
	rIStream >> nParagraphs;

	// Die einzelnen Absaetze...
	for ( ULONG nPara = 0; nPara < nParagraphs; nPara++ )
	{
		ContentInfo* pC = CreateAndInsertContent();

		// Der Text...
		rIStream.ReadByteString( pC->GetText() );

		// StyleName und Family...
		rIStream.ReadByteString( pC->GetStyle() );
		USHORT nStyleFamily;
		rIStream >> nStyleFamily;
		pC->GetFamily() = (SfxStyleFamily)nStyleFamily;

		// Absatzattribute...
		pC->GetParaAttribs().Load( rIStream );

		// Die Anzahl der Attribute...
		sal_uInt32 nAttribs;
		rIStream >> nAttribs;

		// Und die einzelnen Attribute
		// Items als Surregate => immer 8 Byte pro Attrib
		// Which = 2; Surregat = 2; Start = 2; End = 2;
		for ( ULONG nAttr = 0; nAttr < nAttribs; nAttr++ )
		{
			USHORT _nWhich, nStart, nEnd;
			const SfxPoolItem* pItem;

			rIStream >> _nWhich;
			_nWhich = pPool->GetNewWhich( _nWhich );
			pItem = pPool->LoadSurrogate( rIStream, _nWhich, 0 );
			rIStream >> nStart;
			rIStream >> nEnd;
			if ( pItem )
			{
				XEditAttribute* pAttr = new XEditAttribute( *pItem, nStart, nEnd );
				pC->GetAttribs().Insert( pAttr, pC->GetAttribs().Count() );
			}
		}
	}

	// Prueffen, ob ein Zeichensatz gespeichert wurde
	USHORT nCharSetMarker;
	rIStream >> nCharSetMarker;
	if ( nCharSetMarker == CHARSETMARKER )
	{
		USHORT nCharSet;
		rIStream >> nCharSet;
	}
}
