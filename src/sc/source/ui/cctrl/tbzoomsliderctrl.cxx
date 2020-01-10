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
#include "precompiled_sc.hxx"
#ifndef _SC_ZOOMSLIDERTBCONTRL_HXX
#include "tbzoomsliderctrl.hxx"
#endif
#ifndef _SV_IMAGE_HXX
#include <vcl/image.hxx>
#endif
#ifndef _SV_TOOLBOX_HXX
#include <vcl/toolbox.hxx>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _SV_GRADIENT_HXX
#include <vcl/gradient.hxx>
#endif
#include <svtools/itemset.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/objsh.hxx>
#include <svx/zoomslideritem.hxx>
#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>
#include <set>
#include "docsh.hxx"
#include "stlpool.hxx"
#include "scitems.hxx"
#include "printfun.hxx"

//========================================================================
// class ScZoomSliderControl ---------------------------------------
//========================================================================

// -----------------------------------------------------------------------

SFX_IMPL_TOOLBOX_CONTROL( ScZoomSliderControl, SvxZoomSliderItem );

// -----------------------------------------------------------------------

ScZoomSliderControl::ScZoomSliderControl(
    USHORT     nSlotId,
    USHORT	   nId,
    ToolBox&   rTbx ) 
    :SfxToolBoxControl( nSlotId, nId, rTbx )
{
    rTbx.Invalidate();
}

// -----------------------------------------------------------------------

__EXPORT ScZoomSliderControl::~ScZoomSliderControl()
{

}

// -----------------------------------------------------------------------

void ScZoomSliderControl::StateChanged( USHORT /*nSID*/, SfxItemState eState,
                                       const SfxPoolItem* pState )
{
    USHORT                  nId	 = GetId();
    ToolBox&                rTbx = GetToolBox();
    ScZoomSliderWnd*        pBox = (ScZoomSliderWnd*)(rTbx.GetItemWindow( nId ));
    DBG_ASSERT( pBox ,"Control not found!" );

    if ( SFX_ITEM_AVAILABLE != eState || pState->ISA( SfxVoidItem ) )
    {
        SvxZoomSliderItem aZoomSliderItem( 100 );
        pBox->Disable();
        pBox->UpdateFromItem( &aZoomSliderItem );
    }
    else
    {
        pBox->Enable();
        DBG_ASSERT( pState->ISA( SvxZoomSliderItem ), "invalid item type" );
        const SvxZoomSliderItem* pZoomSliderItem = dynamic_cast< const SvxZoomSliderItem* >( pState );

        DBG_ASSERT( pZoomSliderItem, "Sc::ScZoomSliderControl::StateChanged(), wrong item type!" );
        if( pZoomSliderItem )
            pBox->UpdateFromItem( pZoomSliderItem );
    }
}

// -----------------------------------------------------------------------

Window* ScZoomSliderControl::CreateItemWindow( Window *pParent )
{
    // #i98000# Don't try to get a value via SfxViewFrame::Current here.
    // The view's value is always notified via StateChanged later.
    ScZoomSliderWnd* pSlider    = new ScZoomSliderWnd( pParent,
        ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchProvider >( m_xFrame->getController(),
        ::com::sun::star::uno::UNO_QUERY ), m_xFrame, 100 );
    return  pSlider;
}

// -----------------------------------------------------------------------

struct ScZoomSliderWnd::ScZoomSliderWnd_Impl
{
    USHORT                   mnCurrentZoom;
    USHORT                   mnMinZoom;
    USHORT                   mnMaxZoom;
    USHORT                   mnSliderCenter;
    std::vector< long >      maSnappingPointOffsets;
    std::vector< USHORT >    maSnappingPointZooms;
    Image                    maSliderButton;
    Image                    maIncreaseButton;
    Image                    maDecreaseButton;
    bool                     mbValuesSet;
    bool                     mbOmitPaint;

    ScZoomSliderWnd_Impl( USHORT nCurrentZoom ) :
        mnCurrentZoom( nCurrentZoom ),
        mnMinZoom( 10 ),
        mnMaxZoom( 400 ),
        mnSliderCenter( 100 ),
        maSnappingPointOffsets(),
        maSnappingPointZooms(),
        maSliderButton(),
        maIncreaseButton(),
        maDecreaseButton(),
        mbValuesSet( true ),
        mbOmitPaint( false ) 
        {

        }
};

