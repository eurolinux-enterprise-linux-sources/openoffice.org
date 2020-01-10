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
#include "precompiled_sc.hxx"

#include <svx/impgrf.hxx>
#include <svx/opengrf.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdomedia.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdview.hxx>
#include <svtools/filter.hxx>
#include <svtools/stritem.hxx>
#include <vcl/msgbox.hxx>
#include <tools/urlobj.hxx>
#include <avmedia/mediawindow.hxx>
#include <vcl/svapp.hxx>

#include "fuinsert.hxx"
#include "tabvwsh.hxx"
#include "drwlayer.hxx"
#include "drawview.hxx"
#include "document.hxx"
#include "scresid.hxx"
#include "progress.hxx"
#include "sc.hrc"



////========================================================================
////	class ImportProgress
////
////  Bemerkung:
////	Diese Klasse stellt lediglich den Handler fuer den ImportProgress des
////	Grafikfilters bereit.
////========================================================================
//
//class ImportProgress
//{
//public:
//		ImportProgress( GraphicFilter& rFilter );
//		~ImportProgress();
//
//	DECL_LINK( Update, GraphicFilter* );
//
//private:
//	ScProgress aProgress;
//};
//
////------------------------------------------------------------------------
//
//ImportProgress::ImportProgress( GraphicFilter& rFilter )
//	: aProgress( NULL, // SfxViewFrame*, NULL == alle Docs locken
//				 String( ScResId(STR_INSERTGRAPHIC) ),
//				 100 )
//{
//	rFilter.SetUpdatePercentHdl( LINK( this, ImportProgress, Update) );
//}
//
////------------------------------------------------------------------------
//
//__EXPORT ImportProgress::~ImportProgress()
//{
//	aProgress.SetState( 100 );
//}
//
////------------------------------------------------------------------------
//
//IMPL_LINK( ImportProgress, Update, GraphicFilter*, pGraphicFilter )
//{
//	aProgress.SetState( pGraphicFilter->GetPercent() );
//	return 0;
//}


//------------------------------------------------------------------------

void SC_DLLPUBLIC ScLimitSizeOnDrawPage( Size& rSize, Point& rPos, const Size& rPage )
{
	if ( !rPage.Width() || !rPage.Height() )
		return;

	Size aPageSize = rPage;
	BOOL bNegative = aPageSize.Width() < 0;
	if ( bNegative )
	{
		//	make everything positive temporarily
		aPageSize.Width() = -aPageSize.Width();
		rPos.X() = -rPos.X() - rSize.Width();
	}

	if ( rSize.Width() > aPageSize.Width() || rSize.Height() > aPageSize.Height() )
	{
		double fX = aPageSize.Width()  / (double) rSize.Width();
		double fY = aPageSize.Height() / (double) rSize.Height();

		if ( fX < fY )
		{
			rSize.Width()  = aPageSize.Width();
			rSize.Height() = (long) ( rSize.Height() * fX );
		}
		else
		{
			rSize.Height() = aPageSize.Height();
			rSize.Width()  = (long) ( rSize.Width() * fY );
		}

		if (!rSize.Width())
			rSize.Width() = 1;
		if (!rSize.Height())
			rSize.Height() = 1;
	}

	if ( rPos.X() + rSize.Width() > aPageSize.Width() )
		rPos.X() = aPageSize.Width() - rSize.Width();
	if ( rPos.Y() + rSize.Height() > aPageSize.Height() )
		rPos.Y() = aPageSize.Height() - rSize.Height();

	if ( bNegative )
		rPos.X() = -rPos.X() - rSize.Width();		// back to real position
}

//------------------------------------------------------------------------

