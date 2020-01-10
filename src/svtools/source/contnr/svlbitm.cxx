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


#include <svtools/svlbox.hxx>
#include <svtools/svlbitm.hxx>
#include <vcl/svapp.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <vcl/decoview.hxx>
#include <vcl/sound.hxx>
#include <vcl/salnativewidgets.hxx>

#define TABOFFS_NOT_VALID -2000000

struct SvLBoxButtonData_Impl
{
	SvLBoxEntry*	pEntry;
	BOOL			bDefaultImages;
	BOOL			bShowRadioButton;

	SvLBoxButtonData_Impl() : pEntry( NULL ), bDefaultImages( FALSE ), bShowRadioButton( FALSE ) {}
};


DBG_NAME(SvLBoxButtonData)

void SvLBoxButtonData::InitData( BOOL bImagesFromDefault, bool _bRadioBtn, const Control* pCtrl )
{
	pImpl = new SvLBoxButtonData_Impl;

	bDataOk = FALSE;
	eState = SV_BUTTON_UNCHECKED;
	pImpl->bDefaultImages = bImagesFromDefault;
	pImpl->bShowRadioButton = ( _bRadioBtn != false );

	if ( bImagesFromDefault )
		SetDefaultImages( pCtrl );
}

SvLBoxButtonData::SvLBoxButtonData( const Control* pControlForSettings )
{
	DBG_CTOR(SvLBoxButtonData,0);

	InitData( TRUE, false, pControlForSettings );
}

SvLBoxButtonData::SvLBoxButtonData( const Control* pControlForSettings, bool _bRadioBtn )
{
	DBG_CTOR(SvLBoxButtonData,0);

	InitData( TRUE, _bRadioBtn, pControlForSettings );
}

SvLBoxButtonData::SvLBoxButtonData()
{
	DBG_CTOR(SvLBoxButtonData,0);

	InitData( FALSE, false );
}

SvLBoxButtonData::~SvLBoxButtonData()
{
	DBG_DTOR(SvLBoxButtonData,0);

	delete pImpl;
#ifdef DBG_UTIL
	pImpl = NULL;
#endif
}

void SvLBoxButtonData::CallLink()
{
	DBG_CHKTHIS(SvLBoxButtonData,0);
	aLink.Call( this );
}

USHORT SvLBoxButtonData::GetIndex( USHORT nItemState )
{
	DBG_CHKTHIS(SvLBoxButtonData,0);
	nItemState &= 0x000F;
	USHORT nIdx;
	switch( nItemState )
	{
		case SV_ITEMSTATE_UNCHECKED:
				nIdx = SV_BMP_UNCHECKED; break;
		case SV_ITEMSTATE_CHECKED:
				nIdx = SV_BMP_CHECKED; break;
		case SV_ITEMSTATE_TRISTATE:
				nIdx = SV_BMP_TRISTATE; break;
		case SV_ITEMSTATE_UNCHECKED | SV_ITEMSTATE_HILIGHTED:
				nIdx = SV_BMP_HIUNCHECKED; break;
		case SV_ITEMSTATE_CHECKED | SV_ITEMSTATE_HILIGHTED:
				nIdx = SV_BMP_HICHECKED; break;
		case SV_ITEMSTATE_TRISTATE | SV_ITEMSTATE_HILIGHTED:
				nIdx = SV_BMP_HITRISTATE; break;
		default:
				nIdx = SV_BMP_UNCHECKED;
	}
	return nIdx;
}

void SvLBoxButtonData::SetWidthAndHeight()
{
	DBG_CHKTHIS(SvLBoxButtonData,0);
	Size aSize = aBmps[0].GetSizePixel();
	nWidth = aSize.Width();
	nHeight = aSize.Height();
	bDataOk = TRUE;
}


void SvLBoxButtonData::StoreButtonState( SvLBoxEntry* pActEntry, USHORT nItemFlags )
{
	DBG_CHKTHIS(SvLBoxButtonData,0);
	pImpl->pEntry = pActEntry;
	eState = ConvertToButtonState( nItemFlags );
}

