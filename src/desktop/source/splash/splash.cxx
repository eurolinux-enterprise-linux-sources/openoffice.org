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
#include "precompiled_desktop.hxx"

#include <introbmpnames.hxx>
#include "splash.hxx"
#include <stdio.h>
#include <unotools/bootstrap.hxx>
#include <vos/process.hxx>
#include <tools/urlobj.hxx>
#include <tools/stream.hxx>
#include <sfx2/sfx.hrc>
#include <vcl/svapp.hxx>
#include <vcl/salnativewidgets.hxx>

#include <com/sun/star/registry/XRegistryKey.hpp>
#include <rtl/bootstrap.hxx>
#include <rtl/logfile.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/math.hxx>
#include <vcl/graph.hxx>
#include <svtools/filter.hxx>

#define NOT_LOADED  ((long)-1)

using namespace ::rtl;
using namespace ::com::sun::star::registry;

namespace desktop
{

SplashScreen::SplashScreen(const Reference< XMultiServiceFactory >& rSMgr)
    : IntroWindow()
    , _vdev(*((IntroWindow*)this))
    , _cProgressFrameColor(sal::static_int_cast< ColorData >(NOT_LOADED))
    , _cProgressBarColor(sal::static_int_cast< ColorData >(NOT_LOADED))
    , _bNativeProgress(true)
	, _iMax(100)
	, _iProgress(0)
    , _eBitmapMode(BM_DEFAULTMODE)
	, _bPaintBitmap(sal_True)
	, _bPaintProgress(sal_False)
    , _bShowLogo(sal_True)
    , _bFullScreenSplash(sal_False)
    , _bProgressEnd(sal_False)
    , _tlx(NOT_LOADED)
    , _tly(NOT_LOADED)
    , _barwidth(NOT_LOADED)
    , _barheight(NOT_LOADED)
    , _barspace(2)
    , _fXPos(-1.0)
    , _fYPos(-1.0)
    , _fWidth(-1.0)
    , _fHeight(-1.0)
    , _xoffset(12)
	, _yoffset(18)
{
	_rFactory = rSMgr;

    loadConfig();
}

SplashScreen::~SplashScreen()
{
	Application::RemoveEventListener(
		LINK( this, SplashScreen, AppEventListenerHdl ) );
	Hide();

}

void SAL_CALL SplashScreen::start(const OUString&, sal_Int32 nRange)
	throw (RuntimeException)
{
    _iMax = nRange;
    if (_bVisible) {
        _bProgressEnd = sal_False;
        ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
        if ( _eBitmapMode == BM_FULLSCREEN )
            ShowFullScreenMode( TRUE );
        Show();
        Paint(Rectangle());
        Flush();
    }
}

void SAL_CALL SplashScreen::end()
	throw (RuntimeException)
{
    _iProgress = _iMax;
    if (_bVisible )
    {
        if ( _eBitmapMode == BM_FULLSCREEN )
            EndFullScreenMode();
        Hide();
    }
    _bProgressEnd = sal_True;
}

void SAL_CALL SplashScreen::reset()
	throw (RuntimeException)
{
    _iProgress = 0;
    if (_bVisible && !_bProgressEnd )
    {
        if ( _eBitmapMode == BM_FULLSCREEN )
            ShowFullScreenMode( TRUE );
        Show();
        updateStatus();
    }
}

void SAL_CALL SplashScreen::setText(const OUString&)
	throw (RuntimeException)
{
    if (_bVisible && !_bProgressEnd) {
        if ( _eBitmapMode == BM_FULLSCREEN )
            ShowFullScreenMode( TRUE );
        Show();
        Flush();
    }
}

void SAL_CALL SplashScreen::setValue(sal_Int32 nValue)
	throw (RuntimeException)
{
    RTL_LOGFILE_CONTEXT( aLog, "::SplashScreen::setValue (lo119109)" );
    RTL_LOGFILE_CONTEXT_TRACE1( aLog, "value=%d", nValue );

    ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
    if (_bVisible && !_bProgressEnd) {
        if ( _eBitmapMode == BM_FULLSCREEN )
            ShowFullScreenMode( TRUE );
        Show();
        if (nValue >= _iMax) _iProgress = _iMax;
	else _iProgress = nValue;
	updateStatus();
    }
}

// XInitialize
void SAL_CALL
SplashScreen::initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any>& aArguments )
	throw (RuntimeException)
{
	::osl::ClearableMutexGuard	aGuard(	_aMutex );
	if (aArguments.getLength() > 0)
    {
        aArguments[0] >>= _bVisible;
        if (aArguments.getLength() > 1 )
            aArguments[1] >>= _sAppName;

        // start to determine bitmap and all other required value
        initBitmap();
	    Size aSize = _aIntroBmp.GetSizePixel();
	    SetOutputSizePixel( aSize );
        _vdev.SetOutputSizePixel( aSize );
	    _height = aSize.Height();
	    _width = aSize.Width();
        if (_width > 500)
        {
            Point xtopleft(212,216);
            if ( NOT_LOADED == _tlx || NOT_LOADED == _tly )
            {
                _tlx = xtopleft.X();    // top-left x
                _tly = xtopleft.Y();    // top-left y
            }
            if ( NOT_LOADED == _barwidth )
                _barwidth = 263;
            if ( NOT_LOADED == _barheight )
                _barheight = 8;
            if (( _eBitmapMode == BM_FULLSCREEN ) && 
                _bFullScreenSplash )
            {
                if( ( _fXPos >= 0.0 ) && ( _fYPos >= 0.0 ))
                {
                    _tlx = sal_Int32( double( aSize.Width() ) * _fXPos );
                    _tly = sal_Int32( double( aSize.Height() ) * _fYPos );
                }
                if ( _fWidth >= 0.0 )
                    _barwidth  = sal_Int32( double( aSize.Width() ) * _fWidth );
                if ( _fHeight >= 0.0 )
                    _barheight = sal_Int32( double( aSize.Width() ) * _fHeight );
            }   
        }
        else
        {
            if ( NOT_LOADED == _barwidth )
                _barwidth  = _width - (2 * _xoffset);
            if ( NOT_LOADED == _barheight )
                _barheight = 6;
            if ( NOT_LOADED == _tlx || NOT_LOADED == _tly )
            {
                _tlx = _xoffset;           // top-left x
                _tly = _height - _yoffset; // top-left y
            }
        }

        if ( sal::static_int_cast< ColorData >(NOT_LOADED) ==
             _cProgressFrameColor.GetColor() )
            _cProgressFrameColor = Color( COL_LIGHTGRAY );

        if ( sal::static_int_cast< ColorData >(NOT_LOADED) ==
             _cProgressBarColor.GetColor() )
        {
            // progress bar: new color only for big bitmap format
            if ( _width > 500 )
                _cProgressBarColor = Color( 157, 202, 18 );
            else
                _cProgressBarColor = Color( COL_BLUE );
        }

        Application::AddEventListener(
		    LINK( this, SplashScreen, AppEventListenerHdl ) );

        SetBackgroundBitmap( _aIntroBmp );
    }
}