// -----------------------------------------------------------------------

const long nButtonWidth     = 10;
const long nButtonHeight    = 10;
const long nIncDecWidth     = 11;
const long nIncDecHeight    = 11;
const long nSliderHeight    = 2;      //  
const long nSliderWidth     = 4;      //
const long nSnappingHeight  = 4;
const long nSliderXOffset   = 20;
const long nSnappingEpsilon = 5; // snapping epsilon in pixels
const long nSnappingPointsMinDist = nSnappingEpsilon; // minimum distance of two adjacent snapping points


// -----------------------------------------------------------------------

USHORT ScZoomSliderWnd::Offset2Zoom( long nOffset ) const
{
    Size aSliderWindowSize = GetOutputSizePixel();
    const long nControlWidth = aSliderWindowSize.Width();
    USHORT nRet = 0;
    
    if( nOffset < nSliderXOffset )
        return mpImpl->mnMinZoom;
    if( nOffset > nControlWidth - nSliderXOffset )
        return mpImpl->mnMaxZoom;

    // check for snapping points:
    USHORT nCount = 0;
    std::vector< long >::iterator aSnappingPointIter;
    for ( aSnappingPointIter = mpImpl->maSnappingPointOffsets.begin();
        aSnappingPointIter != mpImpl->maSnappingPointOffsets.end();
        ++aSnappingPointIter )
    {
        const long nCurrent = *aSnappingPointIter;
        if ( Abs(nCurrent - nOffset) < nSnappingEpsilon )
        {
            nOffset = nCurrent;
            nRet = mpImpl->maSnappingPointZooms[ nCount ];
            break;
        }
        ++nCount;
    }

    if( 0 == nRet )
    {
        if( nOffset < nControlWidth / 2 )
        {
            // first half of slider
            const long nFirstHalfRange      = mpImpl->mnSliderCenter - mpImpl->mnMinZoom;
            const long nHalfSliderWidth     = nControlWidth/2 - nSliderXOffset;
            const long nZoomPerSliderPixel  = (1000 * nFirstHalfRange) / nHalfSliderWidth;
            const long nOffsetToSliderLeft  = nOffset - nSliderXOffset;
            nRet = mpImpl->mnMinZoom + USHORT( nOffsetToSliderLeft * nZoomPerSliderPixel / 1000 );
        }
        else
        {
            // second half of slider
            const long nSecondHalfRange         = mpImpl->mnMaxZoom - mpImpl->mnSliderCenter;
            const long nHalfSliderWidth         = nControlWidth/2 - nSliderXOffset;
            const long nZoomPerSliderPixel      = 1000 * nSecondHalfRange / nHalfSliderWidth;
            const long nOffsetToSliderCenter    = nOffset - nControlWidth/2;
            nRet = mpImpl->mnSliderCenter + USHORT( nOffsetToSliderCenter * nZoomPerSliderPixel / 1000 );
        }
    }

    if( nRet < mpImpl->mnMinZoom )
        return mpImpl->mnMinZoom;
    
    else if( nRet > mpImpl->mnMaxZoom )
        return mpImpl->mnMaxZoom;

    return nRet;
}

// -----------------------------------------------------------------------

long ScZoomSliderWnd::Zoom2Offset( USHORT nCurrentZoom ) const
{
    Size aSliderWindowSize = GetOutputSizePixel();
    const long nControlWidth = aSliderWindowSize.Width();
    long  nRect = nSliderXOffset;

    const long nHalfSliderWidth = nControlWidth/2 - nSliderXOffset;
    if( nCurrentZoom <= mpImpl->mnSliderCenter )
    {
        nCurrentZoom = nCurrentZoom - mpImpl->mnMinZoom;
        const long nFirstHalfRange = mpImpl->mnSliderCenter - mpImpl->mnMinZoom;
        const long nSliderPixelPerZoomPercent = 1000 * nHalfSliderWidth  / nFirstHalfRange;
        const long nOffset = (nSliderPixelPerZoomPercent * nCurrentZoom) / 1000;
        nRect += nOffset;
    }
    else
    {
        nCurrentZoom = nCurrentZoom - mpImpl->mnSliderCenter;
        const long nSecondHalfRange = mpImpl->mnMaxZoom - mpImpl->mnSliderCenter;
        const long nSliderPixelPerZoomPercent = 1000 * nHalfSliderWidth  / nSecondHalfRange;
        const long nOffset = (nSliderPixelPerZoomPercent * nCurrentZoom) / 1000;
        nRect += nHalfSliderWidth + nOffset;
    }
    return nRect;
}