SvButtonState SvLBoxButtonData::ConvertToButtonState( USHORT nItemFlags ) const
{
	DBG_CHKTHIS(SvLBoxButtonData,0);
	nItemFlags &= (SV_ITEMSTATE_UNCHECKED |
				   SV_ITEMSTATE_CHECKED |
				   SV_ITEMSTATE_TRISTATE);
	switch( nItemFlags )
	{
		case SV_ITEMSTATE_UNCHECKED:
			return SV_BUTTON_UNCHECKED;

		case SV_ITEMSTATE_CHECKED:
			return SV_BUTTON_CHECKED;

		case SV_ITEMSTATE_TRISTATE:
			return SV_BUTTON_TRISTATE;
		default:
			return SV_BUTTON_UNCHECKED;
	}
}

SvLBoxEntry* SvLBoxButtonData::GetActEntry() const
{
	DBG_ASSERT( pImpl, "-SvLBoxButtonData::GetActEntry(): don't use me that way!" );
	return pImpl->pEntry;
}

void SvLBoxButtonData::SetDefaultImages( const Control* pCtrl )
{
	const AllSettings& rSettings = pCtrl? pCtrl->GetSettings() : Application::GetSettings();

	if ( pImpl->bShowRadioButton )
	{
		aBmps[ SV_BMP_UNCHECKED ]	= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_DEFAULT );
		aBmps[ SV_BMP_CHECKED ]		= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_CHECKED );
		aBmps[ SV_BMP_HICHECKED ]	= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_CHECKED | BUTTON_DRAW_PRESSED );
		aBmps[ SV_BMP_HIUNCHECKED ]	= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_DEFAULT | BUTTON_DRAW_PRESSED );
		aBmps[ SV_BMP_TRISTATE ]	= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_DONTKNOW );
		aBmps[ SV_BMP_HITRISTATE ]	= RadioButton::GetRadioImage( rSettings, BUTTON_DRAW_DONTKNOW | BUTTON_DRAW_PRESSED );
	}
	else
	{
		aBmps[ SV_BMP_UNCHECKED ]	= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_DEFAULT );
		aBmps[ SV_BMP_CHECKED ]		= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_CHECKED );
		aBmps[ SV_BMP_HICHECKED ]	= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_CHECKED | BUTTON_DRAW_PRESSED );
		aBmps[ SV_BMP_HIUNCHECKED ]	= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_DEFAULT | BUTTON_DRAW_PRESSED );
		aBmps[ SV_BMP_TRISTATE ]	= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_DONTKNOW );
		aBmps[ SV_BMP_HITRISTATE ]	= CheckBox::GetCheckImage( rSettings, BUTTON_DRAW_DONTKNOW | BUTTON_DRAW_PRESSED );
	}
}

BOOL SvLBoxButtonData::HasDefaultImages( void ) const
{
	return pImpl->bDefaultImages;
}

BOOL SvLBoxButtonData::IsRadio() {
	return pImpl->bShowRadioButton; 
}

// ***************************************************************
// class SvLBoxString
// ***************************************************************

DBG_NAME(SvLBoxString);

SvLBoxString::SvLBoxString( SvLBoxEntry* pEntry,USHORT nFlags,const XubString& rStr) :
		SvLBoxItem( pEntry, nFlags )
{
	DBG_CTOR(SvLBoxString,0);
	SetText( pEntry, rStr );
}

SvLBoxString::SvLBoxString() : SvLBoxItem()
{
	DBG_CTOR(SvLBoxString,0);
}

SvLBoxString::~SvLBoxString()
{
	DBG_DTOR(SvLBoxString,0);
}

USHORT SvLBoxString::IsA()
{
	DBG_CHKTHIS(SvLBoxString,0);
	return SV_ITEM_ID_LBOXSTRING;
}

void SvLBoxString::Paint( const Point& rPos, SvLBox& rDev, USHORT /* nFlags */,
	SvLBoxEntry* _pEntry)
{
	DBG_CHKTHIS(SvLBoxString,0);
	if ( _pEntry )
	{
		USHORT nStyle = rDev.IsEnabled() ? 0 : TEXT_DRAW_DISABLE;
        if ( rDev.IsEntryMnemonicsEnabled() )
            nStyle |= TEXT_DRAW_MNEMONIC;
		rDev.DrawText( Rectangle(rPos,GetSize(&rDev,_pEntry)),aStr,nStyle);
	}
	else
		rDev.DrawText( rPos, aStr);
	
}

SvLBoxItem* SvLBoxString::Create() const
{
	DBG_CHKTHIS(SvLBoxString,0);
	return new SvLBoxString;
}

void SvLBoxString::Clone( SvLBoxItem* pSource )
{
	DBG_CHKTHIS(SvLBoxString,0);
	aStr = ((SvLBoxString*)pSource)->aStr;
}

