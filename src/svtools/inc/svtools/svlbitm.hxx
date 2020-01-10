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


#ifndef _SVLBOXITM_HXX
#define _SVLBOXITM_HXX

#include "svtools/svtdllapi.h"

#ifndef LINK_HXX
#include <tools/link.hxx>
#endif

#ifndef _IMAGE_HXX
#include <vcl/image.hxx>
#endif
#include <svtools/svlbox.hxx>

class SvLBoxEntry;

#define SV_ITEM_ID_LBOXSTRING		1
#define SV_ITEM_ID_LBOXBMP			2
#define SV_ITEM_ID_LBOXBUTTON		3
#define SV_ITEM_ID_LBOXCONTEXTBMP	4

enum SvButtonState { SV_BUTTON_UNCHECKED, SV_BUTTON_CHECKED, SV_BUTTON_TRISTATE };

#define SV_BMP_UNCHECKED		0
#define SV_BMP_CHECKED   		1
#define SV_BMP_TRISTATE  		2
#define SV_BMP_HIUNCHECKED 		3
#define SV_BMP_HICHECKED   		4
#define SV_BMP_HITRISTATE  		5
#define SV_BMP_STATICIMAGE      6

struct SvLBoxButtonData_Impl;

class SVT_DLLPUBLIC SvLBoxButtonData
{
private:
	Link 					aLink;
	long 					nWidth;
	long					nHeight;
	SvLBoxButtonData_Impl*	pImpl;
	BOOL					bDataOk;
	SvButtonState			eState;

	SVT_DLLPRIVATE void 					SetWidthAndHeight();
	SVT_DLLPRIVATE void					InitData( BOOL bImagesFromDefault,
									  bool _bRadioBtn, const Control* pControlForSettings = NULL );
public:
							// include creating default images (CheckBox or RadioButton)
							SvLBoxButtonData( const Control* pControlForSettings );
							SvLBoxButtonData( const Control* pControlForSettings, bool _bRadioBtn );

							SvLBoxButtonData();
							~SvLBoxButtonData();

	USHORT 					GetIndex( USHORT nItemState );
	inline long				Width();
	inline long				Height();
	void					SetLink( const Link& rLink) { aLink=rLink; }
	const Link&				GetLink() const { return aLink; }
 	BOOL					IsRadio();
	// weil Buttons nicht von LinkHdl abgeleitet sind
	void					CallLink();

	void					StoreButtonState( SvLBoxEntry* pEntry, USHORT nItemFlags );
	SvButtonState 			ConvertToButtonState( USHORT nItemFlags ) const;

	inline SvButtonState	GetActButtonState() const;
	SvLBoxEntry*			GetActEntry() const;

	Image aBmps[24];  // Indizes siehe Konstanten BMP_ ....

	void					SetDefaultImages( const Control* pControlForSettings = NULL );
								// set images acording to the color scheeme of the Control
								// pControlForSettings == NULL: settings are taken from Application
	BOOL					HasDefaultImages( void ) const;
};

inline long SvLBoxButtonData::Width()
{
	if ( !bDataOk )
		SetWidthAndHeight();
	return nWidth;
}

inline long SvLBoxButtonData::Height()
{
	if ( !bDataOk )
		SetWidthAndHeight();
	return nHeight;
}

inline SvButtonState SvLBoxButtonData::GetActButtonState() const
{
	return eState;
}

// **********************************************************************

class SVT_DLLPUBLIC SvLBoxString : public SvLBoxItem
{
	XubString aStr;
public:
					SvLBoxString( SvLBoxEntry*,USHORT nFlags,const XubString& rStr);
					SvLBoxString();
	virtual			~SvLBoxString();
	virtual USHORT	IsA();
	void			InitViewData( SvLBox*,SvLBoxEntry*,SvViewDataItem* );
	XubString		GetText() const { return aStr; }
	void 			SetText( SvLBoxEntry*, const XubString& rStr );
	void			Paint( const Point&, SvLBox& rDev, USHORT nFlags,SvLBoxEntry* );
	SvLBoxItem* 	Create() const;
	void 			Clone( SvLBoxItem* pSource );
};