// -----------------------------------------------------------------------


ScZoomSliderWnd::ScZoomSliderWnd( Window* pParent, const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatchProvider >& rDispatchProvider,
                const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& _xFrame , USHORT nCurrentZoom ):
                Window( pParent ),
                mpImpl( new ScZoomSliderWnd_Impl( nCurrentZoom ) ),
                aLogicalSize( 115, 40 ),
                m_xDispatchProvider( rDispatchProvider ),
                m_xFrame( _xFrame )
{
    BOOL bIsDark                = GetSettings().GetStyleSettings().GetFaceColor().IsDark();
    mpImpl->maSliderButton      = Image( SVX_RES( bIsDark ? RID_SVXBMP_SLIDERBUTTON_HC : RID_SVXBMP_SLIDERBUTTON ) );
    mpImpl->maIncreaseButton    = Image( SVX_RES( bIsDark ? RID_SVXBMP_SLIDERINCREASE_HC : RID_SVXBMP_SLIDERINCREASE ) );
    mpImpl->maDecreaseButton    = Image( SVX_RES( bIsDark ? RID_SVXBMP_SLIDERDECREASE_HC : RID_SVXBMP_SLIDERDECREASE ) );
    Size  aSliderSize           = LogicToPixel( Size( aLogicalSize), MapMode( MAP_10TH_MM ) );
    SetSizePixel( Size( aSliderSize.Width() * nSliderWidth-1, aSliderSize.Height() + nSliderHeight ) );
}

// -----------------------------------------------------------------------

ScZoomSliderWnd::~ScZoomSliderWnd()
{
    delete mpImpl;
}

// -----------------------------------------------------------------------

void ScZoomSliderWnd::MouseButtonDown( const MouseEvent& rMEvt )
{   
    if ( !mpImpl->mbValuesSet )
        return ;
    Size aSliderWindowSize = GetOutputSizePixel();

    const Point aPoint = rMEvt.GetPosPixel();

    const long nButtonLeftOffset    = ( nSliderXOffset - nIncDecWidth )/2;
    const long nButtonRightOffset   = ( nSliderXOffset + nIncDecWidth )/2;

    const long nOldZoom = mpImpl->mnCurrentZoom;

    // click to - button
    if ( aPoint.X() >= nButtonLeftOffset && aPoint.X() <= nButtonRightOffset )
    {
        mpImpl->mnCurrentZoom = mpImpl->mnCurrentZoom - 5;
    }
    // click to + button
    else if ( aPoint.X() >= aSliderWindowSize.Width() - nSliderXOffset + nButtonLeftOffset &&
              aPoint.X() <= aSliderWindowSize.Width() - nSliderXOffset + nButtonRightOffset )
    {
        mpImpl->mnCurrentZoom = mpImpl->mnCurrentZoom + 5;
    }
    else if( aPoint.X() >= nSliderXOffset && aPoint.X() <= aSliderWindowSize.Width() - nSliderXOffset )
    {
        mpImpl->mnCurrentZoom = Offset2Zoom( aPoint.X() );
    }

    if( mpImpl->mnCurrentZoom < mpImpl->mnMinZoom )
        mpImpl->mnCurrentZoom = mpImpl->mnMinZoom;
    else if( mpImpl->mnCurrentZoom > mpImpl->mnMaxZoom )
        mpImpl->mnCurrentZoom = mpImpl->mnMaxZoom;

    if( nOldZoom == mpImpl->mnCurrentZoom )
        return ;

    Rectangle aRect( Point( 0, 0 ), aSliderWindowSize );
    
    Paint( aRect );
    mpImpl->mbOmitPaint = true;
    
    SvxZoomSliderItem   aZoomSliderItem( mpImpl->mnCurrentZoom );

    ::com::sun::star::uno::Any  a;
    aZoomSliderItem.QueryValue( a );
    
    ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > aArgs( 1 );
    aArgs[0].Name = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ScalingFactor" ));
    aArgs[0].Value = a;
    
    SfxToolBoxControl::Dispatch( m_xDispatchProvider, String::CreateFromAscii(".uno:ScalingFactor"), aArgs );

    mpImpl->mbOmitPaint = false;
}

