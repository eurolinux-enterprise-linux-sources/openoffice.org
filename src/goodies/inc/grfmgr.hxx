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

#ifndef _GRFMGR_HXX
#define _GRFMGR_HXX

#include <vcl/graph.hxx>

// -----------
// - Defines -
// -----------

#define GRFMGR_DRAW_NOTCACHED			    0x00000000UL
#define GRFMGR_DRAW_CACHED				    0x00000001UL
#define GRFMGR_DRAW_BILINEAR			    0x00000002UL
#define GRFMGR_DRAW_USE_DRAWMODE_SETTINGS	0x00000004UL
#define GRFMGR_DRAW_SUBSTITUTE	            0x00000008UL
#define GRFMGR_DRAW_NO_SUBSTITUTE	        0x00000010UL
#define GRFMGR_DRAW_STANDARD			    (GRFMGR_DRAW_CACHED|GRFMGR_DRAW_BILINEAR)

// --------------------
// - AutoSwap Defines -
// --------------------

#define GRFMGR_AUTOSWAPSTREAM_LINK		((SvStream*)0x00000000UL)
#define GRFMGR_AUTOSWAPSTREAM_LOADED	((SvStream*)0xfffffffdUL)
#define GRFMGR_AUTOSWAPSTREAM_TEMP		((SvStream*)0xfffffffeUL)
#define GRFMGR_AUTOSWAPSTREAM_NONE		((SvStream*)0xffffffffUL)

// ----------------------
// - Adjustment Defines -
// ----------------------

#define ADJUSTMENT_NONE					0x00000000UL
#define ADJUSTMENT_DRAWMODE				0x00000001UL
#define ADJUSTMENT_COLORS				0x00000002UL
#define ADJUSTMENT_MIRROR				0x00000004UL
#define ADJUSTMENT_ROTATE				0x00000008UL
#define ADJUSTMENT_TRANSPARENCY			0x00000010UL
#define ADJUSTMENT_ALL					0xFFFFFFFFUL

// ---------
// - Enums -
// ---------

enum GraphicDrawMode
{
	GRAPHICDRAWMODE_STANDARD = 0,
	GRAPHICDRAWMODE_GREYS = 1,
	GRAPHICDRAWMODE_MONO = 2,
	GRAPHICDRAWMODE_WATERMARK = 3
};

// ------------
// - Forwards -
// ------------

class GraphicManager;
class SvStream;
class BitmapWriteAccess;
class GraphicCache;
class VirtualDevice;
struct GrfSimpleCacheObj;
struct ImplTileInfo;

// ---------------
// - GraphicAttr -
// ---------------

class GraphicAttr
{
private:

	long			mnDummy1;
	long			mnDummy2;
	double			mfGamma;
	sal_uInt32		mnMirrFlags;
	long			mnLeftCrop;
	long			mnTopCrop;
	long			mnRightCrop;
	long			mnBottomCrop;
	USHORT			mnRotate10;
	short			mnContPercent;
	short			mnLumPercent;
	short			mnRPercent;
	short			mnGPercent;
	short			mnBPercent;
	BOOL			mbInvert;
	BYTE			mcTransparency;
	GraphicDrawMode	meDrawMode;

	void*			mpDummy;

public:

					GraphicAttr();
					~GraphicAttr();

	BOOL			operator==( const GraphicAttr& rAttr ) const;
	BOOL			operator!=( const GraphicAttr& rAttr ) const { return !( *this == rAttr ); }

	void			SetDrawMode( GraphicDrawMode eDrawMode ) { meDrawMode = eDrawMode; }
	GraphicDrawMode	GetDrawMode() const { return meDrawMode; }

	void			SetMirrorFlags( ULONG nMirrFlags ) { mnMirrFlags = nMirrFlags; }
	ULONG			GetMirrorFlags() const { return mnMirrFlags; }