void lcl_InsertGraphic( const Graphic& rGraphic,
						const String& rFileName, const String& rFilterName, BOOL bAsLink, BOOL bApi,
						ScTabViewShell*	pViewSh, Window* pWindow, SdrView* pView )
{
	//	#74778# set the size so the graphic has its original pixel size
	//	at 100% view scale (as in SetMarkedOriginalSize),
	//	instead of respecting the current view scale

	ScDrawView* pDrawView = pViewSh->GetScDrawView();
	MapMode aSourceMap = rGraphic.GetPrefMapMode();
	MapMode aDestMap( MAP_100TH_MM );
	if ( aSourceMap.GetMapUnit() == MAP_PIXEL && pDrawView )
	{
		Fraction aScaleX, aScaleY;
		pDrawView->CalcNormScale( aScaleX, aScaleY );
		aDestMap.SetScaleX(aScaleX);
		aDestMap.SetScaleY(aScaleY);
	}
	Size aLogicSize = pWindow->LogicToLogic(
							rGraphic.GetPrefSize(), &aSourceMap, &aDestMap );

	//	Limit size

	SdrPageView* pPV  = pView->GetSdrPageView();
	SdrPage* pPage = pPV->GetPage();
	Point aInsertPos = pViewSh->GetInsertPos();

	ScViewData* pData = pViewSh->GetViewData();
	if ( pData->GetDocument()->IsNegativePage( pData->GetTabNo() ) )
		aInsertPos.X() -= aLogicSize.Width();		// move position to left edge

	ScLimitSizeOnDrawPage( aLogicSize, aInsertPos, pPage->GetSize() );

	Rectangle aRect ( aInsertPos, aLogicSize );

	SdrGrafObj* pObj = new SdrGrafObj( rGraphic, aRect );

    // #118522# calling SetGraphicLink here doesn't work

	//	#49961# Path is no longer used as name for the graphics object

	ScDrawLayer* pLayer = (ScDrawLayer*) pView->GetModel();
	String aName = pLayer->GetNewGraphicName();					// "Grafik x"
	pObj->SetName(aName);

	//	don't select if from (dispatch) API, to allow subsequent cell operations
	ULONG nInsOptions = bApi ? SDRINSERT_DONTMARK : 0;
	pView->InsertObjectAtView( pObj, *pPV, nInsOptions );

    // #118522# SetGraphicLink has to be used after inserting the object,
    // otherwise an empty graphic is swapped in and the contact stuff crashes.
    // See #i37444#.
	if ( bAsLink )
		pObj->SetGraphicLink( rFileName, rFilterName );
}

//------------------------------------------------------------------------

void lcl_InsertMedia( const ::rtl::OUString& rMediaURL, bool bApi,
					  ScTabViewShell* pViewSh, Window* pWindow, SdrView* pView,
					  const Size& rPrefSize )
{
	SdrPageView* 	pPV  = pView->GetSdrPageView();
	SdrPage* 		pPage = pPV->GetPage();
	ScViewData* 	pData = pViewSh->GetViewData();
	Point 			aInsertPos( pViewSh->GetInsertPos() );
	Size 			aSize;
	
	if( rPrefSize.Width() && rPrefSize.Height() )
	{
		if( pWindow )
			aSize = pWindow->PixelToLogic( rPrefSize, MAP_100TH_MM );
		else
			aSize = Application::GetDefaultDevice()->PixelToLogic( rPrefSize, MAP_100TH_MM );
	}
	else
		aSize = Size( 5000, 5000 );

	ScLimitSizeOnDrawPage( aSize, aInsertPos, pPage->GetSize() );
	
	if( pData->GetDocument()->IsNegativePage( pData->GetTabNo() ) )
		aInsertPos.X() -= aSize.Width();

	SdrMediaObj* pObj = new SdrMediaObj( Rectangle( aInsertPos, aSize ) );
	
	pObj->setURL( rMediaURL ); 
	pView->InsertObjectAtView( pObj, *pPV, bApi ? SDRINSERT_DONTMARK : 0 );
}

/*************************************************************************
|*
|* FuInsertGraphic::Konstruktor
|*
\************************************************************************/

#ifdef _MSC_VER
#pragma optimize("",off)
#endif