// -----------------------------------------------------------------------

void ScZoomSliderWnd::MouseMove( const MouseEvent& rMEvt )
{
    if ( !mpImpl->mbValuesSet )
        return ;

    Size aSliderWindowSize   = GetOutputSizePixel();
    const long nControlWidth = aSliderWindowSize.Width();
    const short nButtons     = rMEvt.GetButtons();

    // check mouse move with button pressed
    if ( 1 == nButtons )
    {
        const Point aPoint = rMEvt.GetPosPixel();

        if ( aPoint.X() >= nSliderXOffset && aPoint.X() <= nControlWidth - nSliderXOffset )
        {
            mpImpl->mnCurrentZoom = Offset2Zoom( aPoint.X() );

            Rectangle aRect( Point( 0, 0 ), aSliderWindowSize  );
            Paint( aRect );

            mpImpl->mbOmitPaint = true; // optimization: paint before executing command,

            // commit state change
            SvxZoomSliderItem aZoomSliderItem( mpImpl->mnCurrentZoom );

            ::com::sun::star::uno::Any a;
            aZoomSliderItem.QueryValue( a );

            ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > aArgs( 1 );
            aArgs[0].Name = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ScalingFactor" ));
            aArgs[0].Value = a;

            SfxToolBoxControl::Dispatch( m_xDispatchProvider, String::CreateFromAscii(".uno:ScalingFactor"), aArgs );

            mpImpl->mbOmitPaint = false;
        }
    }
}

// -----------------------------------------------------------------------

void ScZoomSliderWnd::UpdateFromItem( const SvxZoomSliderItem* pZoomSliderItem )
{
    if( pZoomSliderItem )
    {
        mpImpl->mnCurrentZoom = pZoomSliderItem->GetValue();
        mpImpl->mnMinZoom     = pZoomSliderItem->GetMinZoom();
        mpImpl->mnMaxZoom     = pZoomSliderItem->GetMaxZoom();

        DBG_ASSERT( mpImpl->mnMinZoom <= mpImpl->mnCurrentZoom &&
            mpImpl->mnMinZoom <  mpImpl->mnSliderCenter &&
            mpImpl->mnMaxZoom >= mpImpl->mnCurrentZoom &&
            mpImpl->mnMaxZoom > mpImpl->mnSliderCenter,
            "Looks like the zoom slider item is corrupted" );
       const com::sun::star::uno::Sequence < sal_Int32 > rSnappingPoints = pZoomSliderItem->GetSnappingPoints();
       mpImpl->maSnappingPointOffsets.clear();
       mpImpl->maSnappingPointZooms.clear();

       // get all snapping points:
       std::set< USHORT > aTmpSnappingPoints;
       for ( USHORT j = 0; j < rSnappingPoints.getLength(); ++j )
       {
           const sal_Int32 nSnappingPoint = rSnappingPoints[j];
           aTmpSnappingPoints.insert( (USHORT)nSnappingPoint );
       }

       // remove snapping points that are to close to each other:
       std::set< USHORT >::iterator aSnappingPointIter;
       long nLastOffset = 0;

       for ( aSnappingPointIter = aTmpSnappingPoints.begin(); aSnappingPointIter != aTmpSnappingPoints.end(); ++aSnappingPointIter )
       {
           const USHORT nCurrent = *aSnappingPointIter;
           const long nCurrentOffset = Zoom2Offset( nCurrent );

           if ( nCurrentOffset - nLastOffset >= nSnappingPointsMinDist )
           {
               mpImpl->maSnappingPointOffsets.push_back( nCurrentOffset );
               mpImpl->maSnappingPointZooms.push_back( nCurrent );
               nLastOffset = nCurrentOffset;
           }
       }
    }

    Size aSliderWindowSize = GetOutputSizePixel();
    Rectangle aRect( Point( 0, 0 ), aSliderWindowSize );
   
    if ( !mpImpl->mbOmitPaint )
       Paint(aRect);
}

// -----------------------------------------------------------------------

void ScZoomSliderWnd::Paint( const Rectangle& rRect )
{
    DoPaint( rRect );
}