void SplashScreen::updateStatus()
{
	if (!_bVisible || _bProgressEnd) return;
	if (!_bPaintProgress) _bPaintProgress = sal_True;
	//_bPaintBitmap=sal_False;
	Paint(Rectangle());
	//_bPaintBitmap=sal_True;
    Flush();
}

// internal private methods
IMPL_LINK( SplashScreen, AppEventListenerHdl, VclWindowEvent *, inEvent )
{
	if ( inEvent != 0 )
	{
        // Paint( Rectangle() );
		switch ( inEvent->GetId() )
		{
			case VCLEVENT_WINDOW_SHOW:
				Paint( Rectangle() );
				break;
			default:
				break;
		}
	}
	return 0;
}

// Read keys from edition/edition.ini or soffice{.ini|rc}:
OUString implReadBootstrapKey( const OUString& _rKey )
{
    OUString sValue(
        rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM(
                "${.override:${BRAND_BASE_DIR}/program/edition/edition.ini:")) +
        _rKey + rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("}")));
    rtl::Bootstrap::expandMacros(sValue);
    return sValue;
}

void SplashScreen::loadConfig()
{
    _bShowLogo = !implReadBootstrapKey(
        rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Logo"))).
        equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("0"));

    OUString sProgressFrameColor = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressFrameColor" ) ) );
    OUString sProgressBarColor = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressBarColor" ) ) );
    OUString sSize = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressSize" ) ) );
    OUString sPosition = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "ProgressPosition" ) ) );
    OUString sFullScreenSplash = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "FullScreenSplash" ) ) );
    OUString sNativeProgress = implReadBootstrapKey(
        OUString( RTL_CONSTASCII_USTRINGPARAM( "NativeProgress" ) ) );
                                                    

    // Determine full screen splash mode
    _bFullScreenSplash = (( sFullScreenSplash.getLength() > 0 ) && 
                          ( !sFullScreenSplash.equalsAsciiL( "0", 1 )));

    // Try to retrieve the relative values for the progress bar. The current
    // schema uses the screen ratio to retrieve the associated values.
    if ( _bFullScreenSplash )
        determineProgressRatioValues( _fXPos, _fYPos, _fWidth, _fHeight );

    if ( sProgressFrameColor.getLength() )
    {
        UINT8 nRed = 0;
        UINT8 nGreen = 0;
        UINT8 nBlue = 0;
        sal_Int32 idx = 0;
        sal_Int32 temp = sProgressFrameColor.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            nRed = static_cast< UINT8 >( temp );
            temp = sProgressFrameColor.getToken( 0, ',', idx ).toInt32();
        }
        if ( idx != -1 )
        {
            nGreen = static_cast< UINT8 >( temp );
            nBlue = static_cast< UINT8 >( sProgressFrameColor.getToken( 0, ',', idx ).toInt32() );
            _cProgressFrameColor = Color( nRed, nGreen, nBlue );
        }
    }

    if ( sProgressBarColor.getLength() )
    {
        UINT8 nRed = 0;
        UINT8 nGreen = 0;
        UINT8 nBlue = 0;
        sal_Int32 idx = 0;
        sal_Int32 temp = sProgressBarColor.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            nRed = static_cast< UINT8 >( temp );
            temp = sProgressBarColor.getToken( 0, ',', idx ).toInt32();
        }
        if ( idx != -1 )
        {
            nGreen = static_cast< UINT8 >( temp );
            nBlue = static_cast< UINT8 >( sProgressBarColor.getToken( 0, ',', idx ).toInt32() );
            _cProgressBarColor = Color( nRed, nGreen, nBlue );
        }
    }
    
    if( sNativeProgress.getLength() )
    {
        _bNativeProgress = sNativeProgress.toBoolean();
    }

    if ( sSize.getLength() )
    {
        sal_Int32 idx = 0;
        sal_Int32 temp = sSize.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            _barwidth = temp;
            _barheight = sSize.getToken( 0, ',', idx ).toInt32();
        }
    }

    if ( _barheight >= 10 )
        _barspace = 3;  // more space between frame and bar

    if ( sPosition.getLength() )
    {
        sal_Int32 idx = 0;
        sal_Int32 temp = sPosition.getToken( 0, ',', idx ).toInt32();
        if ( idx != -1 )
        {
            _tlx = temp;
            _tly = sPosition.getToken( 0, ',', idx ).toInt32();
        }
    }
}

