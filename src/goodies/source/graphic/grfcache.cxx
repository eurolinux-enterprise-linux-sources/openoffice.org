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
#include "precompiled_goodies.hxx"

#include <vos/timer.hxx>
#include <tools/debug.hxx>
#include <vcl/outdev.hxx>
#include <tools/poly.hxx>
#include "grfcache.hxx"

#include <memory>

// -----------
// - Defines -
// -----------

#define RELEASE_TIMEOUT 10000
#define MAX_BMP_EXTENT	4096

// -----------
// - statics -
// -----------

static const char aHexData[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// -------------
// - GraphicID -
// -------------

class GraphicID
{
private:

	sal_uInt32	mnID1;
	sal_uInt32	mnID2;
	sal_uInt32	mnID3;
	sal_uInt32	mnID4;

				GraphicID();

public:


				GraphicID( const GraphicObject& rObj );
				~GraphicID() {}

	BOOL		operator==( const GraphicID& rID ) const
				{
					return( rID.mnID1 == mnID1 && rID.mnID2 == mnID2 &&
							rID.mnID3 == mnID3 && rID.mnID4 == mnID4 );
				}
    
	ByteString	GetIDString() const;
    BOOL        IsEmpty() const { return( 0 == mnID4 ); }
};

// -----------------------------------------------------------------------------

GraphicID::GraphicID( const GraphicObject& rObj )
{
	const Graphic& rGraphic = rObj.GetGraphic(); 

	mnID1 = ( (ULONG) rGraphic.GetType() ) << 28;

	switch( rGraphic.GetType() )
	{
		case( GRAPHIC_BITMAP ):
		{
			if( rGraphic.IsAnimated() )
			{
				const Animation aAnimation( rGraphic.GetAnimation() );

				mnID1 |= ( aAnimation.Count() & 0x0fffffff );
				mnID2 = aAnimation.GetDisplaySizePixel().Width();
				mnID3 = aAnimation.GetDisplaySizePixel().Height();
				mnID4 = rGraphic.GetChecksum();
			}
			else
			{
				const BitmapEx aBmpEx( rGraphic.GetBitmapEx() );

				mnID1 |= ( ( ( (ULONG) aBmpEx.GetTransparentType() << 8 ) | ( aBmpEx.IsAlpha() ? 1 : 0 ) ) & 0x0fffffff );
				mnID2 = aBmpEx.GetSizePixel().Width();
				mnID3 = aBmpEx.GetSizePixel().Height();
				mnID4 = rGraphic.GetChecksum();
			}
		}
		break;

		case( GRAPHIC_GDIMETAFILE ):
		{
			const GDIMetaFile aMtf( rGraphic.GetGDIMetaFile() );

			mnID1 |= ( aMtf.GetActionCount() & 0x0fffffff );
			mnID2 = aMtf.GetPrefSize().Width();
			mnID3 = aMtf.GetPrefSize().Height();
			mnID4 = rGraphic.GetChecksum();
		}
		break;

		default:
			mnID2 = mnID3 = mnID4 = 0;
		break;
	}
}

// -----------------------------------------------------------------------------

ByteString GraphicID::GetIDString() const
{
	ByteString	aHexStr;
	sal_Char*	pStr = aHexStr.AllocBuffer( 32 );
	sal_Int32	nShift;

	for( nShift = 28; nShift >= 0; nShift -= 4 )
		*pStr++ = aHexData[ ( mnID1 >> (sal_uInt32) nShift ) & 0xf ];

	for( nShift = 28; nShift >= 0; nShift -= 4 )
		*pStr++ = aHexData[ ( mnID2 >> (sal_uInt32) nShift ) & 0xf ];

	for( nShift = 28; nShift >= 0; nShift -= 4 )
		*pStr++ = aHexData[ ( mnID3 >> (sal_uInt32) nShift ) & 0xf ];

	for( nShift = 28; nShift >= 0; nShift -= 4 )
		*pStr++ = aHexData[ ( mnID4 >> (sal_uInt32) nShift ) & 0xf ];

	return aHexStr;
}

// ---------------------
// - GraphicCacheEntry -
// ---------------------

class GraphicCacheEntry
{
private:
	
	List				maGraphicObjectList;
	GraphicID			maID;
	GfxLink				maGfxLink;
	BitmapEx*			mpBmpEx;
	GDIMetaFile*		mpMtf;
	Animation*			mpAnimation;
	BOOL				mbSwappedAll;
						
	BOOL				ImplInit( const GraphicObject& rObj );
	BOOL				ImplMatches( const GraphicObject& rObj ) const { return( GraphicID( rObj ) == maID ); }
	void				ImplFillSubstitute( Graphic& rSubstitute );
						
public:					
						
						GraphicCacheEntry( const GraphicObject& rObj );
						~GraphicCacheEntry();

	const GraphicID&	GetID() const { return maID; }
						
	void				AddGraphicObjectReference( const GraphicObject& rObj, Graphic& rSubstitute );
	BOOL				ReleaseGraphicObjectReference( const GraphicObject& rObj );
	ULONG				GetGraphicObjectReferenceCount() { return maGraphicObjectList.Count(); }
	BOOL				HasGraphicObjectReference( const GraphicObject& rObj );
						
	void				TryToSwapIn();
	void				GraphicObjectWasSwappedOut( const GraphicObject& rObj );
	BOOL				FillSwappedGraphicObject( const GraphicObject& rObj, Graphic& rSubstitute );
	void				GraphicObjectWasSwappedIn( const GraphicObject& rObj );
};

// -----------------------------------------------------------------------------

GraphicCacheEntry::GraphicCacheEntry( const GraphicObject& rObj ) :
	maID			( rObj ),
	mpBmpEx			( NULL ),
	mpMtf			( NULL ),
	mpAnimation		( NULL ),
	mbSwappedAll    ( !ImplInit( rObj ) )
{
	maGraphicObjectList.Insert( (void*) &rObj, LIST_APPEND );
}

// -----------------------------------------------------------------------------

GraphicCacheEntry::~GraphicCacheEntry()
{
	DBG_ASSERT( !maGraphicObjectList.Count(), "GraphicCacheEntry::~GraphicCacheEntry(): Not all GraphicObjects are removed from this entry" );

	delete mpBmpEx;
	delete mpMtf;
	delete mpAnimation;
}

// -----------------------------------------------------------------------------

BOOL GraphicCacheEntry::ImplInit( const GraphicObject& rObj )
{
	BOOL bRet;

	if( !rObj.IsSwappedOut() )
	{
		const Graphic& rGraphic = rObj.GetGraphic();

		if( mpBmpEx )
			delete mpBmpEx, mpBmpEx = NULL;

		if( mpMtf )
			delete mpMtf, mpMtf = NULL;

		if( mpAnimation )
			delete mpAnimation, mpAnimation = NULL;

		switch( rGraphic.GetType() )
		{
			case( GRAPHIC_BITMAP ):
			{
				if( rGraphic.IsAnimated() )
					mpAnimation = new Animation( rGraphic.GetAnimation() );
				else
					mpBmpEx = new BitmapEx( rGraphic.GetBitmapEx() );
			}
			break;
					
			case( GRAPHIC_GDIMETAFILE ):
			{
				mpMtf = new GDIMetaFile( rGraphic.GetGDIMetaFile() );
			}
			break;

			default:
				DBG_ASSERT( GetID().IsEmpty(), "GraphicCacheEntry::ImplInit: Could not initialize graphic! (=>KA)" );
			break;
		}

		if( rGraphic.IsLink() )
			maGfxLink = ( (Graphic&) rGraphic ).GetLink();
		else
			maGfxLink = GfxLink();

		bRet = TRUE;
	}
	else
		bRet = FALSE;

	return bRet;
}

// -----------------------------------------------------------------------------

void GraphicCacheEntry::ImplFillSubstitute( Graphic& rSubstitute )
{
	// create substitute for graphic;
	const Size			aPrefSize( rSubstitute.GetPrefSize() );
	const MapMode		aPrefMapMode( rSubstitute.GetPrefMapMode() );
	const Link			aAnimationNotifyHdl( rSubstitute.GetAnimationNotifyHdl() );
	const String		aDocFileName( rSubstitute.GetDocFileName() );
	const ULONG			nDocFilePos = rSubstitute.GetDocFilePos();
	const GraphicType	eOldType = rSubstitute.GetType();
	const BOOL			bDefaultType = ( rSubstitute.GetType() == GRAPHIC_DEFAULT );

	if( rSubstitute.IsLink() && ( GFX_LINK_TYPE_NONE == maGfxLink.GetType() ) )
		maGfxLink = rSubstitute.GetLink();

	if( mpBmpEx )
		rSubstitute = *mpBmpEx;
	else if( mpAnimation )
		rSubstitute = *mpAnimation;
	else if( mpMtf )
		rSubstitute = *mpMtf;
	else
		rSubstitute.Clear();

	if( eOldType != GRAPHIC_NONE )
	{
		rSubstitute.SetPrefSize( aPrefSize );
		rSubstitute.SetPrefMapMode( aPrefMapMode );
		rSubstitute.SetAnimationNotifyHdl( aAnimationNotifyHdl );
		rSubstitute.SetDocFileName( aDocFileName, nDocFilePos );
	}

	if( GFX_LINK_TYPE_NONE != maGfxLink.GetType() )
		rSubstitute.SetLink( maGfxLink );

	if( bDefaultType )
		rSubstitute.SetDefaultType();
}

// -----------------------------------------------------------------------------

void GraphicCacheEntry::AddGraphicObjectReference( const GraphicObject& rObj, Graphic& rSubstitute )
{	
	if( mbSwappedAll )
		mbSwappedAll = !ImplInit( rObj );

	ImplFillSubstitute( rSubstitute );
	maGraphicObjectList.Insert( (void*) &rObj, LIST_APPEND );
}	

// -----------------------------------------------------------------------------

BOOL GraphicCacheEntry::ReleaseGraphicObjectReference( const GraphicObject& rObj )
{
	BOOL bRet = FALSE;

	for( void* pObj = maGraphicObjectList.First(); !bRet && pObj; pObj = maGraphicObjectList.Next() )
	{
		if( &rObj == (GraphicObject*) pObj )
		{
			maGraphicObjectList.Remove( pObj );
			bRet = TRUE;
		}
	}

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GraphicCacheEntry::HasGraphicObjectReference( const GraphicObject& rObj )
{
	BOOL bRet = FALSE;

	for( void* pObj = maGraphicObjectList.First(); !bRet && pObj; pObj = maGraphicObjectList.Next() )
		if( &rObj == (GraphicObject*) pObj )
			bRet = TRUE;

	return bRet;
}

// -----------------------------------------------------------------------------

void GraphicCacheEntry::TryToSwapIn()
{
	if( mbSwappedAll && maGraphicObjectList.Count() )
		( (GraphicObject*) maGraphicObjectList.First() )->FireSwapInRequest();
}

// -----------------------------------------------------------------------------

void GraphicCacheEntry::GraphicObjectWasSwappedOut( const GraphicObject& /*rObj*/ )
{
	mbSwappedAll = TRUE;

	for( void* pObj = maGraphicObjectList.First(); mbSwappedAll && pObj; pObj = maGraphicObjectList.Next() )
		if( !( (GraphicObject*) pObj )->IsSwappedOut() )
			mbSwappedAll = FALSE;

	if( mbSwappedAll )
	{
		delete mpBmpEx, mpBmpEx = NULL;
		delete mpMtf, mpMtf = NULL;
		delete mpAnimation, mpAnimation = NULL;
	}
}

// -----------------------------------------------------------------------------

BOOL GraphicCacheEntry::FillSwappedGraphicObject( const GraphicObject& rObj, Graphic& rSubstitute )
{
	BOOL bRet;

	if( !mbSwappedAll && rObj.IsSwappedOut() )
	{
		ImplFillSubstitute( rSubstitute );
		bRet = TRUE;
	}
	else
		bRet = FALSE;

	return bRet;
}

// -----------------------------------------------------------------------------

void GraphicCacheEntry::GraphicObjectWasSwappedIn( const GraphicObject& rObj )
{
	if( mbSwappedAll )
		mbSwappedAll = !ImplInit( rObj );
}

// ----------------------------
// - GraphicDisplayCacheEntry -
// ----------------------------

class GraphicDisplayCacheEntry
{
private:

	::vos::TTimeValue           maReleaseTime;
    const GraphicCacheEntry*	mpRefCacheEntry;
	GDIMetaFile*				mpMtf;
	BitmapEx*					mpBmpEx;
	GraphicAttr					maAttr;
	Size						maOutSizePix;
	ULONG						mnCacheSize;
    ULONG						mnOutDevDrawMode;
    USHORT						mnOutDevBitCount;

public:						
							
	static ULONG				GetNeededSize( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
											   const GraphicObject& rObj, const GraphicAttr& rAttr );
							
public:						
							
								GraphicDisplayCacheEntry( const GraphicCacheEntry* pRefCacheEntry,
														  OutputDevice* pOut, const Point& rPt, const Size& rSz, 
														  const GraphicObject& rObj, const GraphicAttr& rAttr,
														  const BitmapEx& rBmpEx ) :
									mpRefCacheEntry( pRefCacheEntry ),
									mpMtf( NULL ), mpBmpEx( new BitmapEx( rBmpEx ) ),
									maAttr( rAttr ), maOutSizePix( pOut->LogicToPixel( rSz ) ),
									mnCacheSize( GetNeededSize( pOut, rPt, rSz, rObj, rAttr ) ),
                                    mnOutDevDrawMode( pOut->GetDrawMode() ),
                                    mnOutDevBitCount( pOut->GetBitCount() )
                                    {
                                    }
								
								GraphicDisplayCacheEntry( const GraphicCacheEntry* pRefCacheEntry,
														  OutputDevice* pOut, const Point& rPt, const Size& rSz, 
														  const GraphicObject& rObj, const GraphicAttr& rAttr,
														  const GDIMetaFile& rMtf ) :
									mpRefCacheEntry( pRefCacheEntry ),
									mpMtf( new GDIMetaFile( rMtf ) ), mpBmpEx( NULL ),
									maAttr( rAttr ), maOutSizePix( pOut->LogicToPixel( rSz ) ),
									mnCacheSize( GetNeededSize( pOut, rPt, rSz, rObj, rAttr ) ),
                                    mnOutDevDrawMode( pOut->GetDrawMode() ),
                                    mnOutDevBitCount( pOut->GetBitCount() ) 
                                    {
                                    }	
                                    	
								
								~GraphicDisplayCacheEntry();

	const GraphicAttr&			GetAttr() const { return maAttr; }
	const Size&					GetOutputSizePixel() const { return maOutSizePix; }
	ULONG					GetCacheSize() const { return mnCacheSize; }
	const GraphicCacheEntry*	GetReferencedCacheEntry() const { return mpRefCacheEntry; }
    ULONG					GetOutDevDrawMode() const { return mnOutDevDrawMode; }
    USHORT				GetOutDevBitCount()	const { return mnOutDevBitCount; }
    
    void                        SetReleaseTime( const ::vos::TTimeValue& rReleaseTime ) { maReleaseTime = rReleaseTime; }
    const ::vos::TTimeValue&    GetReleaseTime() const { return maReleaseTime; }
								
	BOOL						Matches( OutputDevice* pOut, const Point& /*rPtPixel*/, const Size& rSzPixel,
										 const GraphicCacheEntry* pCacheEntry, const GraphicAttr& rAttr ) const
								{
                                    // #i46805# Additional match
                                    // criteria: outdev draw mode and
                                    // bit count. One cannot reuse
                                    // this cache object, if it's
                                    // e.g. generated for
                                    // DRAWMODE_GRAYBITMAP.
									return( ( pCacheEntry == mpRefCacheEntry ) &&
											( maAttr == rAttr ) &&
											( ( maOutSizePix == rSzPixel ) || ( !maOutSizePix.Width() && !maOutSizePix.Height() ) ) &&
                                            ( pOut->GetBitCount() == mnOutDevBitCount ) &&
                                            ( pOut->GetDrawMode() == mnOutDevDrawMode ) );
								}
								
	void						Draw( OutputDevice* pOut, const Point& rPt, const Size& rSz ) const;
};

// -----------------------------------------------------------------------------

ULONG GraphicDisplayCacheEntry::GetNeededSize( OutputDevice* pOut, const Point& /*rPt*/, const Size& rSz, 
											   const GraphicObject& rObj, const GraphicAttr& rAttr )
{
    const Graphic&      rGraphic = rObj.GetGraphic();
    const GraphicType	eType = rGraphic.GetType();
	ULONG				nNeededSize;

	if( GRAPHIC_BITMAP == eType )
	{
		const Size aOutSizePix( pOut->LogicToPixel( rSz ) );
		const long nBitCount = pOut->GetBitCount();

		if( ( aOutSizePix.Width() > MAX_BMP_EXTENT ) || 
            ( aOutSizePix.Height() > MAX_BMP_EXTENT ) )
        {
		    nNeededSize = ULONG_MAX;
        }
		else if( nBitCount )
		{
		    nNeededSize = aOutSizePix.Width() * aOutSizePix.Height() * nBitCount / 8;

    		if( rObj.IsTransparent() || ( rAttr.GetRotation() % 3600 ) )
	    		nNeededSize += nNeededSize / nBitCount;
		}
		else
	    {
     		DBG_ERROR( "GraphicDisplayCacheEntry::GetNeededSize(): pOut->GetBitCount() == 0" );
		    nNeededSize = 256000;
        }
	}
	else if( GRAPHIC_GDIMETAFILE == eType )
		nNeededSize = rGraphic.GetSizeBytes();
	else
		nNeededSize = 0;

	return nNeededSize;
}

// -----------------------------------------------------------------------------

GraphicDisplayCacheEntry::~GraphicDisplayCacheEntry()
{
	if( mpMtf )
		delete mpMtf;

	if( mpBmpEx )
		delete mpBmpEx;
}

// -----------------------------------------------------------------------------

void GraphicDisplayCacheEntry::Draw( OutputDevice* pOut, const Point& rPt, const Size& rSz ) const
{
	if( mpMtf )
		GraphicManager::ImplDraw( pOut, rPt, rSz, *mpMtf, maAttr );
	else if( mpBmpEx )
	{
		if( maAttr.IsRotated() )
		{
			Polygon aPoly( Rectangle( rPt, rSz ) );
			
			aPoly.Rotate( rPt, maAttr.GetRotation() % 3600 );
			const Rectangle aRotBoundRect( aPoly.GetBoundRect() );
			pOut->DrawBitmapEx( aRotBoundRect.TopLeft(), aRotBoundRect.GetSize(), *mpBmpEx );
		}
		else
			pOut->DrawBitmapEx( rPt, rSz, *mpBmpEx );
	}
}

// -----------------------
// - GraphicCache -
// -----------------------

GraphicCache::GraphicCache( GraphicManager& rMgr, ULONG nDisplayCacheSize, ULONG nMaxObjDisplayCacheSize ) :
	mrMgr				    ( rMgr ),
    mnReleaseTimeoutSeconds ( 0UL ),
	mnMaxDisplaySize	    ( nDisplayCacheSize ),
	mnMaxObjDisplaySize	    ( nMaxObjDisplayCacheSize ),
	mnUsedDisplaySize	    ( 0UL )
{
    maReleaseTimer.SetTimeoutHdl( LINK( this, GraphicCache, ReleaseTimeoutHdl ) );
    maReleaseTimer.SetTimeout( RELEASE_TIMEOUT );
    maReleaseTimer.Start();
}

// -----------------------------------------------------------------------------
										   
GraphicCache::~GraphicCache()
{
	DBG_ASSERT( !maGraphicCache.Count(), "GraphicCache::~GraphicCache(): there are some GraphicObjects in cache" );
	DBG_ASSERT( !maDisplayCache.Count(), "GraphicCache::~GraphicCache(): there are some GraphicObjects in display cache" );
}

// -----------------------------------------------------------------------------

void GraphicCache::AddGraphicObject( const GraphicObject& rObj, Graphic& rSubstitute, 
                                     const ByteString* pID, const GraphicObject* pCopyObj )
{
	BOOL bInserted = FALSE;

	if( !rObj.IsSwappedOut() && 
        ( pID || ( pCopyObj && ( pCopyObj->GetType() != GRAPHIC_NONE ) ) || ( rObj.GetType() != GRAPHIC_NONE ) ) )
	{
        if( pCopyObj )
        {
            GraphicCacheEntry* pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.First() );
        
            while( !bInserted && pEntry )
            {
                if( pEntry->HasGraphicObjectReference( *pCopyObj ) )
                {
                    pEntry->AddGraphicObjectReference( rObj, rSubstitute );
                    bInserted = TRUE;
                }
                else
                {
                    pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.Next() );
                }
            }
        }
        
        if( !bInserted )
        {
            GraphicCacheEntry* pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.First() );
            ::std::auto_ptr< GraphicID > apID;
            
            if( !pID )
            {
                apID.reset( new GraphicID( rObj ) );
            }
    
            while( !bInserted && pEntry )
            {
                const GraphicID& rEntryID = pEntry->GetID();
    
                if( pID )
                {
                    if( rEntryID.GetIDString() == *pID )
                    {
                        pEntry->TryToSwapIn();
    
                        // since pEntry->TryToSwapIn can modify our current list, we have to
                        // iterate from beginning to add a reference to the appropriate
                        // CacheEntry object; after this, quickly jump out of the outer iteration
                        for( pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.First() ); 
                             !bInserted && pEntry; 
                             pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.Next() ) )
                        {
                            const GraphicID& rID = pEntry->GetID();
    
                            if( rID.GetIDString() == *pID )
                            {
                                pEntry->AddGraphicObjectReference( rObj, rSubstitute );
                                bInserted = TRUE;
                            }
                        }
    
                        if( !bInserted )
                        {
                            maGraphicCache.Insert( new GraphicCacheEntry( rObj ), LIST_APPEND );
                            bInserted = TRUE;
                        }
                    }
                }
                else
                {
                    if( rEntryID == *apID )
                    {
                        pEntry->AddGraphicObjectReference( rObj, rSubstitute );
                        bInserted = TRUE;
                    }
                }
    
                if( !bInserted )
                    pEntry = static_cast< GraphicCacheEntry* >( maGraphicCache.Next() );
            }
        }
	}

	if( !bInserted )
		maGraphicCache.Insert( new GraphicCacheEntry( rObj ), LIST_APPEND );
}

