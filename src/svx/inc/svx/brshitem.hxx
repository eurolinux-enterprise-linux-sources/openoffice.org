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
#ifndef _SVX_BRSHITEM_HXX
#define _SVX_BRSHITEM_HXX

// include ---------------------------------------------------------------

#include <svtools/poolitem.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include <vcl/wall.hxx>
#include <tools/link.hxx>
#include "svx/svxdllapi.h"

// class SvxBrushItem ----------------------------------------------------

class Graphic;
class GraphicObject;
class SfxObjectShell;
class CntWallpaperItem;
namespace rtl
{
	class OUString;
}

#define	BRUSH_GRAPHIC_VERSION	((USHORT)0x0001)

enum SvxGraphicPosition
{
	GPOS_NONE,
	GPOS_LT, GPOS_MT, GPOS_RT,
	GPOS_LM, GPOS_MM, GPOS_RM,
	GPOS_LB, GPOS_MB, GPOS_RB,
	GPOS_AREA, GPOS_TILED
};

#define PARA_DEST_PARA	0
#define PARA_DEST_CHAR  1

class SvxBrushItem_Impl;
class SVX_DLLPUBLIC SvxBrushItem : public SfxPoolItem
{
	Color 				aColor;
	SvxBrushItem_Impl*  pImpl;
	String*				pStrLink;
	String*				pStrFilter;
	SvxGraphicPosition	eGraphicPos;
	BOOL				bLoadAgain;

    void        ApplyGraphicTransparency_Impl();
    DECL_STATIC_LINK( SvxBrushItem, DoneHdl_Impl, void *);
	// wird nur von Create benutzt
	SvxBrushItem( SvStream& rStrm,
                  USHORT nVersion, USHORT nWhich  );

public:
	TYPEINFO();

    SvxBrushItem( USHORT nWhich );
    SvxBrushItem( const Color& rColor, USHORT nWhich  );

	SvxBrushItem( const Graphic& rGraphic,
                  SvxGraphicPosition ePos, USHORT nWhich );
	SvxBrushItem( const GraphicObject& rGraphicObj,
                  SvxGraphicPosition ePos, USHORT nWhich );
	SvxBrushItem( const String& rLink, const String& rFilter,
                  SvxGraphicPosition ePos, USHORT nWhich );
	SvxBrushItem( const SvxBrushItem& );
	SvxBrushItem( const CntWallpaperItem&, USHORT nWhich );

	~SvxBrushItem();

public:

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual int 			 operator==( const SfxPoolItem& ) const;
	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	virtual SfxPoolItem*     Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	 Create( SvStream&, USHORT nVersion ) const;
	virtual SvStream&		 Store( SvStream& , USHORT nItemVersion ) const;
	virtual USHORT			 GetVersion( USHORT nFileVersion ) const;

	const Color& 	GetColor() const 				{ return aColor; }
	Color& 			GetColor()  					{ return aColor; }
	void			SetColor( const Color& rCol)  	{ aColor = rCol; }

	void                SetDoneLink( const Link& rLink );

	SvxGraphicPosition	GetGraphicPos() const		{ return eGraphicPos; }

	void                PurgeGraphic() const;
	void                PurgeMedium() const;

	const Graphic* 			GetGraphic( SfxObjectShell* pSh = 0) const;
	const GraphicObject* 	GetGraphicObject( SfxObjectShell* pSh = 0) const;
	const String* 			GetGraphicLink() const		{ return pStrLink; }
	const String* 			GetGraphicFilter() const	{ return pStrFilter; }

	void				SetGraphicPos( SvxGraphicPosition eNew );
	void 				SetGraphic( const Graphic& rNew );
	void 				SetGraphicObject( const GraphicObject& rNewObj );
	void		 		SetGraphicLink( const String& rNew );
	void		 		SetGraphicFilter( const String& rNew );

	SvxBrushItem&		operator=( const SvxBrushItem& rItem);

	//static void					InitSfxLink();
	static SvxGraphicPosition 	WallpaperStyle2GraphicPos( WallpaperStyle eStyle );
	static WallpaperStyle 		GraphicPos2WallpaperStyle( SvxGraphicPosition ePos );
	CntWallpaperItem*			CreateCntWallpaperItem() const;
};

#endif // #ifndef _SVX_BRSHITEM_HXX

