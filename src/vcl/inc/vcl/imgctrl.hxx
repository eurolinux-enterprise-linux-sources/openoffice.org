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

#ifndef _SV_IMGCTRL_HXX
#define _SV_IMGCTRL_HXX

#include <vcl/dllapi.h>

#include <vcl/fixed.hxx>
#include <vcl/bitmapex.hxx>

// ----------------
// - ImageControl -
// ----------------

class VCL_DLLPUBLIC ImageControl : public FixedImage
{
private:
	BitmapEx		maBmp;
	BitmapEx		maBmpHC;
    ::sal_Int16     mnScaleMode;

public:
					ImageControl( Window* pParent, WinBits nStyle = 0 );

    // set/get the scale mode. This is one of the css.awt.ImageScaleMode constants
    void            SetScaleMode( const ::sal_Int16 _nMode );
    ::sal_Int16     GetScaleMode() const { return mnScaleMode; }

	virtual void	Resize();
	virtual void	UserDraw( const UserDrawEvent& rUDEvt );
    virtual void    Paint( const Rectangle& rRect );
    virtual void    GetFocus();
    virtual void    LoseFocus();

	void			SetBitmap( const BitmapEx& rBmp );
    using OutputDevice::GetBitmap;
	const BitmapEx& GetBitmap() const { return maBmp; }
    BOOL            SetModeBitmap( const BitmapEx& rBitmap, BmpColorMode eMode = BMP_COLOR_NORMAL );
    const BitmapEx& GetModeBitmap( BmpColorMode eMode = BMP_COLOR_NORMAL ) const;
};

#endif	// _SV_IMGCTRL_HXX
