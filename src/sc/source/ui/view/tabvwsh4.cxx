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



// INCLUDE ---------------------------------------------------------------

#include <sfx2/topfrm.hxx>
#include "scitems.hxx"
#include <svx/eeitem.hxx>

#include <sfx2/app.hxx>
#include <svx/extrusionbar.hxx>
#include <svx/fontworkbar.hxx>
#include <svx/boxitem.hxx>
#include <svx/fmshell.hxx>
#include <svx/sizeitem.hxx>
#include <svx/boxitem.hxx>
#include <svx/prtqry.hxx>
#include <sfx2/request.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/dispatch.hxx>
#include <svtools/printdlg.hxx>
#include <svtools/whiter.hxx>
#include <svtools/moduleoptions.hxx>
#include <rtl/logfile.hxx>
#include <tools/urlobj.hxx>
#include <sfx2/docfile.hxx>

#include "tabvwsh.hxx"
#include "sc.hrc"
#include "globstr.hrc"
#include "stlpool.hxx"
#include "stlsheet.hxx"
#include "docsh.hxx"
#include "scmod.hxx"
#include "appoptio.hxx"
#include "rangeutl.hxx"
#include "printfun.hxx"
#include "drawsh.hxx"
#include "drformsh.hxx"
#include "editsh.hxx"
#include "pivotsh.hxx"
#include "auditsh.hxx"
#include "drtxtob.hxx"
#include "inputhdl.hxx"
#include "editutil.hxx"
#include "inputopt.hxx"
#include "inputwin.hxx"
#include "scresid.hxx"
#include "dbcolect.hxx"		// fuer ReImport
#include "reffact.hxx"
#include "viewuno.hxx"
#include "dispuno.hxx"
#include "anyrefdg.hxx"
#include "chgtrack.hxx"
#include "cellsh.hxx"
#include "oleobjsh.hxx"
#include "chartsh.hxx"
#include "graphsh.hxx"
#include "mediash.hxx"
#include "pgbrksh.hxx"
#include "dpobject.hxx"
#include "prevwsh.hxx"
#include "tpprint.hxx"
#include "scextopt.hxx"
#include "printopt.hxx"
#include "drawview.hxx"
#include "fupoor.hxx"
#include "navsett.hxx"
#include "sc.hrc" //CHINA001
#include "scabstdlg.hxx" //CHINA001
#include "externalrefmgr.hxx"

void ActivateOlk( ScViewData* pViewData );
void DeActivateOlk( ScViewData* pViewData );

extern SfxViewShell* pScActiveViewShell;			// global.cxx

using namespace com::sun::star;

// STATIC DATA -----------------------------------------------------------

USHORT ScTabViewShell::nInsertCtrlState = SID_INSERT_GRAPHIC;
USHORT ScTabViewShell::nInsCellsCtrlState = 0;
USHORT ScTabViewShell::nInsObjCtrlState = SID_INSERT_DIAGRAM;

// -----------------------------------------------------------------------

void __EXPORT ScTabViewShell::Activate(BOOL bMDI)
{
	SfxViewShell::Activate(bMDI);

	//	hier kein GrabFocus, sonst gibt's Probleme wenn etwas inplace editiert wird!

	if ( bMDI )
	{
		//	fuer Eingabezeile (ClearCache)
		ScModule* pScMod = SC_MOD();
		pScMod->ViewShellChanged();

		ActivateView( TRUE, bFirstActivate );
		ActivateOlk( GetViewData() );

		//	#56870# AutoCorrect umsetzen, falls der Writer seins neu angelegt hat
		UpdateDrawTextOutliner();

		//	RegisterNewTargetNames gibts nicht mehr

		SfxViewFrame* pThisFrame  = GetViewFrame();
		if ( pInputHandler && pThisFrame->HasChildWindow(FID_INPUTLINE_STATUS) )
		{
			//	eigentlich nur beim Reload (letzte Version) noetig:
			//	Das InputWindow bleibt stehen, aber die View mitsamt InputHandler wird
			//	neu angelegt, darum muss der InputHandler am InputWindow gesetzt werden.
			SfxChildWindow* pChild = pThisFrame->GetChildWindow(FID_INPUTLINE_STATUS);
			if (pChild)
			{
				ScInputWindow* pWin = (ScInputWindow*)pChild->GetWindow();
				if (pWin && pWin->IsVisible())
				{

					ScInputHandler* pOldHdl=pWin->GetInputHandler();

					TypeId aScType = TYPE(ScTabViewShell);

					SfxViewShell* pSh = SfxViewShell::GetFirst( &aScType );
					while ( pSh!=NULL && pOldHdl!=NULL)
					{
						if (((ScTabViewShell*)pSh)->GetInputHandler() == pOldHdl)
						{
							pOldHdl->ResetDelayTimer();
							break;
						}
						pSh = SfxViewShell::GetNext( *pSh, &aScType );
					}

					pWin->SetInputHandler( pInputHandler );
				}
			}
		}

		UpdateInputHandler( TRUE );

		if ( bFirstActivate )
		{
			SFX_APP()->Broadcast( SfxSimpleHint( SC_HINT_NAVIGATOR_UPDATEALL ) );
			bFirstActivate = FALSE;

			if ( aPendingUserData.hasElements() )
			{
				//	#89897# read user data from print preview now, after ctor
				DoReadUserDataSequence( aPendingUserData );
				aPendingUserData.realloc( 0 );
			}

            // #116278# ReadExtOptions (view settings from Excel import) must also be done
            // after the ctor, because of the potential calls to Window::Show.
            // Even after the fix for #104887# (Window::Show no longer notifies the access
            // bridge, it's done in ImplSetReallyVisible), there are problems if Window::Show
            // is called during the ViewShell ctor and reschedules asynchronous calls
            // (for example from the FmFormShell ctor).
            ScExtDocOptions* pExtOpt = GetViewData()->GetDocument()->GetExtDocOptions();
            if ( pExtOpt && pExtOpt->IsChanged() )
            {
                GetViewData()->ReadExtOptions(*pExtOpt);        // Excel view settings
                SetTabNo( GetViewData()->GetTabNo(), TRUE );
                pExtOpt->SetChanged( false );
            }
		}

		pScActiveViewShell = this;

		ScInputHandler* pHdl = pScMod->GetInputHdl(this);
		if (pHdl)
		{
			pHdl->SetRefScale( GetViewData()->GetZoomX(), GetViewData()->GetZoomY() );
		}

		//	Aenderungs-Dialog aktualisieren

		if ( pThisFrame->HasChildWindow(FID_CHG_ACCEPT) )
		{
			SfxChildWindow* pChild = pThisFrame->GetChildWindow(FID_CHG_ACCEPT);
			if (pChild)
			{
				((ScAcceptChgDlgWrapper*)pChild)->ReInitDlg();
			}
		}

		if(pScMod->IsRefDialogOpen())
		{
			USHORT nModRefDlgId=pScMod->GetCurRefDlgId();
			SfxChildWindow* pChildWnd = pThisFrame->GetChildWindow( nModRefDlgId );
			if ( pChildWnd )
			{
				IAnyRefDialog* pRefDlg = dynamic_cast<IAnyRefDialog*>(pChildWnd->GetWindow());
				pRefDlg->ViewShellChanged(this);
			}
		}
	}

	//	don't call CheckSelectionTransfer here - activating a view should not change the
	//	primary selection (may be happening just because the mouse was moved over the window)

	//	Wenn Referenzeingabe-Tip-Hilfe hier wieder angezeigt werden soll (ShowRefTip),
	//	muss sie beim Verschieben der View angepasst werden (gibt sonst Probleme unter OS/2
	//	beim Umschalten zwischen Dokumenten)
}

void __EXPORT ScTabViewShell::Deactivate(BOOL bMDI)
{
	HideTip();

	ScDocument*	pDoc=GetViewData()->GetDocument();

	ScChangeTrack* pChanges=pDoc->GetChangeTrack();

	if(pChanges!=NULL)
	{
		Link aLink;
		pChanges->SetModifiedLink(aLink);
	}

	SfxViewShell::Deactivate(bMDI);

	ScInputHandler* pHdl = SC_MOD()->GetInputHdl(this);

	if( bMDI )
	{
		//	#85421# during shell deactivation, shells must not be switched, or the loop
		//	through the shell stack (in SfxDispatcher::DoDeactivate_Impl) will not work
		BOOL bOldDontSwitch = bDontSwitch;
		bDontSwitch = TRUE;

		DeActivateOlk( GetViewData() );
		ActivateView( FALSE, FALSE );

        if ( GetViewFrame()->GetFrame()->IsInPlace() ) // inplace
			GetViewData()->GetDocShell()->UpdateOle(GetViewData(),TRUE);

		if ( pHdl )
			pHdl->NotifyChange( NULL, TRUE ); // Timer-verzoegert wg. Dokumentwechsel

		if (pScActiveViewShell == this)
			pScActiveViewShell = NULL;

		bDontSwitch = bOldDontSwitch;
	}
	else
	{
		HideNoteMarker();			// Notiz-Anzeige

		if ( pHdl )
			pHdl->HideTip();		// Formel-AutoEingabe-Tip abschalten
	}
}

void ScTabViewShell::SetActive()
{
	// Die Sfx-View moechte sich gerne selbst aktivieren, weil dabei noch
	// magische Dinge geschehen (z.B. stuerzt sonst evtl. der Gestalter ab)
	ActiveGrabFocus();

#if 0
	SfxViewFrame* pFrame = GetViewFrame();
	if ( pFrame->ISA(SfxTopViewFrame) )
		pFrame->GetFrame()->Appear();

	SFX_APP()->SetViewFrame( pFrame );			// immer erst Appear, dann SetViewFrame (#29290#)
#endif
}