FuInsertGraphic::FuInsertGraphic( ScTabViewShell*	pViewSh,
								  Window*			pWin,
                                  ScDrawView*       pViewP,
								  SdrModel*			pDoc,
								  SfxRequest&		rReq )
       : FuPoor(pViewSh, pWin, pViewP, pDoc, rReq)
{
	const SfxItemSet* pReqArgs = rReq.GetArgs();
	const SfxPoolItem* pItem;
	if ( pReqArgs &&
		 pReqArgs->GetItemState( SID_INSERT_GRAPHIC, TRUE, &pItem ) == SFX_ITEM_SET )
	{
		String aFileName = ((const SfxStringItem*)pItem)->GetValue();

		String aFilterName;
		if ( pReqArgs->GetItemState( FN_PARAM_FILTER, TRUE, &pItem ) == SFX_ITEM_SET )
			aFilterName = ((const SfxStringItem*)pItem)->GetValue();

		BOOL bAsLink = FALSE;
		if ( pReqArgs->GetItemState( FN_PARAM_1, TRUE, &pItem ) == SFX_ITEM_SET )
			bAsLink = ((const SfxBoolItem*)pItem)->GetValue();

		Graphic aGraphic;
        int nError = ::LoadGraphic( aFileName, aFilterName, aGraphic, ::GetGrfFilter() );
        if ( nError == GRFILTER_OK )
        {
			lcl_InsertGraphic( aGraphic, aFileName, aFilterName, bAsLink, TRUE, pViewSh, pWindow, pView );
        }
	}
	else
	{
		SvxOpenGraphicDialog aDlg(ScResId(STR_INSERTGRAPHIC));

		if( aDlg.Execute() == GRFILTER_OK )
		{
			Graphic aGraphic;
            int nError = aDlg.GetGraphic(aGraphic);
			if( nError == GRFILTER_OK )
			{
				String aFileName = aDlg.GetPath();
				String aFilterName = aDlg.GetCurrentFilter();
				BOOL bAsLink = aDlg.IsAsLink();

				lcl_InsertGraphic( aGraphic, aFileName, aFilterName, bAsLink, FALSE, pViewSh, pWindow, pView );

				//	append items for recording
				rReq.AppendItem( SfxStringItem( SID_INSERT_GRAPHIC, aFileName ) );
				rReq.AppendItem( SfxStringItem( FN_PARAM_FILTER, aFilterName ) );
				rReq.AppendItem( SfxBoolItem( FN_PARAM_1, bAsLink ) );
				rReq.Done();
			}
			else
			{
				//	error is handled in SvxOpenGraphicDialog::GetGraphic

#if 0
				USHORT nRes = 0;
				switch ( nError )
				{
					case GRFILTER_OPENERROR:	nRes = SCSTR_GRFILTER_OPENERROR;	break;
					case GRFILTER_IOERROR:		nRes = SCSTR_GRFILTER_IOERROR;		break;
					case GRFILTER_FORMATERROR:	nRes = SCSTR_GRFILTER_FORMATERROR;	break;
					case GRFILTER_VERSIONERROR:	nRes = SCSTR_GRFILTER_VERSIONERROR;	break;
					case GRFILTER_FILTERERROR:	nRes = SCSTR_GRFILTER_FILTERERROR;	break;
					case GRFILTER_TOOBIG:		nRes = SCSTR_GRFILTER_TOOBIG;		break;
				}
				if ( nRes )
				{
					InfoBox aInfoBox( pWindow, String(ScResId(nRes)) );
					aInfoBox.Execute();
				}
				else
				{
					ULONG nStreamError = GetGrfFilter()->GetLastError().nStreamError;
					if( ERRCODE_NONE != nStreamError )
						ErrorHandler::HandleError( nStreamError );
				}
#endif
			}
		}
	}
}

/*************************************************************************
|*
|* FuInsertGraphic::Destruktor
|*
\************************************************************************/

FuInsertGraphic::~FuInsertGraphic()
{
}

/*************************************************************************
|*
|* FuInsertGraphic::Function aktivieren
|*
\************************************************************************/

void FuInsertGraphic::Activate()
{
	FuPoor::Activate();
}

/*************************************************************************
|*
|* FuInsertGraphic::Function deaktivieren
|*
\************************************************************************/

void FuInsertGraphic::Deactivate()
{
	FuPoor::Deactivate();
}

/*************************************************************************
|*
|* FuInsertMedia::Konstruktor
|*
\************************************************************************/

FuInsertMedia::FuInsertMedia( ScTabViewShell*	pViewSh,
							  Window*			pWin,
                              ScDrawView*       pViewP,
							  SdrModel*			pDoc,
							  SfxRequest&		rReq ) :
    FuPoor(pViewSh, pWin, pViewP, pDoc, rReq)
{
	::rtl::OUString 	aURL;
	const SfxItemSet*	pReqArgs = rReq.GetArgs();
	bool				bAPI = false;

	if( pReqArgs )
	{
		const SfxStringItem* pStringItem = PTR_CAST( SfxStringItem, &pReqArgs->Get( rReq.GetSlot() ) );
		
		if( pStringItem )
		{
			aURL = pStringItem->GetValue();
			bAPI = aURL.getLength();
		}
	}

	if( bAPI || ::avmedia::MediaWindow::executeMediaURLDialog( pWindow, aURL ) )
	{
		Size aPrefSize;

		if( pWin )
			pWin->EnterWait();
			
		if( !::avmedia::MediaWindow::isMediaURL( aURL, true, &aPrefSize ) )
		{
			if( pWin )
				pWin->LeaveWait();
			
			if( !bAPI )
				::avmedia::MediaWindow::executeFormatErrorBox( pWindow );
		}
		else
		{
			lcl_InsertMedia( aURL, bAPI, pViewSh, pWindow, pView, aPrefSize );
		
			if( pWin )
				pWin->LeaveWait();
		}
	}
}

/*************************************************************************
|*
|* FuInsertMedia::Destruktor
|*
\************************************************************************/

FuInsertMedia::~FuInsertMedia()
{
}

/*************************************************************************
|*
|* FuInsertMedia::Function aktivieren
|*
\************************************************************************/

void FuInsertMedia::Activate()
{
	FuPoor::Activate();
}

/*************************************************************************
|*
|* FuInsertMedia::Function deaktivieren
|*
\************************************************************************/

void FuInsertMedia::Deactivate()
{
	FuPoor::Deactivate();
}