// -----------------------------------------------------------------------------

void GraphicCache::ReleaseGraphicObject( const GraphicObject& rObj )
{
	// Release cached object
	GraphicCacheEntry*	pEntry = (GraphicCacheEntry*) maGraphicCache.First();
	BOOL				bRemoved = FALSE;

	while( !bRemoved && pEntry )
	{
		bRemoved = pEntry->ReleaseGraphicObjectReference( rObj );

		if( bRemoved )
		{
			if( 0 == pEntry->GetGraphicObjectReferenceCount() )
			{
				// if graphic cache entry has no more references,
				// the corresponding display cache object can be removed
				GraphicDisplayCacheEntry* pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.First();

				while( pDisplayEntry )
				{
					if( pDisplayEntry->GetReferencedCacheEntry() == pEntry )
					{
						mnUsedDisplaySize -= pDisplayEntry->GetCacheSize();
						maDisplayCache.Remove( pDisplayEntry );
						delete pDisplayEntry;
						pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.GetCurObject();
					}
					else
						pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.Next();
				}

				// delete graphic cache entry
				maGraphicCache.Remove( (void*) pEntry );
				delete pEntry;
			}
		}
		else
			pEntry = (GraphicCacheEntry*) maGraphicCache.Next();
	}

	DBG_ASSERT( bRemoved, "GraphicCache::ReleaseGraphicObject(...): GraphicObject not found in cache" );
}