void SplashScreen::initBitmap()
{
    if ( _bShowLogo )
    {
        OUString sExecutePath;
        ::rtl::Bootstrap::get(
            OUString( RTL_CONSTASCII_USTRINGPARAM( "BRAND_BASE_DIR" ) ),
            sExecutePath );
        sExecutePath += OUString( RTL_CONSTASCII_USTRINGPARAM( "/program/" ) );

        bool haveBitmap = false;

        // Try all bitmaps in INTRO_BITMAP_NAMES
        sal_Int32 nIndex = 0;
        OUString  aIntroBitmapFiles( RTL_CONSTASCII_USTRINGPARAM( INTRO_BITMAP_STRINGLIST ));
        do
        {
            haveBitmap = loadBitmap( sExecutePath, aIntroBitmapFiles.getToken( 0, ',', nIndex ) );
        } 
        while ( !haveBitmap && ( nIndex >= 0 ) );

        if (!haveBitmap) {
            rtl::OUString edition(
                rtl::OUString(
                    RTL_CONSTASCII_USTRINGPARAM(
                        "${BRAND_BASE_DIR}/program/edition")));
            rtl::Bootstrap::expandMacros(edition);
            haveBitmap = findBitmap(edition);
        }
        if (!haveBitmap) {
            findBitmap(sExecutePath);
        }
    }
}