	void			SetCrop( long nLeft_100TH_MM, long nTop_100TH_MM, long nRight_100TH_MM, long nBottom_100TH_MM )
					{
						mnLeftCrop = nLeft_100TH_MM; mnTopCrop = nTop_100TH_MM;
						mnRightCrop = nRight_100TH_MM; mnBottomCrop = nBottom_100TH_MM;
					}
	long			GetLeftCrop() const { return mnLeftCrop; }
	long			GetTopCrop() const { return mnTopCrop; }
	long			GetRightCrop() const { return mnRightCrop; }
	long			GetBottomCrop() const { return mnBottomCrop; }

	void			SetRotation( USHORT nRotate10 ) { mnRotate10 = nRotate10; }
	USHORT			GetRotation() const { return mnRotate10; }

	void			SetLuminance( short nLuminancePercent ) { mnLumPercent = nLuminancePercent; }
	short			GetLuminance() const { return mnLumPercent; }

	void			SetContrast( short nContrastPercent ) { mnContPercent = nContrastPercent; }
	short			GetContrast() const { return mnContPercent; }

	void			SetChannelR( short nChannelRPercent ) { mnRPercent = nChannelRPercent; }
	short			GetChannelR() const { return mnRPercent; }

	void			SetChannelG( short nChannelGPercent ) { mnGPercent = nChannelGPercent; }
	short			GetChannelG() const { return mnGPercent; }

	void			SetChannelB( short nChannelBPercent ) { mnBPercent = nChannelBPercent; }
	short			GetChannelB() const { return mnBPercent; }

	void			SetGamma( double fGamma ) { mfGamma = fGamma; }
	double			GetGamma() const { return mfGamma; }

	void			SetInvert( BOOL bInvert ) { mbInvert = bInvert; }
	BOOL			IsInvert() const { return mbInvert; }

	void			SetTransparency( BYTE cTransparency ) { mcTransparency = cTransparency; }
	BYTE			GetTransparency() const { return mcTransparency; }

	BOOL			IsSpecialDrawMode() const { return( meDrawMode != GRAPHICDRAWMODE_STANDARD ); }
	BOOL			IsMirrored() const { return( mnMirrFlags != 0UL ); }
	BOOL			IsCropped() const
					{
						return( mnLeftCrop != 0 || mnTopCrop != 0 || 
								mnRightCrop != 0 || mnBottomCrop != 0 );
					}
	BOOL			IsRotated() const { return( ( mnRotate10 % 3600 ) != 0 ); }
	BOOL			IsTransparent() const { return( mcTransparency > 0 ); }
	BOOL			IsAdjusted() const
					{
						return( mnLumPercent != 0 || mnContPercent != 0 || mnRPercent != 0 ||
								mnGPercent != 0 || mnBPercent != 0 || mfGamma != 1.0 || mbInvert );
					}

    friend SvStream& operator<<( SvStream& rOStm, const GraphicAttr& rAttr );
    friend SvStream& operator>>( SvStream& rIStm, GraphicAttr& rAttr );
};

// -----------------
// - GraphicObject -
// -----------------

class GraphicObject : public SvDataCopyStream
{
	friend class GraphicManager;

private:

	static GraphicManager*	mpGlobalMgr;

	Graphic					maGraphic;
	GraphicAttr				maAttr;
	Size					maPrefSize;
	MapMode					maPrefMapMode;
	ULONG					mnSizeBytes;
	GraphicType				meType;
	GraphicManager*			mpMgr;
	String*					mpLink;
	Link*					mpSwapStreamHdl;
	String*					mpUserData;
	Timer*					mpSwapOutTimer;
	GrfSimpleCacheObj*		mpSimpleCache;
    ULONG                   mnAnimationLoopCount;
	void*                   mpDummy1;
	void*                   mpDummy2;
	BOOL					mbAutoSwapped	: 1;
	BOOL					mbTransparent	: 1;
	BOOL					mbAnimated		: 1;
	BOOL					mbEPS			: 1;
	BOOL					mbIsInSwapIn	: 1;
	BOOL					mbIsInSwapOut	: 1;
	BOOL					mbAlpha			: 1;
	BOOL					mbDummyFlag8	: 1;

//#if 0 // _SOLAR__PRIVATE

