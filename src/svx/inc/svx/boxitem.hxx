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
#ifndef _SVX_BOXITEM_HXX
#define _SVX_BOXITEM_HXX

#include <svtools/poolitem.hxx>
#include <svx/borderline.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include "svx/svxdllapi.h"

namespace rtl { class OUString; }

// class SvxBoxItem ------------------------------------------------------

/*
[Beschreibung]
Dieses Item beschreibt ein Umrandungsattribut (alle vier Kanten und
Abstand nach innen.
*/

#define BOX_LINE_TOP	((USHORT)0)
#define BOX_LINE_BOTTOM	((USHORT)1)
#define BOX_LINE_LEFT	((USHORT)2)
#define BOX_LINE_RIGHT	((USHORT)3)

#define BOX_4DISTS_VERSION ((USHORT)1)

class SVX_DLLPUBLIC SvxBoxItem : public SfxPoolItem
{
	SvxBorderLine  *pTop,
				   *pBottom,
				   *pLeft,
				   *pRight;
	USHORT			nTopDist,
					nBottomDist,
					nLeftDist,
					nRightDist;

public:
	TYPEINFO();

    SvxBoxItem( const USHORT nId );
	SvxBoxItem( const SvxBoxItem &rCpy );
	~SvxBoxItem();
	SvxBoxItem &operator=( const SvxBoxItem& rBox );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual int 			 operator==( const SfxPoolItem& ) const;
	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*	 Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	 Create(SvStream &, USHORT) const;
	virtual SvStream&		 Store(SvStream &, USHORT nItemVersion ) const;
	virtual USHORT			 GetVersion( USHORT nFileVersion ) const;

	virtual int				 ScaleMetrics( long nMult, long nDiv );
	virtual	int				 HasMetrics() const;

	const	SvxBorderLine* GetTop()    const { return pTop; }
	const	SvxBorderLine* GetBottom() const { return pBottom; }
	const	SvxBorderLine* GetLeft()   const { return pLeft; }
	const	SvxBorderLine* GetRight()  const { return pRight; }

	const	SvxBorderLine* GetLine( USHORT nLine ) const;

		//Die Pointer werden kopiert!
	void	SetLine( const SvxBorderLine* pNew, USHORT nLine );

	USHORT	GetDistance( USHORT nLine ) const;
	USHORT	GetDistance() const;

	void 	SetDistance( USHORT nNew, USHORT nLine );
	inline void SetDistance( USHORT nNew );

		//Breite der Linien plus Zwischenraum plus Abstand nach innen.
		//JP 09.06.99: bIgnoreLine = TRUE -> Distance auch returnen, wenn
		//							keine Line gesetzt ist
	USHORT 	CalcLineSpace( USHORT nLine, BOOL bIgnoreLine = FALSE ) const;
};

inline void SvxBoxItem::SetDistance( USHORT nNew )
{
	nTopDist = nBottomDist = nLeftDist = nRightDist = nNew;
}

// class SvxBoxInfoItem --------------------------------------------------

/*
[Beschreibung]
Noch ein Item fuer die Umrandung. Dieses Item hat lediglich SS-Funktionalitaet.
Einerseits wird dem allgemeinen Dialog mit diesem Item mitgeteilt, welche
Moeglichkeiten er anbieten soll.
Andererseits werden ueber dieses Attribut ggf. die BorderLines fuer die
horizontalen und vertikalen innerern Linien transportiert.
*/

#define BOXINFO_LINE_HORI	((USHORT)0)
#define BOXINFO_LINE_VERT	((USHORT)1)

#define VALID_TOP			0x01
#define VALID_BOTTOM		0x02
#define VALID_LEFT			0x04
#define VALID_RIGHT			0x08
#define VALID_HORI			0x10
#define VALID_VERT			0x20
#define VALID_DISTANCE		0x40
#define VALID_DISABLE		0x80

class SVX_DLLPUBLIC SvxBoxInfoItem : public SfxPoolItem
{
	SvxBorderLine* pHori;   //innere horizontale Linie
	SvxBorderLine* pVert;   //innere vertikale Linie

    bool                mbEnableHor;   /// true = Enable inner horizonal line.
    bool                mbEnableVer;   /// true = Enable inner vertical line.

	/*
	 z.Z. nur fuer StarWriter: Abstand nach innen von SvxBoxItem.
	 Wenn der Abstand gewuenscht ist, so muss das Feld fuer den Abstand vom
	 Dialog freigeschaltet werden. nDefDist ist als Defaultwert anzusehen.
	 Wenn irgendeine	Linie eingeschalt ist oder wird, so muss dieser
	 Abstand defaultet werden. bMinDist gibt an, ob der Wert durch den
	 Anwender unterschritten werden darf. Mit nDist wird der aktuelle
	 Abstand von der App zum Dialog und zurueck transportiert.
	*/

	BOOL	bDist      :1;  // TRUE, Abstand freischalten.
	BOOL	bMinDist   :1;  // TRUE, Abstand darf nicht unterschritten werden.

	BYTE	nValidFlags;	// 0000 0000
							// ���� ����� VALID_TOP
							// ���� ����� VALID_BOTTOM
							// ���� ����� VALID_LEFT
							// ���� ����� VALID_RIGHT
							// ���������� VALID_HORI
							// ���������� VALID_VERT
							// ���������� VALID_DIST
							// ���������� VALID_DISABLE

	USHORT	nDefDist;       // Der Default- bzw. Minimalabstand.

public:
	TYPEINFO();

    SvxBoxInfoItem( const USHORT nId );
	SvxBoxInfoItem( const SvxBoxInfoItem &rCpy );
	~SvxBoxInfoItem();
	SvxBoxInfoItem &operator=( const SvxBoxInfoItem &rCpy );

	// "pure virtual Methoden" vom SfxPoolItem
	virtual int 			 operator==( const SfxPoolItem& ) const;
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;
	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	Create(SvStream &, USHORT) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion ) const;
	virtual int				 ScaleMetrics( long nMult, long nDiv );
	virtual	int				 HasMetrics() const;

	const SvxBorderLine*	GetHori() const { return pHori; }
	const SvxBorderLine*	GetVert() const { return pVert; }

	//Die Pointer werden kopiert!
	void					SetLine( const SvxBorderLine* pNew, USHORT nLine );

    BOOL    IsTable() const             { return mbEnableHor && mbEnableVer; }
    void    SetTable( BOOL bNew )       { mbEnableHor = mbEnableVer = bNew; }

    inline bool         IsHorEnabled() const { return mbEnableHor; }
    inline void         EnableHor( bool bEnable ) { mbEnableHor = bEnable; }
    inline bool         IsVerEnabled() const { return mbEnableVer; }
    inline void         EnableVer( bool bEnable ) { mbEnableVer = bEnable; }

	BOOL 	IsDist() const				{ return bDist; }
	void 	SetDist( BOOL bNew )		{ bDist = bNew; }
	BOOL 	IsMinDist() const			{ return bMinDist; }
	void 	SetMinDist( BOOL bNew )		{ bMinDist = bNew; }
	USHORT	GetDefDist() const			{ return nDefDist; }
	void 	SetDefDist( USHORT nNew )	{ nDefDist = nNew; }

	BOOL					IsValid( BYTE nValid ) const
								{ return ( nValidFlags & nValid ) == nValid; }
	void 					SetValid( BYTE nValid, BOOL bValid = TRUE )
								{ bValid ? ( nValidFlags |= nValid )
										 : ( nValidFlags &= ~nValid ); }
	void					ResetFlags();
};
#endif