bool SplashScreen::loadBitmap(
    rtl::OUString const & path, const rtl::OUString &rBmpFileName )
{
    if ( rBmpFileName.getLength() == 0 )
        return false;

    INetURLObject aObj( path, INET_PROT_FILE );
    aObj.insertName( rBmpFileName );

    SvFileStream aStrm( aObj.PathToFileName(), STREAM_STD_READ );
    if ( !aStrm.GetError() )
    {
        // Use graphic class to also support more graphic formats (bmp,png,...)
        Graphic aGraphic;

        GraphicFilter* pGF = GraphicFilter::GetGraphicFilter();
        pGF->ImportGraphic( aGraphic, String(), aStrm, GRFILTER_FORMAT_DONTKNOW );

        // Default case, we load the intro bitmap from a seperate file
        // (e.g. staroffice_intro.bmp or starsuite_intro.bmp)
        _aIntroBmp = aGraphic.GetBitmapEx();
        return true;
    }

    return false;
}

bool SplashScreen::findBitmap(rtl::OUString const & path) {
    bool haveBitmap = false;
    if ( _bFullScreenSplash )
    {
        haveBitmap = findScreenBitmap(path);
        if ( haveBitmap )
            _eBitmapMode = BM_FULLSCREEN;
        else
            haveBitmap = findAppBitmap(path);
    }
    if ( !haveBitmap )
    {
        haveBitmap = loadBitmap(
            path, rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("intro.png")));
        if ( !haveBitmap )
            haveBitmap = loadBitmap(
                path, rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("intro.bmp")));
    }

    return haveBitmap;
}

bool SplashScreen::findScreenBitmap(rtl::OUString const & path)
{
    sal_Int32 nWidth( 0 );
    sal_Int32 nHeight( 0 );

    // determine desktop resolution
    sal_uInt32 nCount = Application::GetScreenCount();
    if ( nCount > 0 )
    {
        // retrieve size from first screen
        Rectangle aScreenArea = Application::GetScreenPosSizePixel((unsigned int)0);
        nWidth  = aScreenArea.GetWidth();
        nHeight = aScreenArea.GetHeight();
    }
    
    // create file name from screen resolution information
    OUStringBuffer aStrBuf( 128 );
    aStrBuf.appendAscii( "intro_" );
    if ( _sAppName.getLength() > 0 )
    {
        aStrBuf.append( _sAppName );
        aStrBuf.appendAscii( "_" );
    }
    aStrBuf.append( OUString::valueOf( nWidth ));
    aStrBuf.appendAscii( "x" );
    aStrBuf.append( OUString::valueOf( nHeight ));
    
    OUString aRootIntroFileName = aStrBuf.makeStringAndClear();
    OUString aBmpFileName       = aRootIntroFileName + OUString::createFromAscii(".png");

    bool haveBitmap = loadBitmap( path, aBmpFileName );
    if ( !haveBitmap )
    {
        aBmpFileName = aRootIntroFileName + OUString::createFromAscii(".bmp");
        haveBitmap   = loadBitmap( path, aBmpFileName );
    }
    
    if ( !haveBitmap )
    {
        aStrBuf.appendAscii( "intro_" );
        aStrBuf.appendAscii( "_" );
        aStrBuf.append( OUString::valueOf( nWidth ));
        aStrBuf.appendAscii( "x" );
        aStrBuf.append( OUString::valueOf( nHeight ));
        
        aRootIntroFileName = aStrBuf.makeStringAndClear();
        aBmpFileName = aRootIntroFileName + OUString::createFromAscii(".png");

        haveBitmap = loadBitmap( path, aBmpFileName );
        if ( !haveBitmap )
        {
            aBmpFileName = aRootIntroFileName + OUString::createFromAscii(".bmp");
            haveBitmap   = loadBitmap( path, aBmpFileName );
        }
    }
    return haveBitmap;
}