// -----------------------------------------------------------------------------

void GraphicCache::GraphicObjectWasSwappedOut( const GraphicObject& rObj )
{
    // notify cache that rObj is swapped out (and can thus be pruned
    // from the cache)
	GraphicCacheEntry* pEntry = ImplGetCacheEntry( rObj );

    if( pEntry )
        pEntry->GraphicObjectWasSwappedOut( rObj );
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::FillSwappedGraphicObject( const GraphicObject& rObj, Graphic& rSubstitute )
{
	GraphicCacheEntry* pEntry = ImplGetCacheEntry( rObj );

    if( !pEntry )
        return FALSE;

	return pEntry->FillSwappedGraphicObject( rObj, rSubstitute );
}

// -----------------------------------------------------------------------------

void GraphicCache::GraphicObjectWasSwappedIn( const GraphicObject& rObj )
{
	GraphicCacheEntry* pEntry = ImplGetCacheEntry( rObj );

    if( pEntry )
    {
	    if( pEntry->GetID().IsEmpty() )
	    {
		    ReleaseGraphicObject( rObj );
		    AddGraphicObject( rObj, (Graphic&) rObj.GetGraphic(), NULL, NULL ); 
	    }
	    else
		    pEntry->GraphicObjectWasSwappedIn( rObj );
    }
}

// -----------------------------------------------------------------------------

void GraphicCache::SetMaxDisplayCacheSize( ULONG nNewCacheSize )
{
	mnMaxDisplaySize = nNewCacheSize;

	if( GetMaxDisplayCacheSize() < GetUsedDisplayCacheSize() )
		ImplFreeDisplayCacheSpace( GetUsedDisplayCacheSize() - GetMaxDisplayCacheSize() );
}

// -----------------------------------------------------------------------------

void GraphicCache::SetMaxObjDisplayCacheSize( ULONG nNewMaxObjSize, BOOL bDestroyGreaterCached )
{
	const BOOL bDestroy = ( bDestroyGreaterCached && ( nNewMaxObjSize < mnMaxObjDisplaySize ) );

	mnMaxObjDisplaySize = Min( nNewMaxObjSize, mnMaxDisplaySize );

	if( bDestroy )
	{
		GraphicDisplayCacheEntry* pCacheObj = (GraphicDisplayCacheEntry*) maDisplayCache.First();

		while( pCacheObj )
		{
			if( pCacheObj->GetCacheSize() > mnMaxObjDisplaySize )
			{
				mnUsedDisplaySize -= pCacheObj->GetCacheSize();
				maDisplayCache.Remove( pCacheObj );
				delete pCacheObj;
				pCacheObj = (GraphicDisplayCacheEntry*) maDisplayCache.GetCurObject();
			}
			else
				pCacheObj = (GraphicDisplayCacheEntry*) maDisplayCache.Next();
		}
	}
}

// -----------------------------------------------------------------------------

void GraphicCache::SetCacheTimeout( ULONG nTimeoutSeconds )
{
    if( mnReleaseTimeoutSeconds != nTimeoutSeconds )
    {
        GraphicDisplayCacheEntry*   pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.First();
        ::vos::TTimeValue           aReleaseTime;

        if( ( mnReleaseTimeoutSeconds = nTimeoutSeconds ) != 0 )
        {
            osl_getSystemTime( &aReleaseTime );
            aReleaseTime.addTime( ::vos::TTimeValue( nTimeoutSeconds, 0 ) );
        }

	    while( pDisplayEntry )
	    {
            pDisplayEntry->SetReleaseTime( aReleaseTime );
		    pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.Next();
	    }
    }
}

// -----------------------------------------------------------------------------

void GraphicCache::ClearDisplayCache()
{
	for( void* pObj = maDisplayCache.First(); pObj; pObj = maDisplayCache.Next() )
		delete (GraphicDisplayCacheEntry*) pObj;

	maDisplayCache.Clear();
	mnUsedDisplaySize = 0UL;
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::IsDisplayCacheable( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
									   const GraphicObject& rObj, const GraphicAttr& rAttr ) const
{
	return( GraphicDisplayCacheEntry::GetNeededSize( pOut, rPt, rSz, rObj, rAttr ) <= 
			GetMaxObjDisplayCacheSize() );
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::IsInDisplayCache( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
									 const GraphicObject& rObj, const GraphicAttr& rAttr ) const
{
	const Point					aPtPixel( pOut->LogicToPixel( rPt ) );
	const Size					aSzPixel( pOut->LogicToPixel( rSz ) );
	const GraphicCacheEntry*	pCacheEntry = ( (GraphicCache*) this )->ImplGetCacheEntry( rObj );
	//GraphicDisplayCacheEntry*	pDisplayEntry = (GraphicDisplayCacheEntry*) ( (GraphicCache*) this )->maDisplayCache.First(); // -Wall removed ....
	BOOL						bFound = FALSE;

    if( pCacheEntry )
    {
        for( long i = 0, nCount = maDisplayCache.Count(); !bFound && ( i < nCount ); i++ )
            if( ( (GraphicDisplayCacheEntry*) maDisplayCache.GetObject( i ) )->Matches( pOut, aPtPixel, aSzPixel, pCacheEntry, rAttr ) )
                bFound = TRUE;
    }

	return bFound;
}

// -----------------------------------------------------------------------------

ByteString GraphicCache::GetUniqueID( const GraphicObject& rObj ) const
{
	ByteString			aRet;
	GraphicCacheEntry*	pEntry = ( (GraphicCache*) this )->ImplGetCacheEntry( rObj );

	// ensure that the entry is correctly initialized (it has to be read at least once)
	if( pEntry && pEntry->GetID().IsEmpty() )
		pEntry->TryToSwapIn();

	// do another call to ImplGetCacheEntry in case of modified entry list
	pEntry = ( (GraphicCache*) this )->ImplGetCacheEntry( rObj );

	if( pEntry )
		aRet = pEntry->GetID().GetIDString();
	
	return aRet;
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::CreateDisplayCacheObj( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
										  const GraphicObject& rObj, const GraphicAttr& rAttr,
										  const BitmapEx& rBmpEx )
{
	const ULONG nNeededSize = GraphicDisplayCacheEntry::GetNeededSize( pOut, rPt, rSz, rObj, rAttr );
	BOOL		bRet = FALSE;

	if( nNeededSize <= GetMaxObjDisplayCacheSize() )
	{
		if( nNeededSize > GetFreeDisplayCacheSize() )
			ImplFreeDisplayCacheSpace( nNeededSize - GetFreeDisplayCacheSize() );

		GraphicDisplayCacheEntry* pNewEntry = new GraphicDisplayCacheEntry( ImplGetCacheEntry( rObj ), 
																			pOut, rPt, rSz, rObj, rAttr, rBmpEx );

        if( GetCacheTimeout() )
        {
            ::vos::TTimeValue aReleaseTime;

            osl_getSystemTime( &aReleaseTime );
            aReleaseTime.addTime( ::vos::TTimeValue( GetCacheTimeout(), 0 ) );
            pNewEntry->SetReleaseTime( aReleaseTime );
        }
																			
		maDisplayCache.Insert( pNewEntry, LIST_APPEND );
		mnUsedDisplaySize += pNewEntry->GetCacheSize();
		bRet = TRUE;
	}

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::CreateDisplayCacheObj( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
										  const GraphicObject& rObj, const GraphicAttr& rAttr,
										  const GDIMetaFile& rMtf )
{
	const ULONG nNeededSize = GraphicDisplayCacheEntry::GetNeededSize( pOut, rPt, rSz, rObj, rAttr );
	BOOL		bRet = FALSE;

	if( nNeededSize <= GetMaxObjDisplayCacheSize() )
	{
		if( nNeededSize > GetFreeDisplayCacheSize() )
			ImplFreeDisplayCacheSpace( nNeededSize - GetFreeDisplayCacheSize() );

		GraphicDisplayCacheEntry* pNewEntry = new GraphicDisplayCacheEntry( ImplGetCacheEntry( rObj ), 
																			pOut, rPt, rSz, rObj, rAttr, rMtf );

        if( GetCacheTimeout() )
        {
            ::vos::TTimeValue aReleaseTime;

            osl_getSystemTime( &aReleaseTime );
            aReleaseTime.addTime( ::vos::TTimeValue( GetCacheTimeout(), 0 ) );
            pNewEntry->SetReleaseTime( aReleaseTime );
        }
																			
		maDisplayCache.Insert( pNewEntry, LIST_APPEND );
		mnUsedDisplaySize += pNewEntry->GetCacheSize();
		bRet = TRUE;
	}

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::DrawDisplayCacheObj( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
										const GraphicObject& rObj, const GraphicAttr& rAttr )
{
	const Point					aPtPixel( pOut->LogicToPixel( rPt ) );
	const Size					aSzPixel( pOut->LogicToPixel( rSz ) );
	const GraphicCacheEntry*	pCacheEntry = ImplGetCacheEntry( rObj );
	GraphicDisplayCacheEntry*	pDisplayCacheEntry = (GraphicDisplayCacheEntry*) maDisplayCache.First();
	BOOL						bRet = FALSE;

	while( !bRet && pDisplayCacheEntry )
	{
		if( pDisplayCacheEntry->Matches( pOut, aPtPixel, aSzPixel, pCacheEntry, rAttr ) )
		{
            ::vos::TTimeValue aReleaseTime;

			// put found object at last used position
			maDisplayCache.Insert( maDisplayCache.Remove( pDisplayCacheEntry ), LIST_APPEND );

            if( GetCacheTimeout() )
            {
                osl_getSystemTime( &aReleaseTime );
                aReleaseTime.addTime( ::vos::TTimeValue( GetCacheTimeout(), 0 ) );
            }

            pDisplayCacheEntry->SetReleaseTime( aReleaseTime );
			bRet = TRUE;
		}
		else
			pDisplayCacheEntry = (GraphicDisplayCacheEntry*) maDisplayCache.Next();
	}

	if( bRet )
		pDisplayCacheEntry->Draw( pOut, rPt, rSz );

	return bRet;
}

// -----------------------------------------------------------------------------

BOOL GraphicCache::ImplFreeDisplayCacheSpace( ULONG nSizeToFree )
{
	ULONG nFreedSize = 0UL;

	if( nSizeToFree )
	{
		void* pObj = maDisplayCache.First();

        if( nSizeToFree > mnUsedDisplaySize )
            nSizeToFree = mnUsedDisplaySize;

		while( pObj )
		{
			GraphicDisplayCacheEntry* pCacheObj = (GraphicDisplayCacheEntry*) pObj;

			nFreedSize += pCacheObj->GetCacheSize();
			mnUsedDisplaySize -= pCacheObj->GetCacheSize();
			maDisplayCache.Remove( pObj );
			delete pCacheObj;

			if( nFreedSize >= nSizeToFree )
				break;
			else
				pObj = maDisplayCache.GetCurObject();
		}
	}

	return( nFreedSize >= nSizeToFree );
}

// -----------------------------------------------------------------------------

GraphicCacheEntry* GraphicCache::ImplGetCacheEntry( const GraphicObject& rObj )
{
	GraphicCacheEntry* pRet = NULL;

	for( void* pObj = maGraphicCache.First(); !pRet && pObj; pObj = maGraphicCache.Next() )
		if( ( (GraphicCacheEntry*) pObj )->HasGraphicObjectReference( rObj ) )
			pRet = (GraphicCacheEntry*) pObj;

	return pRet;
}

// -----------------------------------------------------------------------------

IMPL_LINK( GraphicCache, ReleaseTimeoutHdl, Timer*, pTimer )
{
    pTimer->Stop();

    ::vos::TTimeValue           aCurTime;
	GraphicDisplayCacheEntry*   pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.First();

    osl_getSystemTime( &aCurTime );

	while( pDisplayEntry )
	{
        const ::vos::TTimeValue& rReleaseTime = pDisplayEntry->GetReleaseTime();

		if( !rReleaseTime.isEmpty() && ( rReleaseTime < aCurTime ) )
		{
			mnUsedDisplaySize -= pDisplayEntry->GetCacheSize();
			maDisplayCache.Remove( pDisplayEntry );
			delete pDisplayEntry;
			pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.GetCurObject();
		}
		else
			pDisplayEntry = (GraphicDisplayCacheEntry*) maDisplayCache.Next();
	}

    pTimer->Start();

    return 0;
}