class SvLBoxBmp : public SvLBoxItem
{
	Image aBmp;
public:
					SvLBoxBmp( SvLBoxEntry*, USHORT nFlags, Image );
					SvLBoxBmp();
	virtual			~SvLBoxBmp();
	virtual USHORT	IsA();
	void			InitViewData( SvLBox*,SvLBoxEntry*,SvViewDataItem* );
	void			SetBitmap( SvLBoxEntry*, Image );
	void			Paint( const Point&, SvLBox& rView, USHORT nFlags,SvLBoxEntry* );
	SvLBoxItem*	 	Create() const;
	void 			Clone( SvLBoxItem* pSource );
};


#define SV_ITEMSTATE_UNCHECKED			0x0001
#define SV_ITEMSTATE_CHECKED			0x0002
#define SV_ITEMSTATE_TRISTATE			0x0004
#define SV_ITEMSTATE_HILIGHTED			0x0008
#define SV_STATE_MASK 0xFFF8  // zum Loeschen von UNCHECKED,CHECKED,TRISTATE

enum SvLBoxButtonKind
{
    SvLBoxButtonKind_enabledCheckbox,
    SvLBoxButtonKind_disabledCheckbox,
    SvLBoxButtonKind_staticImage
};

class SVT_DLLPUBLIC SvLBoxButton : public SvLBoxItem
{
	SvLBoxButtonData*	pData;
    SvLBoxButtonKind eKind;
	USHORT nItemFlags;
	USHORT nImgArrOffs;
	USHORT nBaseOffs;
public:
                    // An SvLBoxButton can be of three different kinds: an
                    // enabled checkbox (the normal kind), a disabled checkbox
                    // (which cannot be modified via UI), or a static image
                    // (see SV_BMP_STATICIMAGE; nFlags are effectively ignored
                    // for that kind).
					SvLBoxButton( SvLBoxEntry* pEntry,
                                  SvLBoxButtonKind eTheKind, USHORT nFlags,
                                  SvLBoxButtonData* pBData );
					SvLBoxButton();
	virtual			~SvLBoxButton();
	void			InitViewData( SvLBox*,SvLBoxEntry*,SvViewDataItem* );
	virtual USHORT	IsA();
	void 			Check( SvLBox* pView, SvLBoxEntry*, BOOL bCheck );
	virtual BOOL 	ClickHdl(SvLBox* pView, SvLBoxEntry* );
	void			Paint( const Point&, SvLBox& rView, USHORT nFlags,SvLBoxEntry* );
	SvLBoxItem*	 	Create() const;
	void 			Clone( SvLBoxItem* pSource );
	USHORT			GetButtonFlags() const { return nItemFlags; }
	BOOL			IsStateChecked() const { return (BOOL)(nItemFlags & SV_ITEMSTATE_CHECKED)!=0; }
	BOOL			IsStateUnchecked() const { return (BOOL)(nItemFlags & SV_ITEMSTATE_UNCHECKED)!=0; }
	BOOL			IsStateTristate() const { return (BOOL)(nItemFlags & SV_ITEMSTATE_TRISTATE)!=0; }
	BOOL			IsStateHilighted() const { return (BOOL)(nItemFlags & SV_ITEMSTATE_HILIGHTED)!=0; }
	void			SetStateChecked();
	void			SetStateUnchecked();
	void			SetStateTristate();
	void			SetStateHilighted( BOOL bHilight );

    SvLBoxButtonKind GetKind() const { return eKind; }

	void			SetBaseOffs( USHORT nOffs ) { nBaseOffs = nOffs; }
	USHORT			GetBaseOffs() const { return nBaseOffs; }

    // Check whether this button can be modified via UI, sounding a beep if it
    // cannot be modified:
    bool            CheckModification() const;
};

