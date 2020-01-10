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
#include "precompiled_svx.hxx"
#include <vcl/wrkwin.hxx>
#include <sot/factory.hxx>
#include <tools/shl.hxx>
#include <vcl/salbtype.hxx>		// FRound
#include <vcl/msgbox.hxx>
#include <svtools/eitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <svtools/miscopt.hxx>
#include <unotools/localedatawrapper.hxx>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif

#define _SDR_NOITEMS
#define _SDR_NOTOUCH
#define _SDR_NOTRANSFORM
#define _SDR_NOOBJECTS
#define _SDR_NOVIEWMARKER
#define _SDR_NODRAGMETHODS
#define _SDR_NOUNDO
#define _SDR_NOXOUTDEV
#define _XOUTBMP_STATICS_ONLY

#include <svx/dialmgr.hxx>
#include "xoutbmp.hxx"
#include <svx/dialogs.hrc>
#include <svx/svxids.hrc>
#include <svx/contdlg.hxx>
#include "contimp.hxx"
#include "contdlg.hrc"
#include "contwnd.hxx"
#include <svx/svdtrans.hxx>
#include <svx/svdopath.hxx>
#include "dlgutil.hxx"
#include <vcl/svapp.hxx>

/******************************************************************************/

inline String GetUnitString( long nVal_100, FieldUnit eFieldUnit, sal_Unicode cSep )
{
	String aVal = UniString::CreateFromInt64( MetricField::ConvertValue( nVal_100, 2, MAP_100TH_MM, eFieldUnit ) );

	while( aVal.Len() < 3 )
		aVal.Insert( sal_Unicode('0'), 0 );

	aVal.Insert( cSep, aVal.Len() - 2 );
	aVal += sal_Unicode(' ');
	aVal += SdrFormatter::GetUnitStr( eFieldUnit );

	return aVal;
}


/******************************************************************************/


SFX_IMPL_FLOATINGWINDOW( SvxContourDlgChildWindow, SID_CONTOUR_DLG );


/******************************************************************************/


/*************************************************************************
|*
|* ControllerItem
|*
\************************************************************************/