USHORT __EXPORT ScTabViewShell::PrepareClose(BOOL bUI, BOOL bForBrowsing)
{
    // Call EnterHandler even in formula mode here,
    // so a formula change in an embedded object isn't lost
    // (ScDocShell::PrepareClose isn't called then).
    ScInputHandler* pHdl = SC_MOD()->GetInputHdl( this );
    if ( pHdl && pHdl->IsInputMode() )
        pHdl->EnterHandler();

    // #110797# draw text edit mode must be closed
    FuPoor* pPoor = GetDrawFuncPtr();
    if ( pPoor && ( IsDrawTextShell() || pPoor->GetSlotID() == SID_DRAW_NOTEEDIT ) )
    {
        // "clean" end of text edit, including note handling, subshells and draw func switching,
        // as in FuDraw and ScTabView::DrawDeselectAll
        GetViewData()->GetDispatcher().Execute( pPoor->GetSlotID(), SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
    }
    ScDrawView* pDrView = GetScDrawView();
    if ( pDrView )
    {
        // force end of text edit, to be safe
        // #128314# ScEndTextEdit must always be used, to ensure correct UndoManager
        pDrView->ScEndTextEdit();
    }

	if ( pFormShell )
	{
		USHORT nRet = pFormShell->PrepareClose(bUI, bForBrowsing);
		if (nRet!=TRUE)
			return nRet;
	}
	return SfxViewShell::PrepareClose(bUI,bForBrowsing);
}

//------------------------------------------------------------------

Size __EXPORT ScTabViewShell::GetOptimalSizePixel() const
{
	Size aOptSize;

	SCTAB				nCurTab		= GetViewData()->GetTabNo();
	ScDocument*			pDoc		= GetViewData()->GetDocument();
	ScStyleSheetPool*	pStylePool  = pDoc->GetStyleSheetPool();
	SfxStyleSheetBase*	pStyleSheet = pStylePool->Find(
										pDoc->GetPageStyle( nCurTab ),
										SFX_STYLE_FAMILY_PAGE );

	DBG_ASSERT( pStyleSheet, "PageStyle not found :-/" );

	if ( pStyleSheet )
	{
		const SfxItemSet&  rSet 	 = pStyleSheet->GetItemSet();
		const SvxSizeItem& rItem	 = (const SvxSizeItem&)rSet.Get( ATTR_PAGE_SIZE );
		const Size&		   rPageSize = rItem.GetSize();

		aOptSize.Width()  = (long) (rPageSize.Width()  * GetViewData()->GetPPTX());
		aOptSize.Height() = (long) (rPageSize.Height() * GetViewData()->GetPPTY());
	}

	return aOptSize;
}

//------------------------------------------------------------------

//	Zoom fuer In-Place berechnen
//	aus Verhaeltnis von VisArea und Fenstergroesse des GridWin

void ScTabViewShell::UpdateOleZoom()
{
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	if ( pDocSh->GetCreateMode() == SFX_CREATE_MODE_EMBEDDED )
	{
        //TODO/LATER: is there a difference between the two GetVisArea methods?
        Size aObjSize = ((const SfxObjectShell*)pDocSh)->GetVisArea().GetSize();
		if ( aObjSize.Width() > 0 && aObjSize.Height() > 0 )
		{
			Window* pWin = GetActiveWin();
			Size aWinHMM = pWin->PixelToLogic( pWin->GetOutputSizePixel(), MAP_100TH_MM );
			SetZoomFactor( Fraction( aWinHMM.Width(),aObjSize.Width() ),
							Fraction( aWinHMM.Height(),aObjSize.Height() ) );
		}
	}
}

void __EXPORT ScTabViewShell::AdjustPosSizePixel( const Point &rPos, const Size &rSize )
{
	OuterResizePixel( rPos, rSize );
}

void __EXPORT ScTabViewShell::InnerResizePixel( const Point &rOfs, const Size &rSize )
{
	Size aNewSize( rSize );
    if ( GetViewFrame()->GetFrame()->IsInPlace() )
	{
		SvBorder aBorder;
	   	GetBorderSize( aBorder, rSize );
		SetBorderPixel( aBorder );

		Size aObjSize = GetObjectShell()->GetVisArea().GetSize();

      	Size aSize( rSize );
        aSize.Width() -= (aBorder.Left() + aBorder.Right());
        aSize.Height() -= (aBorder.Top() + aBorder.Bottom());

		if ( aObjSize.Width() > 0 && aObjSize.Height() > 0 )
    	{
        	Size aLogicSize = GetWindow()->PixelToLogic( aSize, MAP_100TH_MM );
        	SfxViewShell::SetZoomFactor( Fraction( aLogicSize.Width(),aObjSize.Width() ),
                        	Fraction( aLogicSize.Height(),aObjSize.Height() ) );
    	}

        Point aPos( rOfs );
        aPos.X() += aBorder.Left();
        aPos.Y() += aBorder.Top();
        GetWindow()->SetPosSizePixel( aPos, aSize );
	}
	else
    {
        SvBorder aBorder;
        GetBorderSize( aBorder, rSize );
        SetBorderPixel( aBorder );
        aNewSize.Width()  += aBorder.Left() + aBorder.Right();
        aNewSize.Height() += aBorder.Top() + aBorder.Bottom();
    }

	DoResize( rOfs, aNewSize, TRUE );					// rSize = Groesse von gridwin

	UpdateOleZoom();									//	Zoom fuer In-Place berechnen

//	GetViewData()->GetDocShell()->UpdateOle( GetViewData() );
	GetViewData()->GetDocShell()->SetDocumentModified();
}

void __EXPORT ScTabViewShell::OuterResizePixel( const Point &rOfs, const Size &rSize )
{
	SvBorder aBorder;
	GetBorderSize( aBorder, rSize );
	SetBorderPixel( aBorder );

	DoResize( rOfs, rSize );					// Position und Groesse von tabview wie uebergeben

	// ForceMove als Ersatz fuer den Sfx-Move-Mechanismus
	// (aWinPos muss aktuell gehalten werden, damit ForceMove beim Ole-Deaktivieren klappt)

	ForceMove();
}

void __EXPORT ScTabViewShell::SetZoomFactor( const Fraction &rZoomX, const Fraction &rZoomY )
{
	//	fuer OLE...

	Fraction aFrac20( 1,5 );
	Fraction aFrac400( 4,1 );

	Fraction aNewX( rZoomX );
	if ( aNewX < aFrac20 )
		aNewX = aFrac20;
	if ( aNewX > aFrac400 )
		aNewX = aFrac400;
	Fraction aNewY( rZoomY );
	if ( aNewY < aFrac20 )
		aNewY = aFrac20;
	if ( aNewY > aFrac400 )
		aNewY = aFrac400;

	GetViewData()->UpdateScreenZoom( aNewX, aNewY );
    SetZoom( aNewX, aNewY, TRUE );

	PaintGrid();
	PaintTop();
	PaintLeft();

	SfxViewShell::SetZoomFactor( rZoomX, rZoomY );
}

void __EXPORT ScTabViewShell::QueryObjAreaPixel( Rectangle& rRect ) const
{
	//	auf ganze Zellen anpassen (in 1/100 mm)

	Size aPixelSize = rRect.GetSize();
	Window* pWin = ((ScTabViewShell*)this)->GetActiveWin();
	Size aLogicSize = pWin->PixelToLogic( aPixelSize );

	const ScViewData* pViewData = GetViewData();
	ScDocument* pDoc = pViewData->GetDocument();
	ScSplitPos ePos = pViewData->GetActivePart();
	SCCOL nCol = pViewData->GetPosX(WhichH(ePos));
	SCROW nRow = pViewData->GetPosY(WhichV(ePos));
	SCTAB nTab = pViewData->GetTabNo();
    BOOL bNegativePage = pDoc->IsNegativePage( nTab );

	Rectangle aLogicRect = pDoc->GetMMRect( nCol, nRow, nCol, nRow, nTab );
	if ( bNegativePage )
	{
	    // use right edge of aLogicRect, and aLogicSize
	    aLogicRect.Left() = aLogicRect.Right() - aLogicSize.Width() + 1;    // Right() is set below
	}
	aLogicRect.SetSize( aLogicSize );

	pDoc->SnapVisArea( aLogicRect );

	rRect.SetSize( pWin->LogicToPixel( aLogicRect.GetSize() ) );

#if 0
	//	auf ganze Zellen anpassen (in Pixeln)

	ScViewData* pViewData = ((ScTabViewShell*)this)->GetViewData();
	Size aSize = rRect.GetSize();

	ScSplitPos ePos = pViewData->GetActivePart();
	Window* pWin = ((ScTabViewShell*)this)->GetActiveWin();

	Point aTest( aSize.Width(), aSize.Height() );
	SCsCOL nPosX;
	SCsROW nPosY;
	pViewData->GetPosFromPixel( aTest.X(), aTest.Y(), ePos, nPosX, nPosY );
	BOOL bLeft;
	BOOL bTop;
	pViewData->GetMouseQuadrant( aTest, ePos, nPosX, nPosY, bLeft, bTop );
	if (!bLeft)
		++nPosX;
	if (!bTop)
		++nPosY;
	aTest = pViewData->GetScrPos( (SCCOL)nPosX, (SCROW)nPosY, ePos, TRUE );

	rRect.SetSize(Size(aTest.X(),aTest.Y()));
#endif
}

//------------------------------------------------------------------

void __EXPORT ScTabViewShell::Move()
{
	Point aNewPos = GetViewFrame()->GetWindow().OutputToScreenPixel(Point());

	if (aNewPos != aWinPos)
	{
		StopMarking();
		aWinPos = aNewPos;
	}
}

//------------------------------------------------------------------

void __EXPORT ScTabViewShell::ShowCursor(FASTBOOL /* bOn */)
{
/*!!!	ShowCursor wird nicht paarweise wie im gridwin gerufen.
		Der CursorLockCount am Gridwin muss hier direkt auf 0 gesetzt werden

	if (bOn)
		ShowAllCursors();
	else
		HideAllCursors();
*/
}

//------------------------------------------------------------------

void __EXPORT ScTabViewShell::WriteUserData(String& rData, BOOL /* bBrowse */)
{
	GetViewData()->WriteUserData(rData);
}

void ScTabViewShell::WriteUserDataSequence (uno::Sequence < beans::PropertyValue >& rSettings, sal_Bool /* bBrowse */ )
{
	GetViewData()->WriteUserDataSequence (rSettings);
}

void __EXPORT ScTabViewShell::ReadUserData(const String& rData, BOOL /* bBrowse */)
{
	if ( !GetViewData()->GetDocShell()->IsPreview() )
		DoReadUserData( rData );
}

void ScTabViewShell::ReadUserDataSequence (const uno::Sequence < beans::PropertyValue >& rSettings, sal_Bool /* bBrowse */ )
{
    if ( !GetViewData()->GetDocShell()->IsPreview() )
        DoReadUserDataSequence( rSettings );
}

void ScTabViewShell::DoReadUserDataSequence( const uno::Sequence < beans::PropertyValue >& rSettings )
{
	Window* pOldWin = GetActiveWin();
	BOOL bFocus = pOldWin && pOldWin->HasFocus();

	GetViewData()->ReadUserDataSequence(rSettings);
	SetTabNo( GetViewData()->GetTabNo(), TRUE );

	if ( GetViewData()->IsPagebreakMode() )
		SetCurSubShell( GetCurObjectSelectionType(), TRUE );

	Window* pNewWin = GetActiveWin();
	if (pNewWin && pNewWin != pOldWin)
	{
		SetWindow( pNewWin );		//! ist diese ViewShell immer aktiv???
		if (bFocus)
			pNewWin->GrabFocus();
		WindowChanged();			// Drawing-Layer (z.B. #56771#)
	}

	if (GetViewData()->GetHSplitMode() == SC_SPLIT_FIX ||
		GetViewData()->GetVSplitMode() == SC_SPLIT_FIX)
	{
		InvalidateSplit();
	}

	ZoomChanged();

	TestHintWindow();

	//!	if ViewData has more tables than document, remove tables in ViewData
}

// DoReadUserData is also called from ctor when switching from print preview

void ScTabViewShell::DoReadUserData( const String& rData )
{
	Window* pOldWin = GetActiveWin();
	BOOL bFocus = pOldWin && pOldWin->HasFocus();

	GetViewData()->ReadUserData(rData);
	SetTabNo( GetViewData()->GetTabNo(), TRUE );

	if ( GetViewData()->IsPagebreakMode() )
		SetCurSubShell( GetCurObjectSelectionType(), TRUE );

	Window* pNewWin = GetActiveWin();
	if (pNewWin && pNewWin != pOldWin)
	{
		SetWindow( pNewWin );		//! ist diese ViewShell immer aktiv???
		if (bFocus)
			pNewWin->GrabFocus();
		WindowChanged();			// Drawing-Layer (z.B. #56771#)
	}

	if (GetViewData()->GetHSplitMode() == SC_SPLIT_FIX ||
		GetViewData()->GetVSplitMode() == SC_SPLIT_FIX)
	{
		InvalidateSplit();
	}

	ZoomChanged();

	TestHintWindow();

	//!	if ViewData has more tables than document, remove tables in ViewData
}

//------------------------------------------------------------------

//UNUSED2008-05  void ScTabViewShell::ExecuteShowNIY( SfxRequest& /* rReq */ )
//UNUSED2008-05  {
//UNUSED2008-05      ErrorMessage(STR_BOX_YNI);
//UNUSED2008-05  }
//UNUSED2008-05
//UNUSED2008-05  //------------------------------------------------------------------
//UNUSED2008-05
//UNUSED2008-05  void ScTabViewShell::StateDisabled( SfxItemSet& rSet )
//UNUSED2008-05  {
//UNUSED2008-05      SfxWhichIter aIter( rSet );
//UNUSED2008-05      USHORT       nWhich = aIter.FirstWhich();
//UNUSED2008-05
//UNUSED2008-05      while ( nWhich )
//UNUSED2008-05      {
//UNUSED2008-05          rSet.DisableItem( nWhich );
//UNUSED2008-05          nWhich = aIter.NextWhich();
//UNUSED2008-05      }
//UNUSED2008-05  }

void ScTabViewShell::SetDrawShellOrSub()
{
	bActiveDrawSh = TRUE;

	if(bActiveDrawFormSh)
	{
		SetCurSubShell(OST_DrawForm);
	}
	else if(bActiveGraphicSh)
	{
		SetCurSubShell(OST_Graphic);
	}
	else if(bActiveMediaSh)
	{
		SetCurSubShell(OST_Media);
	}
	else if(bActiveChartSh)
	{
		SetCurSubShell(OST_Chart);
	}
	else if(bActiveOleObjectSh)
	{
		SetCurSubShell(OST_OleObject);
	}
	else
	{
		SetCurSubShell(OST_Drawing, true /* force: different toolbars are
                                            visible concerning shape type
                                            and shape state */);
	}
}

void ScTabViewShell::SetDrawShell( BOOL bActive )
{
	if(bActive)
	{
		SetCurSubShell(OST_Drawing, true /* force: different toolbars are
                                            visible concerning shape type
                                            and shape state */);
	}
	else
	{
		if(bActiveDrawFormSh || bActiveDrawSh ||
            bActiveGraphicSh || bActiveMediaSh || bActiveOleObjectSh||
			bActiveChartSh || bActiveDrawTextSh)
		{
			SetCurSubShell(OST_Cell);
		}
		bActiveDrawFormSh=FALSE;
		bActiveGraphicSh=FALSE;
        bActiveMediaSh=FALSE;
		bActiveOleObjectSh=FALSE;
		bActiveChartSh=FALSE;
	}

	BOOL bWasDraw = bActiveDrawSh || bActiveDrawTextSh;

	bActiveDrawSh = bActive;
	bActiveDrawTextSh = FALSE;

	if ( !bActive )
	{
		ResetDrawDragMode();		//	Mirror / Rotate aus

		if (bWasDraw && (GetViewData()->GetHSplitMode() == SC_SPLIT_FIX ||
						 GetViewData()->GetVSplitMode() == SC_SPLIT_FIX))
		{
			//	Aktiven Teil an Cursor anpassen, etc.
			MoveCursorAbs( GetViewData()->GetCurX(), GetViewData()->GetCurY(),
							SC_FOLLOW_NONE, FALSE, FALSE, TRUE );
		}
	}
}

void ScTabViewShell::SetDrawTextShell( BOOL bActive )
{
	bActiveDrawTextSh = bActive;
	if ( bActive )
	{
		bActiveDrawFormSh=FALSE;
		bActiveGraphicSh=FALSE;
        bActiveMediaSh=FALSE;
		bActiveOleObjectSh=FALSE;
		bActiveChartSh=FALSE;
		bActiveDrawSh = FALSE;
		SetCurSubShell(OST_DrawText);
	}
	else
		SetCurSubShell(OST_Cell);

}

void ScTabViewShell::SetPivotShell( BOOL bActive )
{
	bActivePivotSh = bActive;

	//	#68771# #76198# SetPivotShell is called from CursorPosChanged every time
	//	-> don't change anything except switching between cell and pivot shell

	if ( eCurOST == OST_Pivot || eCurOST == OST_Cell )
	{
		if ( bActive )
		{
			bActiveDrawTextSh = bActiveDrawSh = FALSE;
			bActiveDrawFormSh=FALSE;
			bActiveGraphicSh=FALSE;
            bActiveMediaSh=FALSE;
			bActiveOleObjectSh=FALSE;
			bActiveChartSh=FALSE;
			SetCurSubShell(OST_Pivot);
		}
		else
			SetCurSubShell(OST_Cell);
	}
}

void ScTabViewShell::SetAuditShell( BOOL bActive )
{
	bActiveAuditingSh = bActive;
	if ( bActive )
	{
		bActiveDrawTextSh = bActiveDrawSh = FALSE;
		bActiveDrawFormSh=FALSE;
		bActiveGraphicSh=FALSE;
        bActiveMediaSh=FALSE;
		bActiveOleObjectSh=FALSE;
		bActiveChartSh=FALSE;
		SetCurSubShell(OST_Auditing);
	}
	else
		SetCurSubShell(OST_Cell);
}

void ScTabViewShell::SetDrawFormShell( BOOL bActive )
{
	bActiveDrawFormSh = bActive;

	if(bActiveDrawFormSh)
		SetCurSubShell(OST_DrawForm);
}
void ScTabViewShell::SetChartShell( BOOL bActive )
{
	bActiveChartSh = bActive;

	if(bActiveChartSh)
		SetCurSubShell(OST_Chart);
}

void ScTabViewShell::SetGraphicShell( BOOL bActive )
{
	bActiveGraphicSh = bActive;

	if(bActiveGraphicSh)
		SetCurSubShell(OST_Graphic);
}

void ScTabViewShell::SetMediaShell( BOOL bActive )
{
	bActiveMediaSh = bActive;

	if(bActiveMediaSh)
		SetCurSubShell(OST_Media);
}

void ScTabViewShell::SetOleObjectShell( BOOL bActive )
{
	bActiveOleObjectSh = bActive;

	if(bActiveOleObjectSh)
		SetCurSubShell(OST_OleObject);
	else
		SetCurSubShell(OST_Cell);
}

void ScTabViewShell::SetEditShell(EditView* pView, BOOL bActive )
{
	if(bActive)
	{
		if (pEditShell)
			pEditShell->SetEditView( pView );
		else
			pEditShell = new ScEditShell( pView, GetViewData() );

		SetCurSubShell(OST_Editing);
	}
	else if(bActiveEditSh)
	{
		SetCurSubShell(OST_Cell);
	}
	bActiveEditSh = bActive;
}

void ScTabViewShell::SetCurSubShell(ObjectSelectionType	eOST, BOOL bForce)
{
	ScViewData* pViewData	= GetViewData();
	ScDocShell* pDocSh		= pViewData->GetDocShell();

	if(bDontSwitch) return;

	if(!pCellShell) //Wird eh immer gebraucht.
	{
		pCellShell = new ScCellShell( GetViewData() );
		pCellShell->SetRepeatTarget( &aTarget );
	}

	BOOL bPgBrk=pViewData->IsPagebreakMode();

	if(bPgBrk && !pPageBreakShell)
	{
		pPageBreakShell = new ScPageBreakShell( this );
		pPageBreakShell->SetRepeatTarget( &aTarget );
	}


	if ( eOST!=eCurOST || bForce )
	{
        BOOL bCellBrush = FALSE;    // "format paint brush" allowed for cells
        BOOL bDrawBrush = FALSE;    // "format paint brush" allowed for drawing objects

		if(eCurOST!=OST_NONE) RemoveSubShell();

		if (pFormShell && !bFormShellAtTop)
		    AddSubShell(*pFormShell);               // add below own subshells

		switch(eOST)
		{
			case	OST_Cell:
					{
						AddSubShell(*pCellShell);
						if(bPgBrk) AddSubShell(*pPageBreakShell);
                        bCellBrush = TRUE;
					}
					break;
			case	OST_Editing:
					{
						AddSubShell(*pCellShell);
						if(bPgBrk) AddSubShell(*pPageBreakShell);

						if(pEditShell)
						{
							AddSubShell(*pEditShell);
						}
					}
					break;
			case	OST_DrawText:
					{
						if ( !pDrawTextShell )
						{
							pDocSh->MakeDrawLayer();
							pDrawTextShell = new ScDrawTextObjectBar( GetViewData() );
						}
						AddSubShell(*pDrawTextShell);
					}
					break;
			case	OST_Drawing:
					{
                        if (svx::checkForSelectedCustomShapes(
                                GetScDrawView(), true /* bOnlyExtruded */ )) {
                            if (pExtrusionBarShell == 0)
                                pExtrusionBarShell = new svx::ExtrusionBar(this);
                            AddSubShell( *pExtrusionBarShell );
                        }
                        sal_uInt32 nCheckStatus = 0;
                        if (svx::checkForSelectedFontWork(
                                GetScDrawView(), nCheckStatus )) {
                            if (pFontworkBarShell == 0)
                                pFontworkBarShell = new svx::FontworkBar(this);
                            AddSubShell( *pFontworkBarShell );
                        }

						if ( !pDrawShell )
						{
							pDocSh->MakeDrawLayer();
							pDrawShell = new ScDrawShell( GetViewData() );
							pDrawShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pDrawShell);
						bDrawBrush = TRUE;
					}
					break;

			case	OST_DrawForm:
					{
						if ( !pDrawFormShell )
						{
							pDocSh->MakeDrawLayer();
							pDrawFormShell = new ScDrawFormShell( GetViewData() );
							pDrawFormShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pDrawFormShell);
						bDrawBrush = TRUE;
					}
					break;

			case	OST_Chart:
					{
						if ( !pChartShell )
						{
							pDocSh->MakeDrawLayer();
							pChartShell = new ScChartShell( GetViewData() );
							pChartShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pChartShell);
						bDrawBrush = TRUE;
					}
					break;

			case	OST_OleObject:
					{
						if ( !pOleObjectShell )
						{
							pDocSh->MakeDrawLayer();
							pOleObjectShell = new ScOleObjectShell( GetViewData() );
							pOleObjectShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pOleObjectShell);
						bDrawBrush = TRUE;
					}
					break;

			case	OST_Graphic:
					{
						if ( !pGraphicShell)
						{
							pDocSh->MakeDrawLayer();
							pGraphicShell = new ScGraphicShell( GetViewData() );
							pGraphicShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pGraphicShell);
						bDrawBrush = TRUE;
					}
					break;

			case	OST_Media:
					{
						if ( !pMediaShell)
						{
							pDocSh->MakeDrawLayer();
							pMediaShell = new ScMediaShell( GetViewData() );
							pMediaShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pMediaShell);
					}
					break;

			case	OST_Pivot:
					{
						AddSubShell(*pCellShell);
						if(bPgBrk) AddSubShell(*pPageBreakShell);

						if ( !pPivotShell )
						{
							pPivotShell = new ScPivotShell( this );
							pPivotShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pPivotShell);
                        bCellBrush = TRUE;
					}
					break;
			case	OST_Auditing:
					{
						AddSubShell(*pCellShell);
						if(bPgBrk) AddSubShell(*pPageBreakShell);

						if ( !pAuditingShell )
						{
							pDocSh->MakeDrawLayer();	// die Wartezeit lieber jetzt als beim Klick

							pAuditingShell = new ScAuditingShell( GetViewData() );
							pAuditingShell->SetRepeatTarget( &aTarget );
						}
						AddSubShell(*pAuditingShell);
                        bCellBrush = TRUE;
					}
					break;
			default:
					DBG_ERROR("Falsche Shell angefordert");
					break;
		}

		if (pFormShell && bFormShellAtTop)
		    AddSubShell(*pFormShell);               // add on top of own subshells

		eCurOST=eOST;

        // abort "format paint brush" when switching to an incompatible shell
        if ( ( GetBrushDocument() && !bCellBrush ) || ( GetDrawBrushSet() && !bDrawBrush ) )
            ResetBrushDocument();
	}
}

void ScTabViewShell::SetFormShellAtTop( BOOL bSet )
{
    if ( pFormShell && !bSet )
        pFormShell->ForgetActiveControl();      // let the FormShell know it no longer has the focus

    if ( bFormShellAtTop != bSet )
    {
        bFormShellAtTop = bSet;
        SetCurSubShell( GetCurObjectSelectionType(), TRUE );
    }
}

IMPL_LINK( ScTabViewShell, FormControlActivated, FmFormShell*, EMPTYARG )
{
    // a form control got the focus, so the form shell has to be on top
    SetFormShellAtTop( TRUE );
    return 0;
}

ObjectSelectionType ScTabViewShell::GetCurObjectSelectionType()
{
	return eCurOST;
}

//	GetMySubShell / SetMySubShell: altes Verhalten simulieren,
//	dass es nur eine SubShell gibt (nur innerhalb der 5 eignenen SubShells)

SfxShell* ScTabViewShell::GetMySubShell() const
{
	//	GetSubShell() war frueher const, und GetSubShell(USHORT) sollte es auch sein...

	USHORT nPos = 0;
	SfxShell* pSub = ((ScTabViewShell*)this)->GetSubShell(nPos);
	while (pSub)
	{
		if ( pSub == pDrawShell  || pSub == pDrawTextShell || pSub == pEditShell ||
			 pSub == pPivotShell || pSub == pAuditingShell || pSub == pDrawFormShell ||
			 pSub == pCellShell	 || pSub == pOleObjectShell|| pSub == pChartShell ||
			 pSub == pGraphicShell || pSub == pMediaShell || pSub == pPageBreakShell)
			return pSub;	// gefunden

		pSub = ((ScTabViewShell*)this)->GetSubShell(++nPos);
	}
	return NULL;		// keine von meinen dabei
}

//UNUSED2008-05  void ScTabViewShell::SetMySubShell( SfxShell* pShell )
//UNUSED2008-05  {
//UNUSED2008-05      SfxShell* pOld = GetMySubShell();
//UNUSED2008-05      if ( pOld != pShell )
//UNUSED2008-05      {
//UNUSED2008-05          if (pOld)
//UNUSED2008-05              RemoveSubShell(pOld);   // alte SubShell entfernen
//UNUSED2008-05          if (pShell)
//UNUSED2008-05              AddSubShell(*pShell);   // neue setzen
//UNUSED2008-05      }
//UNUSED2008-05  }

BOOL ScTabViewShell::IsDrawTextShell() const
{
	return ( pDrawTextShell && ( GetMySubShell() == pDrawTextShell ) );
}

BOOL ScTabViewShell::IsAuditShell() const
{
	return ( pAuditingShell && ( GetMySubShell() == pAuditingShell ) );
}

void ScTabViewShell::SetDrawTextUndo( SfxUndoManager* pNewUndoMgr )
{
	// Default: Undo-Manager der DocShell
	if (!pNewUndoMgr)
		pNewUndoMgr = GetViewData()->GetDocShell()->GetUndoManager();

	if (pDrawTextShell)
    {
		pDrawTextShell->SetUndoManager(pNewUndoMgr);
        ScDocShell* pDocSh = GetViewData()->GetDocShell();
        if ( pNewUndoMgr == pDocSh->GetUndoManager() &&
             !pDocSh->GetDocument()->IsUndoEnabled() )
        {
            pNewUndoMgr->SetMaxUndoActionCount( 0 );
        }
    }
	else
	{
		DBG_ERROR("SetDrawTextUndo ohne DrawTextShell");
	}
}

//------------------------------------------------------------------

ScTabViewShell* ScTabViewShell::GetActiveViewShell()
{
	return PTR_CAST(ScTabViewShell,Current());
}

//------------------------------------------------------------------

SfxPrinter* __EXPORT ScTabViewShell::GetPrinter( BOOL bCreate )
{
	//	Drucker ist immer da (wird fuer die FontListe schon beim Starten angelegt)
	return GetViewData()->GetDocShell()->GetPrinter(bCreate);
}

USHORT __EXPORT ScTabViewShell::SetPrinter( SfxPrinter *pNewPrinter, USHORT nDiffFlags, bool )
{
	return GetViewData()->GetDocShell()->SetPrinter( pNewPrinter, nDiffFlags );
}

PrintDialog* __EXPORT ScTabViewShell::CreatePrintDialog( Window *pParent )
{
	ScDocShell*	pDocShell	= GetViewData()->GetDocShell();
	ScDocument*	pDoc		= pDocShell->GetDocument();

	pDoc->SetPrintOptions();				// Optionen aus OFA am Printer setzen
	SfxPrinter* pPrinter = GetPrinter();

	String			aStrRange;
    PrintDialog*    pDlg        = new PrintDialog( pParent, true );
	SCTAB			nTabCount	= pDoc->GetTableCount();
	long			nDocPageMax = 0;

    pDlg->EnableSheetRange( true, PRINTSHEETS_ALL );
    pDlg->EnableSheetRange( true, PRINTSHEETS_SELECTED_SHEETS );
    pDlg->EnableSheetRange( true, PRINTSHEETS_SELECTED_CELLS );
    bool bAllTabs = SC_MOD()->GetPrintOptions().GetAllSheets();
    pDlg->CheckSheetRange( bAllTabs ? PRINTSHEETS_ALL : PRINTSHEETS_SELECTED_SHEETS );

    // update all pending row heights with a single progress bar,
    // instead of a separate progress for each sheet from ScPrintFunc
    pDocShell->UpdatePendingRowHeights( MAXTAB, true );

	for ( SCTAB i=0; i<nTabCount; i++ )
	{
		ScPrintFunc aPrintFunc( pDocShell, pPrinter, i );
		nDocPageMax += aPrintFunc.GetTotalPages();
	}

	if ( nDocPageMax > 0 )
	{
		aStrRange = '1';
		if ( nDocPageMax > 1 )
		{
			aStrRange += '-';
			aStrRange += String::CreateFromInt32( nDocPageMax );
		}
	}

    pDlg->SetRangeText	( aStrRange );
	pDlg->EnableRange	( PRINTDIALOG_ALL );
	pDlg->EnableRange	( PRINTDIALOG_RANGE );
	pDlg->SetFirstPage	( 1 );
	pDlg->SetMinPage	( 1 );
	pDlg->SetLastPage	( (USHORT)nDocPageMax );
	pDlg->SetMaxPage	( (USHORT)nDocPageMax );
	pDlg->EnableCollate	();

	return pDlg;
}

SfxTabPage* ScTabViewShell::CreatePrintOptionsPage( Window *pParent, const SfxItemSet &rOptions )
{
	ScAbstractDialogFactory* pFact = ScAbstractDialogFactory::Create();
	DBG_ASSERT(pFact, "ScAbstractFactory create fail!");//CHINA001
	//CHINA001 return ScTpPrintOptions::Create( pParent, rOptions );
	::CreateTabPage ScTpPrintOptionsCreate = 	pFact->GetTabPageCreatorFunc( RID_SCPAGE_PRINT );
	if ( ScTpPrintOptionsCreate )
		return  (*ScTpPrintOptionsCreate)( pParent, rOptions);
	return 0;
}

void __EXPORT ScTabViewShell::PreparePrint( PrintDialog* pPrintDialog )
{
	ScDocShell* pDocShell = GetViewData()->GetDocShell();

	SfxViewShell::PreparePrint( pPrintDialog );
	pDocShell->PreparePrint( pPrintDialog, &GetViewData()->GetMarkData() );
}

ErrCode ScTabViewShell::DoPrint( SfxPrinter *pPrinter,
                                 PrintDialog *pPrintDialog, BOOL bSilent, BOOL bIsAPI )
{
	//	#72527# if SID_PRINTDOCDIRECT is executed and there's a selection,
	//	ask if only the selection should be printed

	const ScMarkData& rMarkData = GetViewData()->GetMarkData();
    if ( !pPrintDialog && !bSilent && !bIsAPI && ( rMarkData.IsMarked() || rMarkData.IsMultiMarked() ) )
	{
		SvxPrtQryBox aQuery( GetDialogParent() );
		short nBtn = aQuery.Execute();

		if ( nBtn == RET_CANCEL )
			return ERRCODE_IO_ABORT;

		if ( nBtn == RET_OK )
			bPrintSelected = TRUE;
	}

    ErrCode nRet = ERRCODE_IO_ABORT;

    ScDocShell* pDocShell = GetViewData()->GetDocShell();
    if ( pDocShell->CheckPrint( pPrintDialog, &GetViewData()->GetMarkData(), bPrintSelected, bIsAPI ) )
    {
        // get the list of affected sheets before SfxViewShell::Print
        bool bAllTabs = ( pPrintDialog ? ( pPrintDialog->GetCheckedSheetRange() == PRINTSHEETS_ALL ) : SC_MOD()->GetPrintOptions().GetAllSheets() );

        uno::Sequence<sal_Int32> aSheets;
        SCTAB nTabCount = pDocShell->GetDocument()->GetTableCount();
        USHORT nPrinted = 0;
        for ( SCTAB nTab=0; nTab<nTabCount; nTab++ )
            if ( bAllTabs || rMarkData.GetTableSelect(nTab) )
            {
                aSheets.realloc( nPrinted + 1 );
                aSheets[nPrinted] = nTab;
                ++nPrinted;
            }

        uno::Sequence < beans::PropertyValue > aProps(1);
        aProps[0].Name=::rtl::OUString::createFromAscii("PrintSheets");
        aProps[0].Value <<= aSheets;
        SetAdditionalPrintOptions( aProps );

        // SfxViewShell::DoPrint calls Print (after StartJob etc.)
		nRet = SfxViewShell::DoPrint( pPrinter, pPrintDialog, bSilent, bIsAPI );
    }

	bPrintSelected = FALSE;

	return nRet;
}

USHORT __EXPORT ScTabViewShell::Print( SfxProgress& rProgress, BOOL bIsAPI,
									   PrintDialog* pPrintDialog )
{
	ScDocShell* pDocShell = GetViewData()->GetDocShell();
	pDocShell->GetDocument()->SetPrintOptions();	// Optionen aus OFA am Printer setzen

    SfxViewShell::Print( rProgress, bIsAPI, pPrintDialog );
	pDocShell->Print( rProgress, pPrintDialog, &GetViewData()->GetMarkData(),
                        GetDialogParent(), bPrintSelected, bIsAPI );
	return 0;
}

void ScTabViewShell::StopEditShell()
{
	if ( pEditShell != NULL && !bDontSwitch )
		SetEditShell(NULL, FALSE );
}

//------------------------------------------------------------------

// close handler to ensure function of dialog:

IMPL_LINK( ScTabViewShell, SimpleRefClose, String*, EMPTYARG )
{
    SfxInPlaceClient* pClient = GetIPClient();
    if ( pClient && pClient->IsObjectInPlaceActive() )
    {
        // If range selection was started with an active embedded object,
        // switch back to original sheet (while the dialog is still open).

        SetTabNo( GetViewData()->GetRefTabNo() );
    }

	ScSimpleRefDlgWrapper::SetAutoReOpen( TRUE );
	return 0;
}

// handlers to call UNO listeners:

ScTabViewObj* lcl_GetViewObj( ScTabViewShell& rShell )
{
	ScTabViewObj* pRet = NULL;
	SfxViewFrame* pViewFrame = rShell.GetViewFrame();
	if (pViewFrame)
	{
		SfxFrame* pFrame = pViewFrame->GetFrame();
		if (pFrame)
		{
			uno::Reference<frame::XController> xController = pFrame->GetController();
			if (xController.is())
				pRet = ScTabViewObj::getImplementation( xController );
		}
	}
	return pRet;
}

IMPL_LINK( ScTabViewShell, SimpleRefDone, String*, pResult )
{
	ScTabViewObj* pImpObj = lcl_GetViewObj( *this );
	if ( pImpObj && pResult )
		pImpObj->RangeSelDone( *pResult );
	return 0;
}

IMPL_LINK( ScTabViewShell, SimpleRefAborted, String*, pResult )
{
	ScTabViewObj* pImpObj = lcl_GetViewObj( *this );
	if ( pImpObj && pResult )
		pImpObj->RangeSelAborted( *pResult );
	return 0;
}

IMPL_LINK( ScTabViewShell, SimpleRefChange, String*, pResult )
{
	ScTabViewObj* pImpObj = lcl_GetViewObj( *this );
	if ( pImpObj && pResult )
		pImpObj->RangeSelChanged( *pResult );
	return 0;
}

void ScTabViewShell::StartSimpleRefDialog(
            const String& rTitle, const String& rInitVal,
            BOOL bCloseOnButtonUp, BOOL bSingleCell, BOOL bMultiSelection )
{
	SfxViewFrame* pViewFrm = GetViewFrame();

    if ( GetActiveViewShell() != this )
    {
        // #i18833# / #i34499# The API method can be called for a view that's not active.
        // Then the view has to be activated first, the same way as in Execute for SID_CURRENTDOC.
        // Can't use GrabFocus here, because it needs to take effect immediately.

    	if ( pViewFrm->ISA(SfxTopViewFrame) )
    		pViewFrm->GetFrame()->Appear();
    }

	USHORT nId = ScSimpleRefDlgWrapper::GetChildWindowId();

	SC_MOD()->SetRefDialog( nId, TRUE, pViewFrm );

	ScSimpleRefDlgWrapper* pWnd = (ScSimpleRefDlgWrapper*)pViewFrm->GetChildWindow( nId );
	if (pWnd)
	{
		pWnd->SetCloseHdl( LINK( this, ScTabViewShell, SimpleRefClose ) );
		pWnd->SetUnoLinks( LINK( this, ScTabViewShell, SimpleRefDone ),
						   LINK( this, ScTabViewShell, SimpleRefAborted ),
						   LINK( this, ScTabViewShell, SimpleRefChange ) );
		pWnd->SetRefString( rInitVal );
        pWnd->SetFlags( bCloseOnButtonUp, bSingleCell, bMultiSelection );
		pWnd->SetAutoReOpen( FALSE );
		Window* pWin = pWnd->GetWindow();
		pWin->SetText( rTitle );
		pWnd->StartRefInput();
	}
}

void ScTabViewShell::StopSimpleRefDialog()
{
	SfxViewFrame* pViewFrm = GetViewFrame();
	USHORT nId = ScSimpleRefDlgWrapper::GetChildWindowId();

	ScSimpleRefDlgWrapper* pWnd = (ScSimpleRefDlgWrapper*)pViewFrm->GetChildWindow( nId );
	if (pWnd)
	{
		Window* pWin = pWnd->GetWindow();
		if (pWin && pWin->IsSystemWindow())
			((SystemWindow*)pWin)->Close();		// calls abort handler
	}
}

//------------------------------------------------------------------

BOOL ScTabViewShell::TabKeyInput(const KeyEvent& rKEvt)
{
	ScModule* pScMod = SC_MOD();

	SfxViewFrame* pThisFrame = GetViewFrame();
	if ( pThisFrame->GetChildWindow( SID_OPENDLG_FUNCTION ) )
		return FALSE;

	KeyCode aCode   = rKEvt.GetKeyCode();
	BOOL bShift     = aCode.IsShift();
	BOOL bControl   = aCode.IsMod1();
	BOOL bAlt	    = aCode.IsMod2();
	USHORT nCode	= aCode.GetCode();
	BOOL bUsed	    = FALSE;
	BOOL bInPlace   = pScMod->IsEditMode(); 	// Editengine bekommt alles
	BOOL bAnyEdit   = pScMod->IsInputMode();	// nur Zeichen & Backspace
	BOOL bDraw		= IsDrawTextEdit();

	HideNoteMarker();	// Notiz-Anzeige

    // don't do extra HideCursor/ShowCursor calls if EnterHandler will switch to a different sheet
    BOOL bOnRefSheet = ( GetViewData()->GetRefTabNo() == GetViewData()->GetTabNo() );
    BOOL bHideCursor = ( ( nCode == KEY_RETURN && bInPlace ) || nCode == KEY_TAB ) && bOnRefSheet;

	if (bHideCursor)
		HideAllCursors();

	ScDocument* pDoc = GetViewData()->GetDocument();
	if ( pDoc )
		pDoc->KeyInput( rKEvt );	// TimerDelays etc.

	if( bInPlace )
	{
		bUsed = pScMod->InputKeyEvent( rKEvt );			// Eingabe
		if( !bUsed )
            bUsed = sal::static_int_cast<BOOL>(SfxViewShell::KeyInput( rKEvt ));    // accelerators
	}
	else if( bAnyEdit )
	{
		BOOL bIsType = FALSE;
		USHORT nModi = aCode.GetModifier();
		USHORT nGroup = aCode.GetGroup();

		if ( nGroup == KEYGROUP_NUM || nGroup == KEYGROUP_ALPHA || nGroup == 0 )
			if ( !bControl && !bAlt )
				bIsType = TRUE;

		if ( nGroup == KEYGROUP_MISC )
			switch ( nCode )
			{
				case KEY_RETURN:
					bIsType = bControl && !bAlt;		// Control, Shift-Control-Return
					if ( !bIsType && nModi == 0 )
					{
						//	Will der InputHandler auch ein einfaches Return?

						ScInputHandler* pHdl = pScMod->GetInputHdl(this);
						bIsType = pHdl && pHdl->TakesReturn();
					}
					break;
				case KEY_SPACE:
					bIsType = !bControl && !bAlt;		// ohne Modifier oder Shift-Space
					break;
				case KEY_ESCAPE:
				case KEY_BACKSPACE:
					bIsType = (nModi == 0);	// nur ohne Modifier
					break;
				default:
					bIsType = TRUE;
			}

		if( bIsType )
			bUsed = pScMod->InputKeyEvent( rKEvt );		// Eingabe

		if( !bUsed )
            bUsed = sal::static_int_cast<BOOL>(SfxViewShell::KeyInput( rKEvt ));    // accelerators

		if ( !bUsed && !bIsType && nCode != KEY_RETURN )	// Eingabe nochmal hinterher
			bUsed = pScMod->InputKeyEvent( rKEvt );
	}
	else
	{
		//	#51889# Spezialfall: Copy/Cut bei Mehrfachselektion -> Fehlermeldung
		//	(Slot ist disabled, SfxViewShell::KeyInput wuerde also kommentarlos verschluckt)
		KeyFuncType eFunc = aCode.GetFunction();
		if ( eFunc == KEYFUNC_CUT )
		{
			ScRange aDummy;
			ScMarkType eMarkType = GetViewData()->GetSimpleArea( aDummy );
			if ( eMarkType != SC_MARK_SIMPLE &&
                    !(eFunc == KEYFUNC_COPY && eMarkType == SC_MARK_SIMPLE_FILTERED) )
			{
				ErrorMessage(STR_NOMULTISELECT);
				bUsed = TRUE;
			}
		}
		if (!bUsed)
            bUsed = sal::static_int_cast<BOOL>(SfxViewShell::KeyInput( rKEvt ));    // accelerators

		//	#74696# during inplace editing, some slots are handled by the
		//	container app and are executed during Window::KeyInput.
		//	-> don't pass keys to input handler that would be used there
		//	but should call slots instead.
        BOOL bParent = ( GetViewFrame()->GetFrame()->IsInPlace() && eFunc != KEYFUNC_DONTKNOW );

		if( !bUsed && !bDraw && nCode != KEY_RETURN && !bParent )
			bUsed = pScMod->InputKeyEvent( rKEvt, TRUE );		// Eingabe
	}

	if (!bInPlace && !bUsed && !bDraw)
	{
		switch (nCode)
		{
			case KEY_RETURN:
				{
					BOOL bNormal = !bControl && !bAlt;
					if ( !bAnyEdit && bNormal )
					{
						//	je nach Optionen mit Enter in den Edit-Modus schalten

						const ScInputOptions& rOpt = pScMod->GetInputOptions();
						if ( rOpt.GetEnterEdit() )
						{
							pScMod->SetInputMode( SC_INPUT_TABLE );
							bUsed = TRUE;
						}
					}

					BOOL bEditReturn = bControl && !bShift; 		// An Edit-Engine weiter
					if ( !bUsed && !bEditReturn )
					{
					    if ( bOnRefSheet )
    						HideAllCursors();

						BYTE nMode = SC_ENTER_NORMAL;
						if ( bShift && bControl )
							nMode = SC_ENTER_MATRIX;
						else if ( bAlt )
							nMode = SC_ENTER_BLOCK;
						pScMod->InputEnterHandler(nMode);

						if (nMode == SC_ENTER_NORMAL)
						{
							if( bShift )
								GetViewData()->GetDispatcher().Execute( SID_CURSORENTERUP,
											SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
							else
								GetViewData()->GetDispatcher().Execute( SID_CURSORENTERDOWN,
											SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
						}
						else
							UpdateInputHandler(TRUE);

					    if ( bOnRefSheet )
    						ShowAllCursors();

						//	hier kein UpdateInputHandler, weil bei Referenzeingabe auf ein
						//	anderes Dokument diese ViewShell nicht die ist, auf der eingegeben
						//	wird!

						bUsed = TRUE;
					}
				}
				break;
		}
	}

	//	Alt-Cursortasten hart codiert, weil Alt nicht konfigurierbar ist

	if ( !bUsed && bAlt && !bControl )
	{
		USHORT nSlotId = 0;
		switch (nCode)
		{
			case KEY_UP:
				ModifyCellSize( DIR_TOP, bShift );
				bUsed = TRUE;
				break;
			case KEY_DOWN:
				ModifyCellSize( DIR_BOTTOM, bShift );
				bUsed = TRUE;
				break;
			case KEY_LEFT:
				ModifyCellSize( DIR_LEFT, bShift );
				bUsed = TRUE;
				break;
			case KEY_RIGHT:
				ModifyCellSize( DIR_RIGHT, bShift );
				bUsed = TRUE;
				break;
			case KEY_PAGEUP:
				nSlotId = bShift ? SID_CURSORPAGELEFT_SEL : SID_CURSORPAGELEFT_;
				break;
			case KEY_PAGEDOWN:
				nSlotId = bShift ? SID_CURSORPAGERIGHT_SEL : SID_CURSORPAGERIGHT_;
				break;
		}
		if ( nSlotId )
		{
			GetViewData()->GetDispatcher().Execute( nSlotId, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
			bUsed = TRUE;
		}
	}

	if (bHideCursor)
		ShowAllCursors();

	return bUsed;
}

BOOL ScTabViewShell::SfxKeyInput(const KeyEvent& rKeyEvent)
{
    return sal::static_int_cast<BOOL>(SfxViewShell::KeyInput( rKeyEvent ));
}

FASTBOOL __EXPORT ScTabViewShell::KeyInput( const KeyEvent &rKeyEvent )
{
//	return SfxViewShell::KeyInput( rKeyEvent );
	return TabKeyInput( rKeyEvent );
}

//------------------------------------------------------------------

//	SfxViewShell( pViewFrame, SFX_VIEW_MAXIMIZE_FIRST | SFX_VIEW_DISABLE_ACCELS ),

#define __INIT_ScTabViewShell \
	eCurOST(OST_NONE),			\
	nDrawSfxId(0),				\
	nCtrlSfxId(USHRT_MAX),		\
	nFormSfxId(USHRT_MAX),		\
	pDrawShell(NULL),			\
	pDrawTextShell(NULL),		\
	pEditShell(NULL),			\
	pPivotShell(NULL),			\
	pAuditingShell(NULL),		\
	pDrawFormShell(NULL),		\
	pCellShell(NULL),			\
	pOleObjectShell(NULL),		\
	pChartShell(NULL),			\
	pGraphicShell(NULL),		\
	pMediaShell(NULL),			\
	pPageBreakShell(NULL),		\
	pExtrusionBarShell(NULL),	\
	pFontworkBarShell(NULL),	\
	pFormShell(NULL),			\
	pInputHandler(NULL),		\
	pCurFrameLine(NULL),		\
	aTarget( this ),			\
	pDialogDPObject(NULL),		\
    pNavSettings(NULL),         \
	bActiveDrawSh(FALSE),		\
	bActiveDrawTextSh(FALSE),	\
	bActivePivotSh(FALSE),		\
	bActiveAuditingSh(FALSE),	\
	bActiveDrawFormSh(FALSE),	\
	bActiveOleObjectSh(FALSE),	\
	bActiveChartSh(FALSE),		\
	bActiveGraphicSh(FALSE),	\
	bActiveMediaSh(FALSE),		\
	bActiveEditSh(FALSE),       \
	bFormShellAtTop(FALSE),     \
	bDontSwitch(FALSE),			\
	bInFormatDialog(FALSE),		\
	bPrintSelected(FALSE),		\
	bReadOnly(FALSE),			\
	pScSbxObject(NULL),			\
	/*bChartDlgIsEdit(FALSE),*/		\
	bChartAreaValid(FALSE),		\
    nCurRefDlgId(0),            \
	pAccessibilityBroadcaster(NULL)


//------------------------------------------------------------------

void ScTabViewShell::Construct( BYTE nForceDesignMode )
{
	SfxApplication* pSfxApp  = SFX_APP();
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScDocument* pDoc = pDocSh->GetDocument();

	bReadOnly = pDocSh->IsReadOnly();

	SetName( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("View")) );	// fuer SBX
	Color aColBlack( COL_BLACK );
//	SetPool( &pSfxApp->GetPool() );
	SetPool( &SC_MOD()->GetPool() );
	SetWindow( GetActiveWin() );

	pCurFrameLine	= new SvxBorderLine( &aColBlack, 20, 0, 0 );
	pPivotSource	= new ScArea;
    StartListening(*GetViewData()->GetDocShell(),TRUE);
    StartListening(*GetViewFrame(),TRUE);
    StartListening(*pSfxApp,TRUE);              // #i62045# #i62046# application is needed for Calc's own hints

	SfxViewFrame* pFirst = SfxViewFrame::GetFirst(pDocSh);
	BOOL bFirstView = !pFirst
		  || (pFirst == GetViewFrame() && !SfxViewFrame::GetNext(*pFirst,pDocSh));

	if ( pDocSh->GetCreateMode() == SFX_CREATE_MODE_EMBEDDED )
	{
        //TODO/LATER: is there a difference between the two GetVisArea methods?
        Rectangle aVisArea = ((const SfxObjectShell*)pDocSh)->GetVisArea();

		SCTAB nVisTab = pDoc->GetVisibleTab();
		if (!pDoc->HasTable(nVisTab))
		{
			nVisTab = 0;
			pDoc->SetVisibleTab(nVisTab);
		}
		SetTabNo( nVisTab );
        BOOL bNegativePage = pDoc->IsNegativePage( nVisTab );
        // show the right cells
        GetViewData()->SetScreenPos( bNegativePage ? aVisArea.TopRight() : aVisArea.TopLeft() );

        if ( GetViewFrame()->GetFrame()->IsInPlace() )                         // inplace
		{
			pDocSh->SetInplace( TRUE );				// schon so initialisiert
			if (pDoc->IsEmbedded())
				pDoc->ResetEmbedded();				// keine blaue Markierung
		}
		else if ( bFirstView )
		{
			pDocSh->SetInplace( FALSE );
            GetViewData()->RefreshZoom();           // recalculate PPT
			if (!pDoc->IsEmbedded())
				pDoc->SetEmbedded( aVisArea );					// VisArea markieren
		}
	}

	// ViewInputHandler
	//	#48721# jeder Task hat neuerdings sein eigenes InputWindow,
	//	darum muesste eigentlich entweder jeder Task seinen InputHandler bekommen,
	//	oder das InputWindow muesste sich beim App-InputHandler anmelden, wenn der
	//	Task aktiv wird, oder das InputWindow muesste sich den InputHandler selbst
	//	anlegen (dann immer ueber das InputWindow suchen, und nur wenn das nicht da
	//	ist, den InputHandler von der App nehmen).
	//	Als Sofortloesung bekommt erstmal jede View ihren Inputhandler, das gibt
	//	nur noch Probleme, wenn zwei Views in einem Task-Fenster sind.

	pInputHandler = new ScInputHandler;

	// Alte Version:
	//	if ( !GetViewFrame()->ISA(SfxTopViewFrame) )		// OLE oder Plug-In
	//		pInputHandler = new ScInputHandler;

			//	FormShell vor MakeDrawView anlegen, damit die DrawView auf jeden Fall
			//	an der FormShell angemeldet werden kann
			//	Gepusht wird die FormShell im ersten Activate
	pFormShell = new FmFormShell(this);
	pFormShell->SetControlActivationHandler( LINK( this, ScTabViewShell, FormControlActivated ) );

			//	DrawView darf nicht im TabView - ctor angelegt werden,
			//	wenn die ViewShell noch nicht kostruiert ist...
	if (pDoc->GetDrawLayer())
		MakeDrawView( nForceDesignMode );
	ViewOptionsHasChanged(FALSE);	// legt auch evtl. DrawView an

    SfxUndoManager* pMgr = pDocSh->GetUndoManager();
    SetUndoManager( pMgr );
    pFormShell->SetUndoManager( pMgr );
    if ( !pDoc->IsUndoEnabled() )
    {
        pMgr->SetMaxUndoActionCount( 0 );
    }
	SetRepeatTarget( &aTarget );
	pFormShell->SetRepeatTarget( &aTarget );
	SetHelpId( HID_SCSHELL_TABVWSH );

	if ( bFirstView )	// first view?
	{
		pDoc->SetDocVisible( TRUE );		// used when creating new sheets
		if ( pDocSh->IsEmpty() )
		{
			// set first sheet's RTL flag (following will already be initialized because of SetDocVisible)
			pDoc->SetLayoutRTL( 0, ScGlobal::IsSystemRTL() );

			// append additional sheets (not for OLE object)
			if ( pDocSh->GetCreateMode() != SFX_CREATE_MODE_EMBEDDED )
			{
				SCTAB nInitTabCount = 3;							//!	konfigurierbar !!!
				for (SCTAB i=1; i<nInitTabCount; i++)
					pDoc->MakeTable(i,false);
			}

            pDocSh->SetEmpty( FALSE );          // #i6232# make sure this is done only once
		}

		// ReadExtOptions is now in Activate

		//	Link-Update nicht verschachteln
		if ( pDocSh->GetCreateMode() != SFX_CREATE_MODE_INTERNAL &&
             pDocSh->IsUpdateEnabled() )  // #105575#; update only in the first creation of the ViewShell
		{
            // Check if there are any external data.
            bool bLink = pDoc->GetExternalRefManager()->hasExternalData();
            if (!bLink)
            {
                // #i100042# sheet links can still exist independently from external formula references
                SCTAB nTabCount = pDoc->GetTableCount();
                for (SCTAB i=0; i<nTabCount && !bLink; i++)
                    if (pDoc->IsLinked(i))
                        bLink = true;
            }
			if (!bLink)
				if (pDoc->HasDdeLinks() || pDoc->HasAreaLinks())
					bLink = TRUE;
			if (bLink)
			{
				if ( !pFirst )
					pFirst = GetViewFrame();

				if(SC_MOD()->GetCurRefDlgId()==0)
				{
						pFirst->GetDispatcher()->Execute( SID_UPDATETABLINKS,
											SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD );
				}
			}

			BOOL bReImport = FALSE;								// importierte Daten aktualisieren
			ScDBCollection* pDBColl = pDoc->GetDBCollection();
			if ( pDBColl )
			{
				USHORT nCount = pDBColl->GetCount();
				for (USHORT i=0; i<nCount && !bReImport; i++)
				{
					ScDBData* pData = (*pDBColl)[i];
					if ( pData->IsStripData() &&
							pData->HasImportParam() && !pData->HasImportSelection() )
						bReImport = TRUE;
				}
			}
			if (bReImport)
			{
				if ( !pFirst )
					pFirst = GetViewFrame();
				if(SC_MOD()->GetCurRefDlgId()==0)
				{
					pFirst->GetDispatcher()->Execute( SID_REIMPORT_AFTER_LOAD,
											SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD );
				}
			}
		}
	}

	UpdateAutoFillMark();

	// ScDispatchProviderInterceptor registers itself in ctor
	xDisProvInterceptor = new ScDispatchProviderInterceptor( this );

	bFirstActivate = TRUE; // NavigatorUpdate aufschieben bis Activate()

    // #105575#; update only in the first creation of the ViewShell
    pDocSh->SetUpdateEnabled(FALSE);

	if ( GetViewFrame()->GetFrame()->IsInPlace() )
		UpdateHeaderWidth(); // The implace activation requires headers to be calculated

	SvBorder aBorder;
    GetBorderSize( aBorder, Size() );
	SetBorderPixel( aBorder );
}

//------------------------------------------------------------------

//UNUSED2008-05  ScTabViewShell::ScTabViewShell( SfxViewFrame* pViewFrame,
//UNUSED2008-05                                  const ScTabViewShell& rWin ) :
//UNUSED2008-05  SfxViewShell( pViewFrame, SFX_VIEW_MAXIMIZE_FIRST | SFX_VIEW_CAN_PRINT | SFX_VIEW_HAS_PRINTOPTIONS ),
//UNUSED2008-05  ScDBFunc( &pViewFrame->GetWindow(), rWin, this ),
//UNUSED2008-05  __INIT_ScTabViewShell
//UNUSED2008-05  {
//UNUSED2008-05      RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "sc", "nn93723", "ScTabViewShell::ScTabViewShell" );
//UNUSED2008-05  
//UNUSED2008-05      Construct();
//UNUSED2008-05  
//UNUSED2008-05      UpdatePageBreakData();
//UNUSED2008-05  
//UNUSED2008-05      /*uno::Reference<frame::XFrame> xFrame = pViewFrame->GetFrame()->GetFrameInterface();
//UNUSED2008-05      if (xFrame.is())
//UNUSED2008-05          xFrame->setComponent( uno::Reference<awt::XWindow>(), new ScTabViewObj( this ) );*/
//UNUSED2008-05      // make Controller known to SFX
//UNUSED2008-05      new ScTabViewObj( this );
//UNUSED2008-05  
//UNUSED2008-05      SetCurSubShell(OST_Cell);
//UNUSED2008-05      SvBorder aBorder;
//UNUSED2008-05      GetBorderSize( aBorder, Size() );
//UNUSED2008-05      SetBorderPixel( aBorder );
//UNUSED2008-05  }

//------------------------------------------------------------------

ScTabViewShell::ScTabViewShell( SfxViewFrame* pViewFrame,
								SfxViewShell* pOldSh ) :
    SfxViewShell( pViewFrame, SFX_VIEW_MAXIMIZE_FIRST | SFX_VIEW_CAN_PRINT | SFX_VIEW_HAS_PRINTOPTIONS ),
    ScDBFunc( &pViewFrame->GetWindow(), (ScDocShell&)*pViewFrame->GetObjectShell(), this ),
    __INIT_ScTabViewShell
{
	RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "sc", "nn93723", "ScTabViewShell::ScTabViewShell" );

	const ScAppOptions&	rAppOpt = SC_MOD()->GetAppOptions();

	//	if switching back from print preview,
	//	restore the view settings that were active when creating the preview
	//	#89897# ReadUserData must not happen from ctor, because the view's edit window
	//	has to be shown by the sfx. ReadUserData is deferred until the first Activate call.
	//	#106334# old DesignMode state from form layer must be restored, too

	BYTE nForceDesignMode = SC_FORCEMODE_NONE;
	if ( pOldSh && pOldSh->ISA( ScPreviewShell ) )
	{
		ScPreviewShell* pPreviewShell = ((ScPreviewShell*)pOldSh);
		aPendingUserData = pPreviewShell->GetSourceData();		// used in Activate
		nForceDesignMode = pPreviewShell->GetSourceDesignMode();
	}

	Construct( nForceDesignMode );

	if ( GetViewData()->GetDocShell()->IsPreview() )
	{
		//	preview for template dialog: always show whole page
        SetZoomType( SVX_ZOOM_WHOLEPAGE, TRUE );    // zoom value is recalculated at next Resize
	}
	else
	{
		Fraction aFract( rAppOpt.GetZoom(), 100 );
        SetZoom( aFract, aFract, TRUE );
        SetZoomType( rAppOpt.GetZoomType(), TRUE );
	}

    /*uno::Reference<frame::XFrame> xFrame = pViewFrame->GetFrame()->GetFrameInterface();
	if (xFrame.is())
        xFrame->setComponent( uno::Reference<awt::XWindow>(), new ScTabViewObj( this ) );*/
    // make Controller known to SFX
    new ScTabViewObj( this );

	SetCurSubShell(OST_Cell);
	SvBorder aBorder;
    GetBorderSize( aBorder, Size() );
	SetBorderPixel( aBorder );

	// #114409#
	MakeDrawLayer();
}

#undef __INIT_ScTabViewShell

//------------------------------------------------------------------

__EXPORT ScTabViewShell::~ScTabViewShell()
{
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	EndListening(*pDocSh);
	EndListening(*GetViewFrame());
    EndListening(*SFX_APP());           // #i62045# #i62046# needed now - SfxViewShell no longer does it

	SC_MOD()->ViewShellGone(this);

	RemoveSubShell();			// alle
	SetWindow(0);

	//	#54104# alles auf NULL, falls aus dem TabView-dtor noch darauf zugegriffen wird
	//!	(soll eigentlich nicht !??!?!)

	DELETEZ(pFontworkBarShell);
	DELETEZ(pExtrusionBarShell);
	DELETEZ(pCellShell);
	DELETEZ(pPageBreakShell);
	DELETEZ(pDrawShell);
	DELETEZ(pDrawFormShell);
	DELETEZ(pOleObjectShell);
	DELETEZ(pChartShell);
	DELETEZ(pGraphicShell);
	DELETEZ(pMediaShell);
	DELETEZ(pDrawTextShell);
	DELETEZ(pEditShell);
	DELETEZ(pPivotShell);
	DELETEZ(pAuditingShell);
	DELETEZ(pCurFrameLine);
	DELETEZ(pInputHandler);
	DELETEZ(pPivotSource);
	DELETEZ(pDialogDPObject);
    DELETEZ(pNavSettings);

	DELETEZ(pFormShell);
	DELETEZ(pAccessibilityBroadcaster);
}

//------------------------------------------------------------------

void ScTabViewShell::SetDialogDPObject( const ScDPObject* pObj )
{
	delete pDialogDPObject;
	if (pObj)
		pDialogDPObject = new ScDPObject( *pObj );
	else
		pDialogDPObject = NULL;
}

//------------------------------------------------------------------

void ScTabViewShell::FillFieldData( ScHeaderFieldData& rData )
{
	ScDocShell* pDocShell = GetViewData()->GetDocShell();
	ScDocument* pDoc = pDocShell->GetDocument();
	SCTAB nTab = GetViewData()->GetTabNo();
	pDoc->GetName( nTab, rData.aTabName );

	rData.aTitle		= pDocShell->GetTitle();
	const INetURLObject& rURLObj = pDocShell->GetMedium()->GetURLObject();
	rData.aLongDocName	= rURLObj.GetMainURL( INetURLObject::DECODE_UNAMBIGUOUS );
	if ( rData.aLongDocName.Len() )
		rData.aShortDocName = rURLObj.GetName( INetURLObject::DECODE_UNAMBIGUOUS );
	else
		rData.aShortDocName = rData.aLongDocName = rData.aTitle;
	rData.nPageNo		= 1;
	rData.nTotalPages	= 99;

	//	eNumType kennt der Dialog selber
}

//------------------------------------------------------------------

void ScTabViewShell::SetChartArea( const ScRangeListRef& rSource, const Rectangle& rDest )
{
	bChartAreaValid	= TRUE;
	aChartSource	= rSource;
	aChartPos		= rDest;
	nChartDestTab	= GetViewData()->GetTabNo();
}

//UNUSED2008-05  void ScTabViewShell::ResetChartArea()
//UNUSED2008-05  {
//UNUSED2008-05      bChartAreaValid = FALSE;
//UNUSED2008-05  }

BOOL ScTabViewShell::GetChartArea( ScRangeListRef& rSource, Rectangle& rDest, SCTAB& rTab ) const
{
	rSource	= aChartSource;
	rDest	= aChartPos;
	rTab	= nChartDestTab;
	return bChartAreaValid;
}

//UNUSED2008-05  BOOL ScTabViewShell::IsChartDlgEdit() const
//UNUSED2008-05  {
//UNUSED2008-05      return bChartDlgIsEdit;
//UNUSED2008-05  }
//UNUSED2008-05  
//UNUSED2008-05  const String& ScTabViewShell::GetEditChartName() const
//UNUSED2008-05  {
//UNUSED2008-05      return aEditChartName;
//UNUSED2008-05  }

ScNavigatorSettings* ScTabViewShell::GetNavigatorSettings()
{
    if( !pNavSettings )
        pNavSettings = new ScNavigatorSettings;
    return pNavSettings;
}


//------------------------------------------------------------------

void ScTabViewShell::ExecTbx( SfxRequest& rReq )
{
	const SfxItemSet* pReqArgs = rReq.GetArgs();
	USHORT nSlot = rReq.GetSlot();
	const SfxPoolItem* pItem = NULL;
	if ( pReqArgs )
		pReqArgs->GetItemState( nSlot, TRUE, &pItem );

	switch ( nSlot )
	{
		case SID_TBXCTL_INSERT:
			if ( pItem )
				nInsertCtrlState = ((const SfxUInt16Item*)pItem)->GetValue();
			break;
		case SID_TBXCTL_INSCELLS:
			if ( pItem )
				nInsCellsCtrlState = ((const SfxUInt16Item*)pItem)->GetValue();
			break;
		case SID_TBXCTL_INSOBJ:
			if ( pItem )
				nInsObjCtrlState = ((const SfxUInt16Item*)pItem)->GetValue();
			break;
		default:
			DBG_ERROR("Slot im Wald");
	}
	GetViewFrame()->GetBindings().Invalidate( nSlot );
}

void ScTabViewShell::GetTbxState( SfxItemSet& rSet )
{
	rSet.Put( SfxUInt16Item( SID_TBXCTL_INSERT,   nInsertCtrlState ) );
	rSet.Put( SfxUInt16Item( SID_TBXCTL_INSCELLS, nInsCellsCtrlState ) );

	//	ohne installiertes Chart darf Chart nicht Default sein...
	if ( nInsObjCtrlState == SID_DRAW_CHART && !SvtModuleOptions().IsChart() )
		nInsObjCtrlState = SID_INSERT_OBJECT;

	rSet.Put( SfxUInt16Item( SID_TBXCTL_INSOBJ,   nInsObjCtrlState ) );
}