// -----------------------------------------------------------------------

void ScZoomSliderWnd::DoPaint( const Rectangle& /*rRect*/ )
{
    if( mpImpl->mbOmitPaint )
        return;

    Size aSliderWindowSize = GetOutputSizePixel();
    Rectangle aRect( Point( 0, 0 ), aSliderWindowSize );
    
    VirtualDevice* pVDev = new VirtualDevice( *this );
    pVDev->SetOutputSizePixel( aSliderWindowSize );

    Rectangle   aSlider = aRect;

    aSlider.Top()     += ( aSliderWindowSize.Height() - nSliderHeight )/2 - 1;
    aSlider.Bottom()   = aSlider.Top() + nSliderHeight;
    aSlider.Left()    += nSliderXOffset;
    aSlider.Right()   -= nSliderXOffset;

    Rectangle aFirstLine( aSlider );
    aFirstLine.Bottom() = aFirstLine.Top();

    Rectangle aSecondLine( aSlider );
    aSecondLine.Top() = aSecondLine.Bottom();

    Rectangle aLeft( aSlider );
    aLeft.Right() = aLeft.Left();

    Rectangle aRight( aSlider );
    aRight.Left() = aRight.Right();

    // draw VirtualDevice's background color
    Color  aStartColor,aEndColor;
    aStartColor = GetSettings().GetStyleSettings().GetFaceColor();
    aEndColor   = GetSettings().GetStyleSettings().GetFaceColor();
    if( aEndColor.IsDark() )
        aStartColor = aEndColor;

    Gradient g;
    g.SetAngle( 0 );
    g.SetStyle( GRADIENT_LINEAR );

    g.SetStartColor( aStartColor );
    g.SetEndColor( aEndColor );
    pVDev->DrawGradient( aRect, g );

    // draw slider
    pVDev->SetLineColor( Color ( COL_WHITE ) );
    pVDev->DrawRect( aSecondLine );
    pVDev->DrawRect( aRight );

    pVDev->SetLineColor( Color( COL_GRAY ) );
    pVDev->DrawRect( aFirstLine );
    pVDev->DrawRect( aLeft );

    // draw snapping points:
    std::vector< long >::iterator aSnappingPointIter;
    for ( aSnappingPointIter = mpImpl->maSnappingPointOffsets.begin();
        aSnappingPointIter != mpImpl->maSnappingPointOffsets.end();
        ++aSnappingPointIter )
    {
        pVDev->SetLineColor( Color( COL_GRAY ) );
        Rectangle aSnapping( aRect );
        aSnapping.Bottom()   = aSlider.Top();
        aSnapping.Top() = aSnapping.Bottom() - nSnappingHeight;
        aSnapping.Left() += *aSnappingPointIter;
        aSnapping.Right() = aSnapping.Left();
        pVDev->DrawRect( aSnapping );

        aSnapping.Top() += nSnappingHeight + nSliderHeight;
        aSnapping.Bottom() += nSnappingHeight + nSliderHeight;
        pVDev->DrawRect( aSnapping );
    }

    // draw slider button
    Point aImagePoint = aRect.TopLeft();
    aImagePoint.X() += Zoom2Offset( mpImpl->mnCurrentZoom );
    aImagePoint.X() -= nButtonWidth/2;
    aImagePoint.Y() += ( aSliderWindowSize.Height() - nButtonHeight)/2;
    pVDev->DrawImage( aImagePoint, mpImpl->maSliderButton );

    // draw decrease button
    aImagePoint = aRect.TopLeft();
    aImagePoint.X() += (nSliderXOffset - nIncDecWidth)/2;
    aImagePoint.Y() += ( aSliderWindowSize.Height() - nIncDecHeight)/2;
    pVDev->DrawImage( aImagePoint, mpImpl->maDecreaseButton );

    // draw increase button
    aImagePoint.X() = aRect.TopLeft().X() + aSliderWindowSize.Width() - nIncDecWidth - (nSliderXOffset - nIncDecWidth)/2;
    pVDev->DrawImage( aImagePoint, mpImpl->maIncreaseButton );
    
    DrawOutDev( Point(0, 0), aSliderWindowSize, Point(0, 0), aSliderWindowSize, *pVDev );

    delete pVDev;

}

// -----------------------------------------------------------------------