void SvLBoxString::SetText( SvLBoxEntry*, const XubString& rStr )
{
	DBG_CHKTHIS(SvLBoxString,0);
	aStr = rStr;
}

void SvLBoxString::InitViewData( SvLBox* pView,SvLBoxEntry* pEntry,
	SvViewDataItem* pViewData)
{
	DBG_CHKTHIS(SvLBoxString,0);
	if( !pViewData )
		pViewData = pView->GetViewDataItem( pEntry, this );
	pViewData->aSize = Size(pView->GetTextWidth( aStr ), pView->GetTextHeight());
}

// ***************************************************************
// class SvLBoxBmp
// ***************************************************************

DBG_NAME(SvLBoxBmp);

SvLBoxBmp::SvLBoxBmp( SvLBoxEntry* pEntry, USHORT nFlags, Image aBitmap ) :
	SvLBoxItem( pEntry, nFlags )
{
	DBG_CTOR(SvLBoxBmp,0);
	SetBitmap( pEntry, aBitmap);
}

SvLBoxBmp::SvLBoxBmp() : SvLBoxItem()
{
	DBG_CTOR(SvLBoxBmp,0);
}

SvLBoxBmp::~SvLBoxBmp()
{
	DBG_DTOR(SvLBoxBmp,0);
}

USHORT SvLBoxBmp::IsA()
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	return SV_ITEM_ID_LBOXBMP;
}

void SvLBoxBmp::SetBitmap( SvLBoxEntry*, Image aBitmap)
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	aBmp = aBitmap;
}

void SvLBoxBmp::InitViewData( SvLBox* pView,SvLBoxEntry* pEntry,
	SvViewDataItem* pViewData)
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	if( !pViewData )
		pViewData = pView->GetViewDataItem( pEntry, this );
	pViewData->aSize = aBmp.GetSizePixel();
}

void SvLBoxBmp::Paint( const Point& rPos, SvLBox& rDev, USHORT /* nFlags */,
						SvLBoxEntry* )
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	USHORT nStyle = rDev.IsEnabled() ? 0 : IMAGE_DRAW_DISABLE;
	rDev.DrawImage( rPos, aBmp ,nStyle);
}

SvLBoxItem* SvLBoxBmp::Create() const
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	return new SvLBoxBmp;
}

void SvLBoxBmp::Clone( SvLBoxItem* pSource )
{
	DBG_CHKTHIS(SvLBoxBmp,0);
	aBmp = ((SvLBoxBmp*)pSource)->aBmp;
}

// ***************************************************************
// class SvLBoxButton
// ***************************************************************

DBG_NAME(SvLBoxButton);

SvLBoxButton::SvLBoxButton( SvLBoxEntry* pEntry, SvLBoxButtonKind eTheKind,
                            USHORT nFlags, SvLBoxButtonData* pBData )
	: SvLBoxItem( pEntry, nFlags )
{
	DBG_CTOR(SvLBoxButton,0);
    eKind = eTheKind;
	nBaseOffs = 0;
	nItemFlags = 0;
	SetStateUnchecked();
	pData = pBData;
}

SvLBoxButton::SvLBoxButton() : SvLBoxItem()
{
	DBG_CTOR(SvLBoxButton,0);
    eKind = SvLBoxButtonKind_enabledCheckbox;
	nItemFlags = 0;
	SetStateUnchecked();
}

SvLBoxButton::~SvLBoxButton()
{
	DBG_DTOR(SvLBoxButton,0);
}

USHORT SvLBoxButton::IsA()
{
	DBG_CHKTHIS(SvLBoxButton,0);
	return SV_ITEM_ID_LBOXBUTTON;
}

void SvLBoxButton::Check(SvLBox*, SvLBoxEntry*, BOOL bOn)
{
	DBG_CHKTHIS(SvLBoxButton,0);
	if ( bOn != IsStateChecked() )
	{
		if ( bOn )
			SetStateChecked();
		else
			SetStateUnchecked();
	}
}

BOOL SvLBoxButton::ClickHdl( SvLBox*, SvLBoxEntry* pEntry )
{
	DBG_CHKTHIS(SvLBoxButton,0);
    if ( CheckModification() )
    {
        if ( IsStateChecked() )
            SetStateUnchecked();
        else
            SetStateChecked();
        pData->StoreButtonState( pEntry, nItemFlags );
        pData->CallLink();
    }
	return FALSE;
}