	void					ImplConstruct();
	void					ImplAssignGraphicData();
	void					ImplSetGraphicManager( const GraphicManager* pMgr, 
                                                   const ByteString* pID = NULL,
                                                   const GraphicObject* pCopyObj = NULL );
	void					ImplAutoSwapIn();
	BOOL					ImplIsAutoSwapped() const { return mbAutoSwapped; }
	BOOL					ImplGetCropParams( OutputDevice* pOut, Point& rPt, Size& rSz, const GraphicAttr* pAttr,
											   PolyPolygon& rClipPolyPoly, BOOL& bRectClipRegion ) const;

	/** Render a given number of tiles in an optimized way

		This method recursively subdivides the tile rendering problem
		in smaller parts, i.e. rendering output size x with few tiles
		of size y, which in turn are generated from the original
		bitmap in a recursive fashion. The subdivision size can be
		controlled by the exponent argument, which specifies the
		minimal number of smaller tiles used in one recursion
		step. The resulting tile size is given as the integer number
		of repetitions of the original bitmap along x and y. As the
		exponent need not necessarily divide these numbers without
		remainder, the repetition counts are effectively converted to
		base-exponent numbers, where each place denotes the number of
		times the corresponding tile size is rendered.

        @param rVDev
        Virtual device to render everything into

        @param nExponent
        Number of repetitions per subdivision step, _must_ be greater than 1

        @param nNumTilesX
        Number of original tiles to generate in x direction

        @param nNumTilesY
        Number of original tiles to generate in y direction

        @param rTileSizePixel
        Size in pixel of the original tile bitmap to render it in

        @param pAttr
        Graphic attributes to be used for rendering 

        @param nFlags
        Graphic flags to be used for rendering

        @param rCurrPos
        Current output point for this recursion level (should start with (0,0))

        @return true, if everything was successfully rendered.
    */
    bool 					ImplRenderTempTile( VirtualDevice& rVDev, int nExponent, 
                                                int nNumTilesX, int nNumTilesY, 
                                                const Size& rTileSizePixel, 
                                                const GraphicAttr* pAttr, ULONG nFlags );

    /// internally called by ImplRenderTempTile()
    bool 					ImplRenderTileRecursive( VirtualDevice& rVDev, int nExponent, int nMSBFactor,
                                                     int nNumOrigTilesX, int nNumOrigTilesY, 
                                                     int nRemainderTilesX, int nRemainderTilesY, 
                                                     const Size& rTileSizePixel, const GraphicAttr* pAttr, 
                                                     ULONG nFlags, ImplTileInfo& rTileInfo );

    bool 					ImplDrawTiled( OutputDevice* pOut, const Rectangle& rArea, const Size& rSizePixel,
                                           const Size& rOffset, const GraphicAttr* pAttr, ULONG nFlags, int nTileCacheSize1D );

    bool 					ImplDrawTiled( OutputDevice& rOut, const Point& rPos, 
                                           int nNumTilesX, int nNumTilesY, 
                                           const Size& rTileSize, 
                                           const GraphicAttr* pAttr, ULONG nFlags );

    void 					ImplTransformBitmap( BitmapEx& 			rBmpEx,
                                                 const GraphicAttr& rAttr,
                                                 const Size&		rCropLeftTop,
                                                 const Size&		rCropRightBottom,
                                                 const Rectangle&	rCropRect,
                                                 const Size& 		rDstSize,
                                                 BOOL				bEnlarge ) const;

							DECL_LINK( ImplAutoSwapOutHdl, void* );

//#endif // _SOLAR__PRIVATE

protected:

	virtual void			GraphicManagerDestroyed();
	virtual SvStream*		GetSwapStream() const;

	// !!! to be removed
	virtual	ULONG			GetReleaseFromCache() const;