SvxContourDlgItem::SvxContourDlgItem( USHORT _nId, SvxSuperContourDlg& rContourDlg, SfxBindings& rBindings ) :
            SfxControllerItem   ( _nId, rBindings ),
			rDlg				( rContourDlg )
{
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxContourDlgItem::StateChanged( USHORT nSID, SfxItemState /*eState*/, const SfxPoolItem* pItem )
{
	if ( pItem && ( SID_CONTOUR_EXEC == nSID ) )
	{
		const SfxBoolItem* pStateItem = PTR_CAST( SfxBoolItem, pItem );

		DBG_ASSERT( pStateItem || pItem == 0, "SfxBoolItem erwartet");

		rDlg.SetExecState( !pStateItem->GetValue() );
	}
}


/******************************************************************************/


/*************************************************************************
|*
|* Contour-Float
|*
\************************************************************************/

SvxContourDlgChildWindow::SvxContourDlgChildWindow( Window* _pParent, USHORT nId,
													SfxBindings* pBindings, SfxChildWinInfo* pInfo ) :
            SfxChildWindow( _pParent, nId )
{
    SvxSuperContourDlg* pDlg = new SvxSuperContourDlg( pBindings, this, _pParent, CONT_RESID( RID_SVXDLG_CONTOUR ) );
    pWindow = pDlg;

	if ( pInfo->nFlags & SFX_CHILDWIN_ZOOMIN )
		pDlg->RollUp();

	eChildAlignment = SFX_ALIGN_NOALIGNMENT;

    pDlg->Initialize( pInfo );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxContourDlgChildWindow::UpdateContourDlg( const Graphic& rGraphic, BOOL bGraphicLinked,
												 const PolyPolygon* pPolyPoly, void* pEditingObj )
{
	if ( SfxViewFrame::Current() &&
		 SfxViewFrame::Current()->HasChildWindow( SvxContourDlgChildWindow::GetChildWindowId() ) )
		SVXCONTOURDLG()->Update( rGraphic, bGraphicLinked, pPolyPoly, pEditingObj );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

SvxContourDlg::SvxContourDlg( SfxBindings* _pBindings, SfxChildWindow* pCW,
                              Window* _pParent, const ResId& rResId ) :

    SfxFloatingWindow   ( _pBindings, pCW, _pParent, rResId )

{
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

SvxContourDlg::~SvxContourDlg()
{
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

PolyPolygon SvxContourDlg::CreateAutoContour( const Graphic& rGraphic,
											  const Rectangle* pRect,
											  const ULONG nFlags )
{
	Bitmap	aBmp;
	ULONG	nContourFlags = XOUTBMP_CONTOUR_HORZ;

	if ( rGraphic.GetType() == GRAPHIC_BITMAP )
	{
		if( rGraphic.IsAnimated() )
		{
			VirtualDevice		aVDev;
			MapMode				aTransMap;
			PolyPolygon			aAnimPolyPoly;
			const Animation		aAnim( rGraphic.GetAnimation() );
			const Size&			rSizePix = aAnim.GetDisplaySizePixel();
			const USHORT		nCount = aAnim.Count();

			if ( aVDev.SetOutputSizePixel( rSizePix ) )
			{
				aVDev.SetLineColor( Color( COL_BLACK ) );
				aVDev.SetFillColor( Color( COL_BLACK ) );

				for( USHORT i = 0; i < nCount; i++ )
				{
					const AnimationBitmap& rStepBmp = aAnim.Get( i );

					// Polygonausgabe an die richtige Stelle schieben;
					// dies ist der Offset des Teilbildes innerhalb
					// der Gesamtanimation
					aTransMap.SetOrigin( Point( rStepBmp.aPosPix.X(), rStepBmp.aPosPix.Y() ) );
					aVDev.SetMapMode( aTransMap );
					aVDev.DrawPolyPolygon( CreateAutoContour( rStepBmp.aBmpEx, pRect, nFlags ) );
				}

				aTransMap.SetOrigin( Point() );
				aVDev.SetMapMode( aTransMap );
				aBmp = aVDev.GetBitmap( Point(), rSizePix );
				aBmp.Convert( BMP_CONVERSION_1BIT_THRESHOLD );
			}
		}
		else if( rGraphic.IsTransparent() )
			aBmp = rGraphic.GetBitmapEx().GetMask();
		else
		{
			aBmp = rGraphic.GetBitmap();
			nContourFlags |= XOUTBMP_CONTOUR_EDGEDETECT;
		}
	}
	else if( rGraphic.GetType() != GRAPHIC_NONE )
	{
        const Graphic   aTmpGrf( rGraphic.GetGDIMetaFile().GetMonochromeMtf( Color( COL_BLACK ) ) );
        VirtualDevice	aVDev;
        Size			aSizePix( aVDev.LogicToPixel( aTmpGrf.GetPrefSize(), aTmpGrf.GetPrefMapMode() ) );

		if( aSizePix.Width() && aSizePix.Height() && ( aSizePix.Width() > 512 || aSizePix.Height() > 512 ) )
		{
			double fWH = (double) aSizePix.Width() / aSizePix.Height();

			if( fWH <= 1.0 )
				aSizePix.Width() = FRound( ( aSizePix.Height() = 512 ) * fWH );
			else
				aSizePix.Height() = FRound( ( aSizePix.Width() = 512 ) / fWH );
		}

		if( aVDev.SetOutputSizePixel( aSizePix ) )
        {
			const Point aPt;
            aTmpGrf.Draw( &aVDev, aPt, aSizePix );
            aBmp = aVDev.GetBitmap( aPt, aSizePix );
        }

		nContourFlags |= XOUTBMP_CONTOUR_EDGEDETECT;
	}

	aBmp.SetPrefSize( rGraphic.GetPrefSize() );
	aBmp.SetPrefMapMode( rGraphic.GetPrefMapMode() );

	return PolyPolygon( XOutBitmap::GetCountour( aBmp, nContourFlags, 128, pRect ) );
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxContourDlg::ScaleContour( PolyPolygon& rContour, const Graphic& rGraphic,
								  const MapUnit eUnit, const Size& rDisplaySize )
{
	DBG_ASSERT( rGraphic.GetType() != GRAPHIC_NONE, "Graphic is not valid!" );

	OutputDevice*	pOutDev = Application::GetDefaultDevice();
	const MapMode	aDispMap( eUnit );
	const MapMode	aGrfMap( rGraphic.GetPrefMapMode() );
	const Size		aGrfSize( rGraphic.GetPrefSize() );
	double			fScaleX;
	double			fScaleY;
	Size			aOrgSize;
	Point			aNewPoint;
	BOOL			bPixelMap = aGrfMap.GetMapUnit() == MAP_PIXEL;

	if ( bPixelMap )
		aOrgSize = pOutDev->PixelToLogic( aGrfSize, aDispMap );
	else
		aOrgSize = pOutDev->LogicToLogic( aGrfSize, aGrfMap, aDispMap );

	if ( aOrgSize.Width() && aOrgSize.Height() )
	{
		fScaleX = (double) rDisplaySize.Width() / aOrgSize.Width();
		fScaleY = (double) rDisplaySize.Height() / aOrgSize.Height();

		for ( USHORT j = 0, nPolyCount = rContour.Count(); j < nPolyCount; j++ )
		{
			Polygon& rPoly = rContour[ j ];

			for ( USHORT i = 0, nCount = rPoly.GetSize(); i < nCount; i++ )
			{
				if ( bPixelMap )
					aNewPoint = pOutDev->PixelToLogic( rPoly[ i ], aDispMap  );
				else
					aNewPoint = pOutDev->LogicToLogic( rPoly[ i ], aGrfMap, aDispMap  );

				rPoly[ i ] = Point( FRound( aNewPoint.X() * fScaleX ), FRound( aNewPoint.Y() * fScaleY ) );
			}
		}
	}
}


/*************************************************************************
|*
|* Durchschleifen an SuperClass; keine virt. Methoden, um
|* bei IF-Aenderungen nicht inkompatibel zu werden
|*
\************************************************************************/

void SvxContourDlg::SetExecState( BOOL bEnable )
{
	pSuperClass->SetExecState( bEnable );
}

void SvxContourDlg::SetGraphic( const Graphic& rGraphic )
{
	pSuperClass->SetGraphic( rGraphic );
}

void SvxContourDlg::SetGraphicLinked( BOOL bGraphicLinked )
{
	pSuperClass->SetGraphicLinked( bGraphicLinked );
}

const Graphic& SvxContourDlg::GetGraphic() const
{
	return pSuperClass->GetGraphic();
}

BOOL SvxContourDlg::IsGraphicChanged() const
{
	return pSuperClass->IsGraphicChanged();
}

void SvxContourDlg::SetPolyPolygon( const PolyPolygon& rPolyPoly )
{
	pSuperClass->SetPolyPolygon( rPolyPoly );
}

PolyPolygon SvxContourDlg::GetPolyPolygon()
{
	return pSuperClass->GetPolyPolygon( TRUE );
}

void SvxContourDlg::SetEditingObject( void* pObj )
{
	pSuperClass->SetEditingObject( pObj );
}

const void* SvxContourDlg::GetEditingObject() const
{
	return pSuperClass->GetEditingObject();
}

void SvxContourDlg::Update( const Graphic& rGraphic, BOOL bGraphicLinked,
							const PolyPolygon* pPolyPoly, void* pEditingObj )
{
	pSuperClass->UpdateGraphic( rGraphic, bGraphicLinked, pPolyPoly, pEditingObj );
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

SvxSuperContourDlg::SvxSuperContourDlg( SfxBindings *_pBindings, SfxChildWindow *pCW,
                                        Window* _pParent, const ResId& rResId ) :
        SvxContourDlg       ( _pBindings, pCW, _pParent, rResId ),
        pCheckObj           ( NULL ),
        aContourItem        ( SID_CONTOUR_EXEC, *this, *_pBindings ),
        aTbx1               ( this, ResId( TBX1, *rResId.GetResMgr() ) ),
        aMtfTolerance       ( this, ResId( MTF_TOLERANCE, *rResId.GetResMgr() ) ),
		aContourWnd			( this, ResId( CTL_CONTOUR, *rResId.GetResMgr() ) ),
        aStbStatus          ( this, WB_BORDER | WB_3DLOOK | WB_LEFT ),
        nGrfChanged         ( 0UL ),
        bExecState          ( FALSE ),
		bGraphicLinked		( FALSE ),
		maImageList			( SVX_RES( CD_IMAPDLG ) ),
		maImageListH		( SVX_RES( CDH_IMAPDLG ) )
{
	ApplyImageList();

	FreeResource();

	SvxContourDlg::SetSuperClass( *this );

	aContourWnd.SetMousePosLink( LINK( this, SvxSuperContourDlg, MousePosHdl ) );
	aContourWnd.SetGraphSizeLink( LINK( this, SvxSuperContourDlg, GraphSizeHdl ) );
	aContourWnd.SetUpdateLink( LINK( this, SvxSuperContourDlg, StateHdl ) );
	aContourWnd.SetPipetteHdl( LINK( this, SvxSuperContourDlg, PipetteHdl ) );
	aContourWnd.SetPipetteClickHdl( LINK( this, SvxSuperContourDlg, PipetteClickHdl ) );
	aContourWnd.SetWorkplaceClickHdl( LINK( this, SvxSuperContourDlg, WorkplaceClickHdl ) );

	const Size	    aTbxSize( aTbx1.CalcWindowSizePixel() );
	Point		    aPos( aTbx1.GetPosPixel() );
  	SvtMiscOptions  aMiscOptions;

    aMiscOptions.AddListener( LINK( this, SvxSuperContourDlg, MiscHdl ) );

    aTbx1.SetOutStyle( aMiscOptions.GetToolboxStyle() );
	aTbx1.SetSizePixel( aTbxSize );
	aTbx1.SetSelectHdl( LINK( this, SvxSuperContourDlg, Tbx1ClickHdl ) );

	aPos.X() += aTbxSize.Width() + LogicToPixel( Size( 3, 0 ), MapMode( MAP_APPFONT ) ).Width();
	aMtfTolerance.SetPosPixel( aPos );
	aMtfTolerance.SetValue( 10L );

	SetMinOutputSizePixel( aLastSize = GetOutputSizePixel() );

	aStbStatus.InsertItem( 1, 130, SIB_LEFT | SIB_IN | SIB_AUTOSIZE );
	aStbStatus.InsertItem( 2, 10 + GetTextWidth( String::CreateFromAscii( " 9999,99 cm / 9999,99 cm " ) ), SIB_CENTER | SIB_IN );
	aStbStatus.InsertItem( 3, 10 + GetTextWidth( String::CreateFromAscii( " 9999,99 cm x 9999,99 cm " ) ), SIB_CENTER | SIB_IN );
	aStbStatus.InsertItem( 4, 20, SIB_CENTER | SIB_IN );

	Resize();

	aUpdateTimer.SetTimeout( 100 );
	aUpdateTimer.SetTimeoutHdl( LINK( this, SvxSuperContourDlg, UpdateHdl ) );

	aCreateTimer.SetTimeout( 50 );
	aCreateTimer.SetTimeoutHdl( LINK( this, SvxSuperContourDlg, CreateHdl ) );
}


/*************************************************************************
|*
|* Dtor
|*
\************************************************************************/

SvxSuperContourDlg::~SvxSuperContourDlg()
{
}


/*************************************************************************
|*
|* Resize-Methode
|*
\************************************************************************/

void SvxSuperContourDlg::Resize()
{
	SfxFloatingWindow::Resize();

	Size aMinSize( GetMinOutputSizePixel() );
	Size aNewSize( GetOutputSizePixel() );

	if ( aNewSize.Height() >= aMinSize.Height() )
	{
        Size    _aSize( aStbStatus.GetSizePixel() );
        Point   aPoint( 0, aNewSize.Height() - _aSize.Height() );

		// StatusBar positionieren
        aStbStatus.SetPosSizePixel( aPoint, Size( aNewSize.Width(), _aSize.Height() ) );
		aStbStatus.Show();

		// EditWindow positionieren
        _aSize.Width() = aNewSize.Width() - 18;
        _aSize.Height() = aPoint.Y() - aContourWnd.GetPosPixel().Y() - 6;
        aContourWnd.SetSizePixel( _aSize );

		aLastSize = aNewSize;
	}
}


/*************************************************************************
|*
|* Close-Methode
|*
\************************************************************************/

BOOL SvxSuperContourDlg::Close()
{
	BOOL bRet = TRUE;

	if ( aTbx1.IsItemEnabled( TBI_APPLY ) )
	{
		QueryBox	aQBox( this, WB_YES_NO_CANCEL | WB_DEF_YES,
						   String( CONT_RESID( STR_CONTOURDLG_MODIFY ) ) );
		const long	nRet = aQBox.Execute();

		if ( nRet == RET_YES )
		{
			SfxBoolItem aBoolItem( SID_CONTOUR_EXEC, TRUE );
			GetBindings().GetDispatcher()->Execute(
				SID_CONTOUR_EXEC, SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD, &aBoolItem, 0L );
		}
		else if ( nRet == RET_CANCEL )
			bRet = FALSE;
	}

	return( bRet ? SfxFloatingWindow::Close() : FALSE );
}


/*************************************************************************
|*
|* Enabled oder disabled alle Controls
|*
\************************************************************************/

void SvxSuperContourDlg::SetExecState( BOOL bEnable )
{
	bExecState = bEnable;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxSuperContourDlg::SetGraphic( const Graphic& rGraphic )
{
	aUndoGraphic = aRedoGraphic = Graphic();
	aGraphic = rGraphic;
	nGrfChanged = 0UL;
	aContourWnd.SetGraphic( aGraphic );
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxSuperContourDlg::SetPolyPolygon( const PolyPolygon& rPolyPoly )
{
	DBG_ASSERT(  aContourWnd.GetGraphic().GetType() != GRAPHIC_NONE, "Graphic must've been set first!" );

	PolyPolygon		aPolyPoly( rPolyPoly );
	const MapMode	aMap100( MAP_100TH_MM );
	const MapMode	aGrfMap( aGraphic.GetPrefMapMode() );
	OutputDevice*	pOutDev = Application::GetDefaultDevice();
	BOOL			bPixelMap = aGrfMap.GetMapUnit() == MAP_PIXEL;

	for ( USHORT j = 0, nPolyCount = aPolyPoly.Count(); j < nPolyCount; j++ )
	{
		Polygon& rPoly = aPolyPoly[ j ];

		for ( USHORT i = 0, nCount = rPoly.GetSize(); i < nCount; i++ )
		{
			Point& rPt = rPoly[ i ];

			if ( !bPixelMap )
				rPt = pOutDev->LogicToPixel( rPt, aGrfMap );

			rPt = pOutDev->PixelToLogic( rPt, aMap100 );
		}
	}

	aContourWnd.SetPolyPolygon( aPolyPoly );
	aContourWnd.GetSdrModel()->SetChanged( sal_True );
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

PolyPolygon SvxSuperContourDlg::GetPolyPolygon( BOOL bRescaleToGraphic )
{
	PolyPolygon	aRetPolyPoly( aContourWnd.GetPolyPolygon() );

	if ( bRescaleToGraphic )
	{
		const MapMode	aMap100( MAP_100TH_MM );
		const MapMode	aGrfMap( aGraphic.GetPrefMapMode() );
		OutputDevice*	pOutDev = Application::GetDefaultDevice();
		BOOL			bPixelMap = aGrfMap.GetMapUnit() == MAP_PIXEL;

		for ( USHORT j = 0, nPolyCount = aRetPolyPoly.Count(); j < nPolyCount; j++ )
		{
			Polygon& rPoly = aRetPolyPoly[ j ];

			for ( USHORT i = 0, nCount = rPoly.GetSize(); i < nCount; i++ )
			{
				Point& rPt = rPoly[ i ];

				rPt = pOutDev->LogicToPixel( rPt, aMap100  );

				if ( !bPixelMap )
					rPt = pOutDev->PixelToLogic( rPt, aGrfMap  );
			}
		}
	}

	return aRetPolyPoly;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxSuperContourDlg::UpdateGraphic( const Graphic& rGraphic, BOOL _bGraphicLinked,
								 const PolyPolygon* pPolyPoly, void* pEditingObj )
{
	aUpdateGraphic = rGraphic;
    bUpdateGraphicLinked = _bGraphicLinked;
	pUpdateEditingObject = pEditingObj;

	if ( pPolyPoly )
		aUpdatePolyPoly = *pPolyPoly;
	else
		aUpdatePolyPoly = PolyPolygon();

	aUpdateTimer.Start();
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

BOOL SvxSuperContourDlg::IsUndoPossible() const
{
	return aUndoGraphic.GetType() != GRAPHIC_NONE;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

BOOL SvxSuperContourDlg::IsRedoPossible() const
{
	return aRedoGraphic.GetType() != GRAPHIC_NONE;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxSuperContourDlg::DoAutoCreate()
{
	aCreateTimer.Start();
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

void SvxSuperContourDlg::ReducePoints( const long nTol )
{
	PolyPolygon aPolyPoly( GetPolyPolygon( FALSE ) );

	if ( aPolyPoly.Count() )
	{
		const MapMode	aMapMode( MAP_100TH_MM );
		const long		nTol2 = nTol * nTol;
		Polygon&		rPoly = aPolyPoly[ 0 ];
		OutputDevice*	pOutDev = Application::GetDefaultDevice();
		Point			aPtPix;
		const USHORT	nSize = rPoly.GetSize();
		USHORT			nCounter = 0;

		if ( nSize )
			aPtPix = pOutDev->LogicToPixel( rPoly[ 0 ], aMapMode );

		for( USHORT i = 1; i < nSize; i++ )
		{
			const Point&	rNewPt = rPoly[ i ];
			const Point		aNewPtPix( pOutDev->LogicToPixel( rNewPt, aMapMode ) );

			const long nDistX = aNewPtPix.X() - aPtPix.X();
			const long nDistY = aNewPtPix.Y() - aPtPix.Y();

			if( ( nDistX * nDistX + nDistY * nDistY ) >= nTol2 )
			{
				rPoly[ ++nCounter ] = rNewPt;
				aPtPix = aNewPtPix;
			}
		}

		rPoly.SetSize( nCounter );
		aContourWnd.SetPolyPolygon( aPolyPoly );
		aContourWnd.GetSdrModel()->SetChanged( sal_True );
	}
}


/*************************************************************************
|*
|* Click-Hdl fuer ToolBox
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, Tbx1ClickHdl, ToolBox*, pTbx )
{
	USHORT nNewItemId = pTbx->GetCurItemId();

	switch( pTbx->GetCurItemId() )
	{
		case( TBI_APPLY ):
		{
			SfxBoolItem aBoolItem( SID_CONTOUR_EXEC, TRUE );
			GetBindings().GetDispatcher()->Execute(
				SID_CONTOUR_EXEC, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aBoolItem, 0L );
		}
		break;

		case( TBI_WORKPLACE ):
		{
			if ( aTbx1.IsItemChecked( TBI_WORKPLACE ) )
			{
				QueryBox aQBox( this, WB_YES_NO | WB_DEF_NO, String( CONT_RESID( STR_CONTOURDLG_WORKPLACE ) ) );

				if ( !aContourWnd.IsContourChanged() || ( aQBox.Execute() == RET_YES ) )
					aContourWnd.SetWorkplaceMode( TRUE );
				else
					aTbx1.CheckItem( TBI_WORKPLACE, FALSE );
			}
			else
				aContourWnd.SetWorkplaceMode( FALSE );
		}
		break;

		case( TBI_SELECT ):
		{
			pTbx->CheckItem( nNewItemId, TRUE );
			aContourWnd.SetEditMode( TRUE );
		}
		break;

		case( TBI_RECT ):
		{
			pTbx->CheckItem( nNewItemId, TRUE );
			aContourWnd.SetObjKind( OBJ_RECT );
		}
		break;

		case( TBI_CIRCLE ):
		{
			pTbx->CheckItem( nNewItemId, TRUE );
			aContourWnd.SetObjKind( OBJ_CIRC );

		}
		break;

		case( TBI_POLY ):
		{
			pTbx->CheckItem( nNewItemId, TRUE );
			aContourWnd.SetObjKind( OBJ_POLY );
		}
		break;

		case( TBI_FREEPOLY ):
		{
			pTbx->CheckItem( nNewItemId, TRUE );
			aContourWnd.SetObjKind( OBJ_FREEFILL );
		}
		break;

		case( TBI_POLYEDIT ):
			aContourWnd.SetPolyEditMode( pTbx->IsItemChecked( TBI_POLYEDIT ) ? SID_BEZIER_MOVE : 0 );
		break;

		case( TBI_POLYMOVE ):
			aContourWnd.SetPolyEditMode( SID_BEZIER_MOVE );
		break;

		case( TBI_POLYINSERT ):
			aContourWnd.SetPolyEditMode( SID_BEZIER_INSERT );
		break;

		case( TBI_POLYDELETE ):
			aContourWnd.GetSdrView()->DeleteMarkedPoints();
		break;

		case( TBI_UNDO ):
		{
			nGrfChanged = nGrfChanged ? nGrfChanged-- : 0UL;
			aRedoGraphic = aGraphic;
			aGraphic = aUndoGraphic;
			aUndoGraphic = Graphic();
			aContourWnd.SetGraphic( aGraphic, FALSE );
		}
		break;

		case( TBI_REDO ):
		{
			nGrfChanged++;
			aUndoGraphic = aGraphic;
			aGraphic = aRedoGraphic;
			aRedoGraphic = Graphic();
			aContourWnd.SetGraphic( aGraphic, FALSE );
		}
		break;

		case( TBI_AUTOCONTOUR ):
			aCreateTimer.Start();
		break;

		case( TBI_PIPETTE ):
		{
			BOOL bPipette = aTbx1.IsItemChecked( TBI_PIPETTE );

			if ( !bPipette )
				aStbStatus.Invalidate();
			else if ( bGraphicLinked )
			{
				QueryBox aQBox( this, WB_YES_NO | WB_DEF_YES, String( CONT_RESID( STR_CONTOURDLG_LINKED ) ) );

				if ( aQBox.Execute() != RET_YES )
				{
					aTbx1.CheckItem( TBI_PIPETTE, bPipette = FALSE );
					aStbStatus.Invalidate();
				}
			}

			aContourWnd.SetPipetteMode( bPipette );
		}
		break;

		default:
		break;
	}

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, MousePosHdl, ContourWindow*, pWnd )
{
	String aStr;
	const FieldUnit eFieldUnit = GetModuleFieldUnit();
	const Point& rMousePos = pWnd->GetMousePos();
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
	const sal_Unicode cSep = aLocaleWrapper.getNumDecimalSep().GetChar(0);

	aStr.Assign( GetUnitString( rMousePos.X(), eFieldUnit, cSep ) );
	aStr.Append( String::CreateFromAscii( " / " ) );
	aStr.Append( GetUnitString( rMousePos.Y(), eFieldUnit, cSep ) );

	aStbStatus.SetItemText( 2, aStr );

	return 0L;
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, GraphSizeHdl, ContourWindow*, pWnd )
{
	String aStr;
	const FieldUnit eFieldUnit = GetModuleFieldUnit();
	const Size& rSize = pWnd->GetGraphicSize();
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
	const sal_Unicode cSep = aLocaleWrapper.getNumDecimalSep().GetChar(0);

	aStr.Assign( GetUnitString( rSize.Width(), eFieldUnit, cSep ) );
	aStr.Append( String::CreateFromAscii( " x " ) );
	aStr.Append( GetUnitString( rSize.Height(), eFieldUnit, cSep ) );

	aStbStatus.SetItemText( 3, aStr );

	return 0L;
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, UpdateHdl, Timer*, EMPTYARG )
{
	aUpdateTimer.Stop();

	if ( pUpdateEditingObject != pCheckObj )
	{
		if( !GetEditingObject() )
			aContourWnd.GrabFocus();

		SetGraphic( aUpdateGraphic );
		SetPolyPolygon( aUpdatePolyPoly );
		SetEditingObject( pUpdateEditingObject );
		bGraphicLinked = bUpdateGraphicLinked;

		aUpdateGraphic = Graphic();
		aUpdatePolyPoly = PolyPolygon();
		bUpdateGraphicLinked = FALSE;

		aContourWnd.GetSdrModel()->SetChanged( sal_False );
	}

	GetBindings().Invalidate( SID_CONTOUR_EXEC );

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, CreateHdl, Timer*, EMPTYARG )
{
	aCreateTimer.Stop();

	const Rectangle	aWorkRect = aContourWnd.LogicToPixel( aContourWnd.GetWorkRect(), MapMode( MAP_100TH_MM ) );
	const Graphic&	rGraphic = aContourWnd.GetGraphic();
	const BOOL		bValid = aWorkRect.Left() != aWorkRect.Right() && aWorkRect.Top() != aWorkRect.Bottom();

	EnterWait();
	SetPolyPolygon( CreateAutoContour( rGraphic, bValid ? &aWorkRect : NULL ) );
	LeaveWait();

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, StateHdl, ContourWindow*, pWnd )
{
	const SdrObject*	pObj = pWnd->GetSelectedSdrObject();
	const SdrView*		pView = pWnd->GetSdrView();
	const BOOL			bPolyEdit = ( pObj != NULL ) && pObj->ISA( SdrPathObj );
	const BOOL			bDrawEnabled = !( bPolyEdit && aTbx1.IsItemChecked( TBI_POLYEDIT ) );
	const BOOL			bPipette = aTbx1.IsItemChecked( TBI_PIPETTE );
	const BOOL			bWorkplace = aTbx1.IsItemChecked( TBI_WORKPLACE );
	const BOOL			bDontHide = !( bPipette || bWorkplace );
	const BOOL			bBitmap = pWnd->GetGraphic().GetType() == GRAPHIC_BITMAP;

	aTbx1.EnableItem( TBI_APPLY, bDontHide && bExecState && pWnd->IsChanged() );

	aTbx1.EnableItem( TBI_WORKPLACE, !bPipette && bDrawEnabled );

	aTbx1.EnableItem( TBI_SELECT, bDontHide && bDrawEnabled );
	aTbx1.EnableItem( TBI_RECT, bDontHide && bDrawEnabled );
	aTbx1.EnableItem( TBI_CIRCLE, bDontHide && bDrawEnabled );
	aTbx1.EnableItem( TBI_POLY, bDontHide && bDrawEnabled );
	aTbx1.EnableItem( TBI_FREEPOLY, bDontHide && bDrawEnabled );

	aTbx1.EnableItem( TBI_POLYEDIT, bDontHide && bPolyEdit );
	aTbx1.EnableItem( TBI_POLYMOVE, bDontHide && !bDrawEnabled );
	aTbx1.EnableItem( TBI_POLYINSERT, bDontHide && !bDrawEnabled );
	aTbx1.EnableItem( TBI_POLYDELETE, bDontHide && !bDrawEnabled && pView->IsDeleteMarkedPointsPossible() );

	aTbx1.EnableItem( TBI_AUTOCONTOUR, bDontHide && bDrawEnabled );
	aTbx1.EnableItem( TBI_PIPETTE, !bWorkplace && bDrawEnabled && bBitmap );

	aTbx1.EnableItem( TBI_UNDO, bDontHide && IsUndoPossible() );
	aTbx1.EnableItem( TBI_REDO, bDontHide && IsRedoPossible() );

	if ( bPolyEdit )
	{
		USHORT nId = 0;

		switch( pWnd->GetPolyEditMode() )
		{
			case( SID_BEZIER_MOVE ): nId = TBI_POLYMOVE; break;
			case( SID_BEZIER_INSERT ): nId = TBI_POLYINSERT; break;

			default:
			break;
		}

		aTbx1.CheckItem( nId, TRUE );
	}
	else
	{
		aTbx1.CheckItem( TBI_POLYEDIT, FALSE );
		aTbx1.CheckItem( TBI_POLYMOVE, TRUE );
		aTbx1.CheckItem( TBI_POLYINSERT, FALSE );
		pWnd->SetPolyEditMode( 0 );
	}

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, PipetteHdl, ContourWindow*, pWnd )
{
	const Color& rOldLineColor = aStbStatus.GetLineColor();
	const Color& rOldFillColor = aStbStatus.GetFillColor();

	Rectangle		aRect( aStbStatus.GetItemRect( 4 ) );
	const Color&	rColor = pWnd->GetPipetteColor();

	aStbStatus.SetLineColor( rColor );
	aStbStatus.SetFillColor( rColor );

	aRect.Left() += 4;
	aRect.Top() += 4;
	aRect.Right() -= 4;
	aRect.Bottom() -= 4;

	aStbStatus.DrawRect( aRect );

	aStbStatus.SetLineColor( rOldLineColor );
	aStbStatus.SetFillColor( rOldFillColor );

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, PipetteClickHdl, ContourWindow*, pWnd )
{
	if ( pWnd->IsClickValid() )
	{
		Bitmap			aMask;
		const Color&	rColor = pWnd->GetPipetteColor();

		EnterWait();

		if( aGraphic.GetType() == GRAPHIC_BITMAP )
		{
			Bitmap		aBmp( aGraphic.GetBitmap() );
			const long	nTol = static_cast<long>(aMtfTolerance.GetValue() * 255L / 100L);

			aMask = aBmp.CreateMask( rColor, nTol );

			if( aGraphic.IsTransparent() )
				aMask.CombineSimple( aGraphic.GetBitmapEx().GetMask(), BMP_COMBINE_OR );

			if( !!aMask )
			{
				QueryBox	aQBox( this, WB_YES_NO | WB_DEF_YES, String( CONT_RESID( STR_CONTOURDLG_NEWPIPETTE ) ) );
				BOOL		bNewContour;

				aRedoGraphic = Graphic();
				aUndoGraphic = aGraphic;
				aGraphic = Graphic( BitmapEx( aBmp, aMask ) );
				nGrfChanged++;

				bNewContour = ( aQBox.Execute() == RET_YES );
				pWnd->SetGraphic( aGraphic, bNewContour );

				if( bNewContour )
					aCreateTimer.Start();
			}
		}

		LeaveWait();
	}

	aTbx1.CheckItem( TBI_PIPETTE, FALSE );
	pWnd->SetPipetteMode( FALSE );
	aStbStatus.Invalidate();

	return 0L;
}


/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, WorkplaceClickHdl, ContourWindow*, pWnd )
{
	aTbx1.CheckItem( TBI_WORKPLACE, FALSE );
	aTbx1.CheckItem( TBI_SELECT, TRUE );
	pWnd->SetWorkplaceMode( FALSE );

	return 0L;
}

void SvxSuperContourDlg::ApplyImageList()
{
	bool bHighContrast = GetDisplayBackground().GetColor().IsDark() != 0;

    ImageList& rImgLst = bHighContrast ? maImageListH : maImageList;

	aTbx1.SetImageList( rImgLst );
}

void SvxSuperContourDlg::DataChanged( const DataChangedEvent& rDCEvt )
{
    SfxFloatingWindow::DataChanged( rDCEvt );

	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_STYLE) )
            ApplyImageList();
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_LINK( SvxSuperContourDlg, MiscHdl, void*, EMPTYARG )
{
   	SvtMiscOptions aMiscOptions;
    aTbx1.SetOutStyle( aMiscOptions.GetToolboxStyle() );

    return 0L;
}

