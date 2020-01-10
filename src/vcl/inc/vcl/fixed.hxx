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

#ifndef _SV_FIXED_HXX
#define _SV_FIXED_HXX

#include <vcl/sv.h>
#include <vcl/dllapi.h>
#include <vcl/bitmap.hxx>
#include <vcl/image.hxx>
#include <vcl/ctrl.hxx>

class UserDrawEvent;

// -------------
// - FixedText -
// -------------

class VCL_DLLPUBLIC FixedText : public Control
{
//#if 0 // _SOLAR__PRIVATE
private:
    using Window::ImplInit;
    SAL_DLLPRIVATE void    ImplInit( Window* pParent, WinBits nStyle );
    SAL_DLLPRIVATE WinBits ImplInitStyle( WinBits nStyle );
    SAL_DLLPRIVATE void    ImplInitSettings( BOOL bFont, BOOL bForeground, BOOL bBackground );
    SAL_DLLPRIVATE void    ImplDraw( OutputDevice* pDev, ULONG nDrawFlags,
                              const Point& rPos, const Size& rSize, bool bFillLayout = false ) const;
public:
    SAL_DLLPRIVATE static USHORT   ImplGetTextStyle( WinBits nWinBits );
//#endif
protected:
    virtual void    FillLayoutData() const;
public:
                    FixedText( Window* pParent, WinBits nStyle = 0 );
                    FixedText( Window* pParent, const ResId& rResId );
                    FixedText( Window* pParent, const ResId& rResId, bool bDisableAccessibleLabelForRelation );

    virtual void    Paint( const Rectangle& rRect );
    virtual void    Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );
    virtual void    Resize();
    virtual void    StateChanged( StateChangedType nType );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

    static Size     CalcMinimumTextSize( Control const* pControl, long nMaxWidth = 0 );
    Size            CalcMinimumSize( long nMaxWidth = 0 ) const;
    virtual Size    GetOptimalSize(WindowSizeType eType) const;
};

// -------------
// - FixedLine -
// -------------

class VCL_DLLPUBLIC FixedLine : public Control
{
private:
    using Window::ImplInit;
    SAL_DLLPRIVATE void    ImplInit( Window* pParent, WinBits nStyle );
    SAL_DLLPRIVATE WinBits ImplInitStyle( WinBits nStyle );
    SAL_DLLPRIVATE void    ImplInitSettings( BOOL bFont, BOOL bForeground, BOOL bBackground );
    SAL_DLLPRIVATE void    ImplDraw( bool bLayout = false );

protected:
    virtual void	FillLayoutData() const;

public:
                    FixedLine( Window* pParent, WinBits nStyle = WB_HORZ );
                    FixedLine( Window* pParent, const ResId& rResId );

    virtual void    Paint( const Rectangle& rRect );
    virtual void    Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );
    virtual void    Resize();
    virtual void    StateChanged( StateChangedType nType );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

    virtual Size    GetOptimalSize(WindowSizeType eType) const;
};

// ---------------
// - FixedBitmap -
// ---------------

class VCL_DLLPUBLIC FixedBitmap : public Control
{
private:
    Bitmap          maBitmap;
    Bitmap          maBitmapHC;

    using Window::ImplInit;
    SAL_DLLPRIVATE void    ImplInit( Window* pParent, WinBits nStyle );
    SAL_DLLPRIVATE WinBits ImplInitStyle( WinBits nStyle );
    SAL_DLLPRIVATE void    ImplInitSettings();
    SAL_DLLPRIVATE void    ImplDraw( OutputDevice* pDev, ULONG nDrawFlags,
                              const Point& rPos, const Size& rSize );

protected:
    SAL_DLLPRIVATE void    ImplLoadRes( const ResId& rResId );

public:
                    FixedBitmap( Window* pParent, WinBits nStyle = 0 );
                    FixedBitmap( Window* pParent, const ResId& rResId );
                    ~FixedBitmap();

    virtual void    Paint( const Rectangle& rRect );
    virtual void    Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );
    virtual void    Resize();
    virtual void    StateChanged( StateChangedType nType );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

    void            SetBitmap( const Bitmap& rBitmap );
    using OutputDevice::GetBitmap;
    const Bitmap&   GetBitmap() const { return maBitmap; }
    BOOL            SetModeBitmap( const Bitmap& rBitmap, BmpColorMode eMode = BMP_COLOR_NORMAL );
    const Bitmap&   GetModeBitmap( BmpColorMode eMode = BMP_COLOR_NORMAL ) const;
};

// --------------
// - FixedImage -
// --------------

class VCL_DLLPUBLIC FixedImage : public Control
{
private:
    Image           maImage;
    Image           maImageHC;
    BOOL            mbInUserDraw;

private:
    using Window::ImplInit;
    SAL_DLLPRIVATE void    ImplInit( Window* pParent, WinBits nStyle );
    SAL_DLLPRIVATE WinBits ImplInitStyle( WinBits nStyle );
    SAL_DLLPRIVATE void    ImplInitSettings();
    SAL_DLLPRIVATE void    ImplDraw( OutputDevice* pDev, ULONG nDrawFlags,
                              const Point& rPos, const Size& rSize );

protected:
    SAL_DLLPRIVATE void    ImplLoadRes( const ResId& rResId );

public:
                    FixedImage( Window* pParent, WinBits nStyle = 0 );
                    FixedImage( Window* pParent, const ResId& rResId );
                    ~FixedImage();

    virtual void    Paint( const Rectangle& rRect );
    virtual void    Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );
    virtual void    Resize();
    virtual void    StateChanged( StateChangedType nType );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );
    virtual void    UserDraw( const UserDrawEvent& rUDEvt );

    void            SetImage( const Image& rImage );
    const Image&    GetImage() const { return maImage; }

    BOOL            SetModeImage( const Image& rImage, BmpColorMode eMode = BMP_COLOR_NORMAL );
    const Image&    GetModeImage( BmpColorMode eMode = BMP_COLOR_NORMAL ) const;

    Point           CalcImagePos( const Point& rPos,
                                  const Size& rObjSize, const Size& rWinSize );
};

#endif  // _SV_FIXED_HXX