bool SplashScreen::findAppBitmap(rtl::OUString const & path)
{
    bool haveBitmap = false;

    if ( _sAppName.getLength() > 0 )
    {
        OUStringBuffer aStrBuf( 128 );
        aStrBuf.appendAscii( "intro_" );
        aStrBuf.appendAscii( "_" );
        aStrBuf.append( _sAppName );
        
        OUString aRootIntroFileName = aStrBuf.makeStringAndClear();
        
        OUString aBmpFileName = aRootIntroFileName + OUString::createFromAscii( ".png" );
        haveBitmap = loadBitmap( path, aBmpFileName );
        if ( !haveBitmap )
        {
            aBmpFileName = aRootIntroFileName + OUString::createFromAscii( ".bmp" );
            haveBitmap = loadBitmap( path, aBmpFileName );
        }
    }
    return haveBitmap;
}

void SplashScreen::determineProgressRatioValues( 
    double& rXRelPos, double& rYRelPos, 
    double& rRelWidth, double& rRelHeight )
{
    sal_Int32 nWidth( 0 );
    sal_Int32 nHeight( 0 );
    sal_Int32 nScreenRatio( 0 );

    // determine desktop resolution
    sal_uInt32 nCount = Application::GetScreenCount();
    if ( nCount > 0 )
    {
        // retrieve size from first screen
        Rectangle aScreenArea = Application::GetScreenPosSizePixel((unsigned int)0);
        nWidth  = aScreenArea.GetWidth();
        nHeight = aScreenArea.GetHeight();
        nScreenRatio  = sal_Int32( math::round( double( nWidth ) / double( nHeight ), 2 ) * 100 );
    }

    char szFullScreenProgressRatio[] = "FullScreenProgressRatio0";
    char szFullScreenProgressPos[]   = "FullScreenProgressPos0";
    char szFullScreenProgressSize[]  = "FullScreenProgressSize0";
    for ( sal_Int32 i = 0; i <= 9; i++ )
    {
        char cNum = '0' + char( i );
        szFullScreenProgressRatio[23] = cNum;
        szFullScreenProgressPos[21]   = cNum;
        szFullScreenProgressSize[22]  = cNum;

        OUString sFullScreenProgressRatio = implReadBootstrapKey(
            OUString::createFromAscii( szFullScreenProgressRatio ) );

        if ( sFullScreenProgressRatio.getLength() > 0 )
        {
            double fRatio = sFullScreenProgressRatio.toDouble();
            sal_Int32 nRatio = sal_Int32( math::round( fRatio, 2 ) * 100 );
            if ( nRatio == nScreenRatio )
            {
                OUString sFullScreenProgressPos = implReadBootstrapKey(
                    OUString::createFromAscii( szFullScreenProgressPos ) );
                OUString sFullScreenProgressSize = implReadBootstrapKey(
                    OUString::createFromAscii( szFullScreenProgressSize ) );
                
                if ( sFullScreenProgressPos.getLength() )
                {
                    sal_Int32 idx = 0;
                    double temp = sFullScreenProgressPos.getToken( 0, ',', idx ).toDouble();
                    if ( idx != -1 )
                    {
                        rXRelPos = temp;
                        rYRelPos = sFullScreenProgressPos.getToken( 0, ',', idx ).toDouble();
                    }
                }

                if ( sFullScreenProgressSize.getLength() )
                {
                    sal_Int32 idx = 0;
                    double temp = sFullScreenProgressSize.getToken( 0, ',', idx ).toDouble();
                    if ( idx != -1 )
                    {
                        rRelWidth  = temp;
                        rRelHeight = sFullScreenProgressSize.getToken( 0, ',', idx ).toDouble();
                    }
                }
            }
        }
        else
            break;
    }
}