void SvLBoxButton::Paint( const Point& rPos, SvLBox& rDev, USHORT /* nFlags */,
							SvLBoxEntry* )
{
	DBG_CHKTHIS(SvLBoxButton,0);
	USHORT nIndex = eKind == SvLBoxButtonKind_staticImage
        ? SV_BMP_STATICIMAGE : pData->GetIndex( nItemFlags );
	USHORT nStyle = eKind != SvLBoxButtonKind_disabledCheckbox &&
        rDev.IsEnabled() ? 0 : IMAGE_DRAW_DISABLE;
        
///
//Native drawing
///
    BOOL bNativeOK = FALSE;
    Window *pWin = NULL;
    if( rDev.GetOutDevType() == OUTDEV_WINDOW )
        pWin = (Window*) &rDev;
        
    if ( nIndex != SV_BMP_STATICIMAGE && pWin && pWin->IsNativeControlSupported( (pData->IsRadio())? CTRL_RADIOBUTTON : CTRL_CHECKBOX, PART_ENTIRE_CONTROL) )
    {
        ImplControlValue    aControlValue;
        Region              aCtrlRegion( Rectangle(rPos, Size(pData->Width(), pData->Height())) );
        ControlState        nState = 0;
        
        //states CTRL_STATE_DEFAULT, CTRL_STATE_PRESSED and CTRL_STATE_ROLLOVER are not implemented
        if ( IsStateHilighted() ) 					nState |= CTRL_STATE_FOCUSED;
        if ( nStyle != IMAGE_DRAW_DISABLE )			nState |= CTRL_STATE_ENABLED;

        if ( IsStateChecked() )
            aControlValue.setTristateVal( BUTTONVALUE_ON );
        else if ( IsStateUnchecked() )
            aControlValue.setTristateVal( BUTTONVALUE_OFF );
        else if ( IsStateTristate() )
            aControlValue.setTristateVal( BUTTONVALUE_MIXED );

        bNativeOK = pWin->DrawNativeControl( (pData->IsRadio())? CTRL_RADIOBUTTON : CTRL_CHECKBOX, PART_ENTIRE_CONTROL,
                                aCtrlRegion, nState, aControlValue, rtl::OUString() );
    }

    if( !bNativeOK) 
        rDev.DrawImage( rPos, pData->aBmps[nIndex + nBaseOffs] ,nStyle);
}

SvLBoxItem* SvLBoxButton::Create() const
{
	DBG_CHKTHIS(SvLBoxButton,0);
	return new SvLBoxButton;
}

void SvLBoxButton::Clone( SvLBoxItem* pSource )
{
	DBG_CHKTHIS(SvLBoxButton,0);
	pData = ((SvLBoxButton*)pSource)->pData;
}

void SvLBoxButton::InitViewData( SvLBox* pView,SvLBoxEntry* pEntry,
	SvViewDataItem* pViewData )
{
	DBG_CHKTHIS(SvLBoxButton,0);
	if( !pViewData )
		pViewData = pView->GetViewDataItem( pEntry, this );
	pViewData->aSize = Size( pData->Width(), pData->Height() );
}

bool SvLBoxButton::CheckModification() const
{
    if( eKind == SvLBoxButtonKind_disabledCheckbox )
        Sound::Beep();
    return eKind == SvLBoxButtonKind_enabledCheckbox;
}

// ***************************************************************
// class SvLBoxContextBmp
// ***************************************************************

struct SvLBoxContextBmp_Impl
{
	Image		m_aImage1;
	Image		m_aImage2;

	Image		m_aImage1_hc;
	Image		m_aImage2_hc;

	USHORT		m_nB2IndicatorFlags;
};

// ***************************************************************
DBG_NAME(SvLBoxContextBmp)

SvLBoxContextBmp::SvLBoxContextBmp( SvLBoxEntry* pEntry, USHORT nItemFlags,
	Image aBmp1, Image aBmp2, USHORT nEntryFlags )
	:SvLBoxItem( pEntry, nItemFlags )
	,m_pImpl( new SvLBoxContextBmp_Impl )
{
	DBG_CTOR(SvLBoxContextBmp,0);

	m_pImpl->m_nB2IndicatorFlags = nEntryFlags;
	SetModeImages( aBmp1, aBmp2 );
}