	virtual void			Load( SvStream& );
    virtual void			Save( SvStream& );
    virtual void			Assign( const SvDataCopyStream& );

public:

	                        TYPEINFO();

							GraphicObject( const GraphicManager* pMgr = NULL );
							GraphicObject( const Graphic& rGraphic, const GraphicManager* pMgr = NULL );
							GraphicObject( const Graphic& rGraphic, const String& rLink, const GraphicManager* pMgr = NULL );
							GraphicObject( const GraphicObject& rCacheObj, const GraphicManager* pMgr = NULL );
							GraphicObject( const ByteString& rUniqueID, const GraphicManager* pMgr = NULL );
							~GraphicObject();

	GraphicObject&			operator=( const GraphicObject& rCacheObj );
	BOOL					operator==( const GraphicObject& rCacheObj ) const;
	BOOL					operator!=( const GraphicObject& rCacheObj ) const { return !( *this == rCacheObj ); }

	BOOL					HasSwapStreamHdl() const { return( mpSwapStreamHdl != NULL && mpSwapStreamHdl->IsSet() ); }
	void					SetSwapStreamHdl();
	void					SetSwapStreamHdl( const Link& rHdl, const ULONG nSwapOutTimeout = 0UL );
	Link					GetSwapStreamHdl() const;
	ULONG					GetSwapOutTimeout() const { return( mpSwapOutTimer ? mpSwapOutTimer->GetTimeout() : 0 ); }

	void					FireSwapInRequest();
	void					FireSwapOutRequest();

	void					SetGraphicManager( const GraphicManager& rMgr );
	GraphicManager&			GetGraphicManager() const { return *mpMgr; }

	BOOL					IsCached( OutputDevice* pOut, const Point& rPt, const Size& rSz,
									  const GraphicAttr* pAttr = NULL, ULONG nFlags = GRFMGR_DRAW_STANDARD) const;
	void					ReleaseFromCache();

	const Graphic&			GetGraphic() const;
	void					SetGraphic( const Graphic& rGraphic );
	void					SetGraphic( const Graphic& rGraphic, const String& rLink );

    /** Get graphic transformed according to given attributes

    	This method returns a Graphic transformed, cropped and scaled
    	to the given parameters, ready to be rendered to printer or
    	display. The returned graphic has the same visual appearance
    	as if it had been drawn via GraphicObject::Draw() to a
    	specific output device.

        @param rDestSize
        Desired output size in logical coordinates. The mapmode to
        interpret these logical coordinates in is given by the second
        parameter, rDestMap.

        @param rDestMap
        Mapmode the output should be interpreted in. This is used to
        interpret rDestSize, to set the appropriate PrefMapMode on the
        returned Graphic, and to deal correctly with metafile graphics.

        @param rAttr
        Graphic attributes used to transform the graphic. This
        includes cropping, rotation, mirroring, and various color
        adjustment parameters.

        @return the readily transformed Graphic
     */
    Graphic 				GetTransformedGraphic( const Size& rDestSize, const MapMode& rDestMap, const GraphicAttr& rAttr ) const;
	Graphic					GetTransformedGraphic( const GraphicAttr* pAttr = NULL ) const; // TODO: Change to Impl

	void					SetAttr( const GraphicAttr& rAttr );
	const GraphicAttr&		GetAttr() const { return maAttr; }

	BOOL					HasLink() const { return( mpLink != NULL && mpLink->Len() > 0 ); }
	void					SetLink();
	void					SetLink( const String& rLink );
	String					GetLink() const;

	BOOL					HasUserData() const { return( mpUserData != NULL && mpUserData->Len() > 0 ); }
	void					SetUserData();
	void					SetUserData( const String& rUserData );
	String					GetUserData() const;

	ByteString				GetUniqueID() const;