void SplashScreen::Paint( const Rectangle&)
{
	if(!_bVisible) return;

    //native drawing
    BOOL bNativeOK = FALSE;
    
    // in case of native controls we need to draw directly to the window
    if( _bNativeProgress && IsNativeControlSupported( CTRL_INTROPROGRESS, PART_ENTIRE_CONTROL ) )
    {
        DrawBitmapEx( Point(), _aIntroBmp );
        
        ImplControlValue aValue( _iProgress * _barwidth / _iMax);
        Rectangle aDrawRect( Point(_tlx, _tly), Size( _barwidth, _barheight ) );
        Region aControlRegion( aDrawRect );
        Region aNativeControlRegion, aNativeContentRegion;

        if( GetNativeControlRegion( CTRL_INTROPROGRESS, PART_ENTIRE_CONTROL, aControlRegion,
                                             CTRL_STATE_ENABLED, aValue, rtl::OUString(),
                                             aNativeControlRegion, aNativeContentRegion ) ) 
        {
              long nProgressHeight = aNativeControlRegion.GetBoundRect().GetHeight();
              aDrawRect.Top() -= (nProgressHeight - _barheight)/2;
              aDrawRect.Bottom() += (nProgressHeight - _barheight)/2;
              aControlRegion = Region( aDrawRect );
        }
        
        if( (bNativeOK = DrawNativeControl( CTRL_INTROPROGRESS, PART_ENTIRE_CONTROL, aControlRegion,
                                                     CTRL_STATE_ENABLED, aValue, rtl::OUString() )) != FALSE ) 
        {
            return;
        }
    }
    //non native drawing
	// draw bitmap
	if (_bPaintBitmap)
		_vdev.DrawBitmapEx( Point(), _aIntroBmp );

	if (_bPaintProgress) {
		// draw progress...
		long length = (_iProgress * _barwidth / _iMax) - (2 * _barspace);
		if (length < 0) length = 0;
		
		// border
		_vdev.SetFillColor();
		_vdev.SetLineColor( _cProgressFrameColor );
		_vdev.DrawRect(Rectangle(_tlx, _tly, _tlx+_barwidth, _tly+_barheight));
		_vdev.SetFillColor( _cProgressBarColor );
		_vdev.SetLineColor();
		Rectangle aRect(_tlx+_barspace, _tly+_barspace, _tlx+_barspace+length, _tly+_barheight-_barspace);
		_vdev.DrawRect(Rectangle(_tlx+_barspace, _tly+_barspace,
			_tlx+_barspace+length, _tly+_barheight-_barspace));

	}
    Size aSize =  GetOutputSizePixel();
    Size bSize =  _vdev.GetOutputSizePixel();
    //_vdev.Flush();
    //_vdev.DrawOutDev(Point(), GetOutputSize(), Point(), GetOutputSize(), *((IntroWindow*)this) );
    DrawOutDev(Point(), GetOutputSizePixel(), Point(), _vdev.GetOutputSizePixel(), _vdev );
	//Flush();
}


// get service instance...
SplashScreen *SplashScreen::_pINSTANCE = NULL;
osl::Mutex SplashScreen::_aMutex;

Reference< XInterface > SplashScreen::getInstance(const Reference< XMultiServiceFactory >& rSMgr)
{
	if ( _pINSTANCE == 0 )
	{
		osl::MutexGuard guard(_aMutex);
		if (_pINSTANCE == 0)
			return (XComponent*)new SplashScreen(rSMgr);
	}

	return (XComponent*)0;
}

// static service info...
const char* SplashScreen::interfaces[] =
{
    "com.sun.star.task.XStartusIndicator",
    "com.sun.star.lang.XInitialization",
    NULL,
};
const sal_Char *SplashScreen::serviceName = "com.sun.star.office.SplashScreen";
const sal_Char *SplashScreen::implementationName = "com.sun.star.office.comp.SplashScreen";
const sal_Char *SplashScreen::supportedServiceNames[] = {"com.sun.star.office.SplashScreen", NULL};

}