SvLBoxContextBmp::SvLBoxContextBmp()
	:SvLBoxItem( )
	,m_pImpl( new SvLBoxContextBmp_Impl )
{
	m_pImpl->m_nB2IndicatorFlags = 0;
	DBG_CTOR(SvLBoxContextBmp,0);
}

SvLBoxContextBmp::~SvLBoxContextBmp()
{
	delete m_pImpl;
	DBG_DTOR(SvLBoxContextBmp,0);
}

USHORT SvLBoxContextBmp::IsA()
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);
	return SV_ITEM_ID_LBOXCONTEXTBMP;
}

BOOL SvLBoxContextBmp::SetModeImages( const Image& _rBitmap1, const Image& _rBitmap2, BmpColorMode _eMode )
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);

	sal_Bool bSuccess = sal_True;
	switch ( _eMode )
	{
		case BMP_COLOR_NORMAL:
			m_pImpl->m_aImage1 = _rBitmap1;
			m_pImpl->m_aImage2 = _rBitmap2;
			break;

		case BMP_COLOR_HIGHCONTRAST:
			m_pImpl->m_aImage1_hc = _rBitmap1;
			m_pImpl->m_aImage2_hc = _rBitmap2;
			break;

		default:
			DBG_ERROR( "SvLBoxContextBmp::SetModeImages: unexpected mode!");
			bSuccess = sal_False;
			break;
	}
	return bSuccess;
}

Image& SvLBoxContextBmp::implGetImageStore( sal_Bool _bFirst, BmpColorMode _eMode )
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);

	switch ( _eMode )
	{
		case BMP_COLOR_NORMAL:
			return _bFirst ? m_pImpl->m_aImage1 : m_pImpl->m_aImage2;

		case BMP_COLOR_HIGHCONTRAST:
			return _bFirst ? m_pImpl->m_aImage1_hc : m_pImpl->m_aImage2_hc;

		default:
			DBG_ERROR( "SvLBoxContextBmp::implGetImageStore: unexpected mode!");
	}

	// OJ: #i27071# wrong mode so we just return the normal images
    return _bFirst ? m_pImpl->m_aImage1 : m_pImpl->m_aImage2;
}

void SvLBoxContextBmp::InitViewData( SvLBox* pView,SvLBoxEntry* pEntry,
	SvViewDataItem* pViewData)
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);
	if( !pViewData )
		pViewData = pView->GetViewDataItem( pEntry, this );
	pViewData->aSize = m_pImpl->m_aImage1.GetSizePixel();
}

void SvLBoxContextBmp::Paint( const Point& _rPos, SvLBox& _rDev,
	USHORT _nViewDataEntryFlags, SvLBoxEntry* _pEntry )
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);

	// determine the image set
	BmpColorMode eMode( BMP_COLOR_NORMAL );
	if ( !!m_pImpl->m_aImage1_hc )
	{	// we really have HC images
		const Wallpaper& rDeviceBackground = _rDev.GetDisplayBackground();
		if ( rDeviceBackground.GetColor().IsDark() )
			eMode = BMP_COLOR_HIGHCONTRAST;
	}

	// get the image
	const Image& rImage = implGetImageStore( 0 == ( _nViewDataEntryFlags & m_pImpl->m_nB2IndicatorFlags ), eMode );

	sal_Bool _bSemiTransparent = _pEntry && ( 0 != ( SV_ENTRYFLAG_SEMITRANSPARENT  & _pEntry->GetFlags( ) ) );
	// draw
	USHORT nStyle = _rDev.IsEnabled() ? 0 : IMAGE_DRAW_DISABLE;
	if ( _bSemiTransparent )
		nStyle |= IMAGE_DRAW_SEMITRANSPARENT;
	_rDev.DrawImage( _rPos, rImage, nStyle);
}

SvLBoxItem* SvLBoxContextBmp::Create() const
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);
	return new SvLBoxContextBmp;
}

void SvLBoxContextBmp::Clone( SvLBoxItem* pSource )
{
	DBG_CHKTHIS(SvLBoxContextBmp,0);
	m_pImpl->m_aImage1 = static_cast< SvLBoxContextBmp* >( pSource )->m_pImpl->m_aImage1;
	m_pImpl->m_aImage2 = static_cast< SvLBoxContextBmp* >( pSource )->m_pImpl->m_aImage2;
	m_pImpl->m_nB2IndicatorFlags = static_cast< SvLBoxContextBmp* >( pSource )->m_pImpl->m_nB2IndicatorFlags;
}