	GraphicType				GetType() const { return meType; }
	const Size&				GetPrefSize() const { return maPrefSize; }
	const MapMode&			GetPrefMapMode() const { return maPrefMapMode; }
	ULONG					GetSizeBytes() const { return mnSizeBytes; }
	ULONG					GetChecksum() const;
	BOOL					IsTransparent() const { return mbTransparent; }
	BOOL					IsAlpha() const { return mbAlpha; }
	BOOL					IsAnimated() const { return mbAnimated; }
	BOOL					IsEPS() const { return mbEPS; }

	void					ResetAnimationLoopCount();
	List*					GetAnimationInfoList() const;
	Link					GetAnimationNotifyHdl() const { return maGraphic.GetAnimationNotifyHdl(); }
	void					SetAnimationNotifyHdl( const Link& rLink );

	BOOL					SwapOut();
	BOOL					SwapOut( SvStream* pOStm );
	BOOL					SwapIn();
	BOOL					SwapIn( SvStream* pIStm );

	BOOL					IsInSwapIn() const { return mbIsInSwapIn; }
	BOOL					IsInSwapOut() const { return mbIsInSwapOut; }
	BOOL					IsInSwap() const { return( mbIsInSwapOut || mbIsInSwapOut ); }
	BOOL					IsSwappedOut() const { return( mbAutoSwapped || maGraphic.IsSwapOut() ); }
	void					SetSwapState();

	BOOL					Draw( OutputDevice* pOut, const Point& rPt, const Size& rSz,
								  const GraphicAttr* pAttr = NULL, ULONG nFlags = GRFMGR_DRAW_STANDARD );

    /** Draw the graphic repeatedly into the given output rectangle

    	@param pOut
        OutputDevice where the rendering should take place

        @param rArea
        The output area that is filled with tiled instances of this graphic

        @param rSize
        The actual size of a single tile

        @param rOffset
        Offset from the left, top position of rArea, where to start
        the tiling. The upper left corner of the graphic tilings will
        virtually start at this position. Concretely, only that many
        tiles are drawn to completely fill the given output area.

        @param pAttr
        Optional GraphicAttr

        @param nFlags
        Optional rendering flags

        @param nTileCacheSize1D
        Optional dimension of the generated cache tiles. The pOut sees
        a number of tile draws, which have approximately
        nTileCacheSize1D times nTileCacheSize1D bitmap sizes if the
        tile bitmap is smaller. Otherwise, the tile is drawn as
        is. This is useful if e.g. you want only a few, very large
        bitmap drawings appear on the outdev.

        @return TRUE, if drawing completed successfully
     */
	BOOL					DrawTiled( OutputDevice* pOut, const Rectangle& rArea, const Size& rSize,
                                       const Size& rOffset, const GraphicAttr* pAttr = NULL, 
                                       ULONG nFlags = GRFMGR_DRAW_STANDARD, int nTileCacheSize1D=128 );

	BOOL					StartAnimation( OutputDevice* pOut, const Point& rPt, const Size& rSz, long nExtraData = 0L,
											const GraphicAttr* pAttr = NULL, ULONG nFlags = GRFMGR_DRAW_STANDARD,
											OutputDevice* pFirstFrameOutDev = NULL );

	void					StopAnimation( OutputDevice* pOut = NULL, long nExtraData = 0L );

    friend SvStream&		operator<<( SvStream& rOStm, const GraphicObject& rGraphicObj );
    friend SvStream&		operator>>( SvStream& rIStm, GraphicObject& rGraphicObj );
};

// ------------------
// - GraphicManager -
// ------------------

class GraphicManager
{
	friend class GraphicObject;
	friend class GraphicDisplayCacheEntry;	

private:

	List			maObjList;
	GraphicCache*	mpCache;

					GraphicManager( const GraphicManager& ) {}
	GraphicManager&	operator=( const GraphicManager& ) { return *this; }

//#if 0 // _SOLAR__PRIVATE

	BOOL			ImplDraw( OutputDevice* pOut, const Point& rPt,
							  const Size& rSz, GraphicObject& rObj,
							  const GraphicAttr& rAttr, 
                              const ULONG nFlags, BOOL& rCached );

