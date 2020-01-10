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
#ifndef _SVX_FNTCTRL_HXX
#define _SVX_FNTCTRL_HXX

// include ---------------------------------------------------------------

#include <vcl/window.hxx>
#include <svx/svxfont.hxx>
#include "svx/svxdllapi.h"

#ifndef _RTL_USTRING_HXX_
#include <rtl/ustring.hxx>
#endif

// forward ---------------------------------------------------------------

class FontPrevWin_Impl;

// class SvxFontPrevWindow -----------------------------------------------

class SVX_DLLPUBLIC SvxFontPrevWindow : public Window
{
	using OutputDevice::SetFont;
private:
	FontPrevWin_Impl*   pImpl;

	SVX_DLLPRIVATE void				InitSettings( BOOL bForeground, BOOL bBackground );

public:
						SvxFontPrevWindow( Window* pParent, const ResId& rId );
	virtual				~SvxFontPrevWindow();

	virtual void		StateChanged( StateChangedType nStateChange );
	virtual void		DataChanged( const DataChangedEvent& rDCEvt );

	// Aus Effizienz-gr"unden nicht const
	SvxFont& 			GetFont();
	const SvxFont& 		GetFont() const;
	void  				SetFont( const SvxFont& rFont );
	void  				SetFont( const SvxFont& rNormalFont, const SvxFont& rCJKFont, const SvxFont& rCTLFont );
	void  				SetCJKFont( const SvxFont& rFont );
	void  				SetCTLFont( const SvxFont& rFont );
    SvxFont&            GetCJKFont();
	SvxFont&			GetCTLFont();
	void  				SetColor( const Color& rColor );
    void                ResetColor();
    void                SetBackColor( const Color& rColor );
    void                UseResourceText( BOOL bUse = TRUE );
	void  				Paint( const Rectangle& );

	BOOL				IsTwoLines() const;
	void				SetTwoLines(BOOL bSet);

	void				SetBrackets(sal_Unicode cStart, sal_Unicode cEnd);

	void				SetFontWidthScale( UINT16 nScaleInPercent );

	void				AutoCorrectFontColor( void );

    void                SetPreviewText( const ::rtl::OUString& rString );
    void                SetFontNameAsPreviewText();
};

#endif // #ifndef _SVX_FNTCTRL_HXX