inline void	SvLBoxButton::SetStateChecked()
{
	nItemFlags &= SV_STATE_MASK;
	nItemFlags |= SV_ITEMSTATE_CHECKED;
}
inline void	SvLBoxButton::SetStateUnchecked()
{
	nItemFlags &= SV_STATE_MASK;
	nItemFlags |= SV_ITEMSTATE_UNCHECKED;
}
inline void	SvLBoxButton::SetStateTristate()
{
	nItemFlags &= SV_STATE_MASK;
	nItemFlags |= SV_ITEMSTATE_TRISTATE;
}
inline void SvLBoxButton::SetStateHilighted( BOOL bHilight )
{
	if ( bHilight )
		nItemFlags |= SV_ITEMSTATE_HILIGHTED;
	else
		nItemFlags &= ~SV_ITEMSTATE_HILIGHTED;
}


struct SvLBoxContextBmp_Impl;
class SVT_DLLPUBLIC SvLBoxContextBmp : public SvLBoxItem
{
	SvLBoxContextBmp_Impl*	m_pImpl;
public:
					SvLBoxContextBmp( SvLBoxEntry*,USHORT nFlags,Image,Image,
									USHORT nEntryFlagsBmp1);
					SvLBoxContextBmp();
	virtual			~SvLBoxContextBmp();
	virtual USHORT	IsA();
	void			InitViewData( SvLBox*,SvLBoxEntry*,SvViewDataItem* );
	void			Paint( const Point&, SvLBox& rView, USHORT nFlags,SvLBoxEntry* );
	SvLBoxItem*	 	Create() const;
	void 			Clone( SvLBoxItem* pSource );


	BOOL			SetModeImages( const Image& _rBitmap1, const Image& _rBitmap2, BmpColorMode _eMode = BMP_COLOR_NORMAL );
	void			GetModeImages(		 Image& _rBitmap1,		 Image& _rBitmap2, BmpColorMode _eMode = BMP_COLOR_NORMAL ) const;

	inline void			SetBitmap1( const Image& _rImage, BmpColorMode _eMode = BMP_COLOR_NORMAL );
	inline void			SetBitmap2( const Image& _rImage, BmpColorMode _eMode = BMP_COLOR_NORMAL );
	inline const Image&	GetBitmap1( BmpColorMode _eMode = BMP_COLOR_NORMAL ) const;
	inline const Image&	GetBitmap2( BmpColorMode _eMode = BMP_COLOR_NORMAL ) const;

private:
	Image& implGetImageStore( sal_Bool _bFirst, BmpColorMode _eMode );
};

inline void SvLBoxContextBmp::SetBitmap1( const Image& _rImage, BmpColorMode _eMode  )
{
	implGetImageStore( sal_True, _eMode ) = _rImage;
}

inline void	SvLBoxContextBmp::SetBitmap2( const Image& _rImage, BmpColorMode _eMode  )
{
	implGetImageStore( sal_False, _eMode ) = _rImage;
}

inline const Image&	SvLBoxContextBmp::GetBitmap1( BmpColorMode _eMode ) const
{
	Image& rImage = const_cast< SvLBoxContextBmp* >( this )->implGetImageStore( sal_True, _eMode );
	if ( !rImage )
		// fallback to the "normal" image
		rImage = const_cast< SvLBoxContextBmp* >( this )->implGetImageStore( sal_True, BMP_COLOR_NORMAL );
	return rImage;
}

inline const Image&	SvLBoxContextBmp::GetBitmap2( BmpColorMode _eMode ) const
{
	Image& rImage = const_cast< SvLBoxContextBmp* >( this )->implGetImageStore( sal_False, _eMode );
	if ( !rImage )
		// fallback to the "normal" image
		rImage = const_cast< SvLBoxContextBmp* >( this )->implGetImageStore( sal_True, BMP_COLOR_NORMAL );
	return rImage;
}

#endif