	BOOL			ImplCreateOutput( OutputDevice* pOut, const Point& rPt, const Size& rSz,
									  const BitmapEx& rBmpEx, const GraphicAttr& rAttr,
                                      const ULONG nFlags, BitmapEx* pBmpEx = NULL );
    BOOL            ImplCreateOutput( OutputDevice* pOut,
                                      const Point& rPt, const Size& rSz,
                                      const GDIMetaFile& rMtf, const GraphicAttr& rAttr,
                                      const ULONG nFlags, GDIMetaFile& rOutMtf, BitmapEx& rOutBmpEx );

	BOOL			ImplCreateScaled( const BitmapEx& rBmpEx,
									  long* pMapIX, long* pMapFX, long* pMapIY, long* pMapFY,
									  long nStartX, long nEndX, long nStartY, long nEndY,
                                      BitmapEx& rOutBmpEx );

	BOOL			ImplCreateRotatedScaled( const BitmapEx& rBmpEx,
											 USHORT nRot10, const Size& rOutSzPix, const Size& rUntSzPix,
											 long* pMapIX, long* pMapFX, long* pMapIY, long* pMapFY,
											 long nStartX, long nEndX, long nStartY, long nEndY,
                                             BitmapEx& rOutBmpEx );

	static void		ImplAdjust( BitmapEx& rBmpEx, const GraphicAttr& rAttr, ULONG nAdjustmentFlags );
	static void		ImplAdjust( GDIMetaFile& rMtf, const GraphicAttr& rAttr, ULONG nAdjustmentFlags );
	static void		ImplAdjust( Animation& rAnimation, const GraphicAttr& rAttr, ULONG nAdjustmentFlags );

	static void		ImplDraw( OutputDevice* pOut, const Point& rPt, const Size& rSz,
							  const GDIMetaFile& rMtf, const GraphicAttr& rAttr );

					// Only used by GraphicObject's Ctor's and Dtor's
	void			ImplRegisterObj( const GraphicObject& rObj, Graphic& rSubstitute, 
                                     const ByteString* pID = NULL,
                                     const GraphicObject* pCopyObj = NULL );
	void			ImplUnregisterObj( const GraphicObject& rObj );
	inline BOOL		ImplHasObjects() const { return( maObjList.Count() > 0UL ); }

					// Only used in swap case by GraphicObject
	void			ImplGraphicObjectWasSwappedOut( const GraphicObject& rObj );
	BOOL			ImplFillSwappedGraphicObject( const GraphicObject& rObj, Graphic& rSubstitute );
	void			ImplGraphicObjectWasSwappedIn( const GraphicObject& rObj );

	ByteString		ImplGetUniqueID( const GraphicObject& rObj ) const;

//#endif // _SOLAR__PRIVATE

public:

					GraphicManager( ULONG nCacheSize = 10000000UL, ULONG nMaxObjCacheSize = 2400000UL );
					~GraphicManager();

	void			SetMaxCacheSize( ULONG nNewCacheSize );
	ULONG			GetMaxCacheSize() const;

	void			SetMaxObjCacheSize( ULONG nNewMaxObjSize, BOOL bDestroyGreaterCached = FALSE );
	ULONG			GetMaxObjCacheSize() const;

	ULONG			GetUsedCacheSize() const;
	ULONG			GetFreeCacheSize() const;

    void            SetCacheTimeout( ULONG nTimeoutSeconds );
    ULONG           GetCacheTimeout() const;

	void			ClearCache();

	void			ReleaseFromCache( const GraphicObject& rObj );

	BOOL			IsInCache( OutputDevice* pOut, const Point& rPt, const Size& rSz, 
							   const GraphicObject& rObj, const GraphicAttr& rAttr ) const;

	BOOL			DrawObj( OutputDevice* pOut, const Point& rPt, const Size& rSz,
							 GraphicObject& rObj, const GraphicAttr& rAttr,
							 const ULONG nFlags, BOOL& rCached );
};

#endif // _GRFMGR_HXX

