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

#include <algorithm>

#include "scitems.hxx"
#include <svx/eeitem.hxx>

#include <sfx2/app.hxx>
#include <svx/adjitem.hxx>
#include <svx/editview.hxx>
#include <svx/editstat.hxx>
#include <svx/frmdiritem.hxx>
#include <svx/lspcitem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/event.hxx>
#include <sfx2/imgmgr.hxx>
#include <stdlib.h>		// qsort
#include <svx/scriptspaceitem.hxx>
#include <svx/scripttypeitem.hxx>
#include <vcl/cursor.hxx>
#include <vcl/help.hxx>
#include <svtools/stritem.hxx>

#include "inputwin.hxx"
#include "scmod.hxx"
#include "uiitems.hxx"
#include "global.hxx"
#include "scresid.hxx"
#include "sc.hrc"
#include "globstr.hrc"
#include "editutil.hxx"
#include "inputhdl.hxx"
#include "tabvwsh.hxx"
#include "document.hxx"
#include "docsh.hxx"
#include "appoptio.hxx"
#include "rangenam.hxx"
#include <formula/compiler.hrc>
#include "dbcolect.hxx"
#include "rangeutl.hxx"
#include "docfunc.hxx"
#include "funcdesc.hxx"
#include <svx/fontitem.hxx>
#include <com/sun/star/accessibility/XAccessible.hpp>
#include "AccessibleEditObject.hxx"
#include "AccessibleText.hxx"

#define TEXT_STARTPOS		3
#define THESIZE				1000000	//!!! langt... :-)
#define TBX_WINDOW_HEIGHT 	22 // in Pixeln - fuer alle Systeme gleich?

enum ScNameInputType
{
    SC_NAME_INPUT_CELL,
    SC_NAME_INPUT_RANGE,
    SC_NAME_INPUT_NAMEDRANGE,
    SC_NAME_INPUT_DATABASE,
    SC_NAME_INPUT_ROW,
    SC_NAME_INPUT_SHEET,
    SC_NAME_INPUT_DEFINE,
    SC_NAME_INPUT_BAD_NAME,
    SC_NAME_INPUT_BAD_SELECTION
};


//==================================================================
//	class ScInputWindowWrapper
//==================================================================

SFX_IMPL_CHILDWINDOW(ScInputWindowWrapper,FID_INPUTLINE_STATUS)

ScInputWindowWrapper::ScInputWindowWrapper( Window*          pParentP,
											USHORT			 nId,
											SfxBindings*	 pBindings,
                                            SfxChildWinInfo* /* pInfo */ )
    :   SfxChildWindow( pParentP, nId )
{
    ScInputWindow* pWin=new ScInputWindow( pParentP, pBindings );
	pWindow = pWin;

	pWin->Show();

	pWin->SetSizePixel( pWin->CalcWindowSizePixel() );

	eChildAlignment = SFX_ALIGN_LOWESTTOP;
	pBindings->Invalidate( FID_TOGGLEINPUTLINE );
}

//	GetInfo fliegt wieder raus, wenn es ein SFX_IMPL_TOOLBOX gibt !!!!

SfxChildWinInfo __EXPORT ScInputWindowWrapper::GetInfo() const
{
	SfxChildWinInfo aInfo = SfxChildWindow::GetInfo();
	return aInfo;
}

//==================================================================

#define IMAGE(id) pImgMgr->SeekImage(id, bDark)

//==================================================================
//	class ScInputWindow
//==================================================================

ScInputWindow::ScInputWindow( Window* pParent, SfxBindings* pBind ) :
#ifdef OS2
// #37192# ohne WB_CLIPCHILDREN wg. os/2 Paintproblem
		ToolBox         ( pParent, WinBits(WB_BORDER|WB_3DLOOK) ),
#else
// mit WB_CLIPCHILDREN, sonst Flicker
		ToolBox         ( pParent, WinBits(WB_BORDER|WB_3DLOOK|WB_CLIPCHILDREN) ),
#endif
		aWndPos         ( this ),
		aTextWindow     ( this ),
		pInputHdl		( NULL ),
        pBindings       ( pBind ),
		aTextOk			( ScResId( SCSTR_QHELP_BTNOK ) ),		// nicht immer neu aus Resource
		aTextCancel		( ScResId( SCSTR_QHELP_BTNCANCEL ) ),
		aTextSum		( ScResId( SCSTR_QHELP_BTNSUM ) ),
		aTextEqual		( ScResId( SCSTR_QHELP_BTNEQUAL ) ),
		bIsOkCancelMode ( FALSE )
{
	ScModule*		 pScMod  = SC_MOD();
    SfxImageManager* pImgMgr = SfxImageManager::GetImageManager( pScMod );

    // #i73615# don't rely on SfxViewShell::Current while constructing the input line
    // (also for GetInputHdl below)
    ScTabViewShell* pViewSh = NULL;
    SfxDispatcher* pDisp = pBind->GetDispatcher();
    if ( pDisp )
    {
        SfxViewFrame* pViewFrm = pDisp->GetFrame();
        if ( pViewFrm )
            pViewSh = PTR_CAST( ScTabViewShell, pViewFrm->GetViewShell() );
    }
    DBG_ASSERT( pViewSh, "no view shell for input window" );

    BOOL bDark = GetSettings().GetStyleSettings().GetFaceColor().IsDark();

	// Positionsfenster, 3 Buttons, Eingabefenster
	InsertWindow    ( 1, &aWndPos, 0,								      0 );
	InsertSeparator ( 												   	  1 );
	InsertItem      ( SID_INPUT_FUNCTION, IMAGE( SID_INPUT_FUNCTION ), 0, 2 );
	InsertItem      ( SID_INPUT_SUM, 	  IMAGE( SID_INPUT_SUM ), 0,      3 );
	InsertItem      ( SID_INPUT_EQUAL,	  IMAGE( SID_INPUT_EQUAL ), 0,    4 );
	InsertSeparator ( 												      5 );
	InsertWindow    ( 7, &aTextWindow, 0,                                 6 );

	aWndPos	   .SetQuickHelpText( ScResId( SCSTR_QHELP_POSWND ) );
	aWndPos    .SetHelpId		( HID_INSWIN_POS );
	aTextWindow.SetQuickHelpText( ScResId( SCSTR_QHELP_INPUTWND ) );
	aTextWindow.SetHelpId		( HID_INSWIN_INPUT );

	//	kein SetHelpText, die Hilfetexte kommen aus der Hilfe

	SetItemText ( SID_INPUT_FUNCTION, ScResId( SCSTR_QHELP_BTNCALC ) );
	SetHelpId	( SID_INPUT_FUNCTION, HID_INSWIN_CALC );

	SetItemText ( SID_INPUT_SUM, aTextSum );
	SetHelpId	( SID_INPUT_SUM, HID_INSWIN_SUMME );

	SetItemText ( SID_INPUT_EQUAL, aTextEqual );
	SetHelpId	( SID_INPUT_EQUAL, HID_INSWIN_FUNC );

	SetHelpId( HID_SC_INPUTWIN );	// fuer die ganze Eingabezeile

	aWndPos		.Show();
	aTextWindow	.Show();

    pInputHdl = SC_MOD()->GetInputHdl( pViewSh, FALSE );    // use own handler even if ref-handler is set
	if (pInputHdl)
		pInputHdl->SetInputWindow( this );

	if ( pInputHdl && pInputHdl->GetFormString().Len() )
	{
		//	Umschalten waehrend der Funktionsautopilot aktiv ist
		//	-> Inhalt des Funktionsautopiloten wieder anzeigen
		//!	auch Selektion (am InputHdl gemerkt) wieder anzeigen

		aTextWindow.SetTextString( pInputHdl->GetFormString() );
	}
	else if ( pInputHdl && pInputHdl->IsInputMode() )
	{
		//	wenn waehrend des Editierens die Eingabezeile weg war
		//	(Editieren einer Formel, dann umschalten zu fremdem Dokument/Hilfe),
		//	wieder den gerade editierten Text aus dem InputHandler anzeigen

		aTextWindow.SetTextString( pInputHdl->GetEditString() );	// Text anzeigen
		if ( pInputHdl->IsTopMode() )
			pInputHdl->SetMode( SC_INPUT_TABLE );		// Focus kommt eh nach unten
	}
	else if ( pViewSh )
		pViewSh->UpdateInputHandler( TRUE ); // unbedingtes Update

	pImgMgr->RegisterToolBox( this );
}

__EXPORT ScInputWindow::~ScInputWindow()
{
    BOOL bDown = ( ScGlobal::pSysLocale == NULL );    // after Clear?

	//	if any view's input handler has a pointer to this input window, reset it
	//	(may be several ones, #74522#)
	//	member pInputHdl is not used here

	if ( !bDown )
	{
		TypeId aScType = TYPE(ScTabViewShell);
		SfxViewShell* pSh = SfxViewShell::GetFirst( &aScType );
		while ( pSh )
		{
			ScInputHandler* pHdl = ((ScTabViewShell*)pSh)->GetInputHandler();
			if ( pHdl && pHdl->GetInputWindow() == this )
            {
				pHdl->SetInputWindow( NULL );
                pHdl->StopInputWinEngine( FALSE );  // #125841# reset pTopView pointer
            }
			pSh = SfxViewShell::GetNext( *pSh, &aScType );
		}
	}

    SfxImageManager::GetImageManager( SC_MOD() )->ReleaseToolBox( this );
}

void ScInputWindow::SetInputHandler( ScInputHandler* pNew )
{
	//	wird im Activate der View gerufen...

    if ( pNew != pInputHdl )
	{
		//	Bei Reload (letzte Version) ist pInputHdl der Input-Handler der alten,
		//	geloeschten ViewShell, darum hier auf keinen Fall anfassen!

		pInputHdl = pNew;
		if (pInputHdl)
			pInputHdl->SetInputWindow( this );
	}
}

sal_Bool ScInputWindow::UseSubTotal(ScRangeList* pRangeList) const
{
    sal_Bool bSubTotal(sal_False);
    ScTabViewShell* pViewSh = PTR_CAST( ScTabViewShell, SfxViewShell::Current() );
    if ( pViewSh )
    {
        ScDocument* pDoc = pViewSh->GetViewData()->GetDocument();
        sal_Int32 nRangeCount (pRangeList->Count());
        sal_Int32 nRangeIndex (0);
        while (!bSubTotal && nRangeIndex < nRangeCount)
        {
            const ScRange* pRange = pRangeList->GetObject( nRangeIndex );
            if( pRange )
            {
                SCTAB nTabEnd(pRange->aEnd.Tab());
                SCTAB nTab(pRange->aStart.Tab());
                while (!bSubTotal && nTab <= nTabEnd)
                {
                    SCROW nRowEnd(pRange->aEnd.Row());
                    SCROW nRow(pRange->aStart.Row());
                    while (!bSubTotal && nRow <= nRowEnd)
                    {
                        if (pDoc->IsFiltered(nRow, nTab))
                            bSubTotal = sal_True;
                        else
                            ++nRow;
                    }
                    ++nTab;
                }
            }
            ++nRangeIndex;
        }

        ScDBCollection* pDBCollection = pDoc->GetDBCollection();
        sal_uInt16 nDBCount (pDBCollection->GetCount());
        sal_uInt16 nDBIndex (0);
        while (!bSubTotal && nDBIndex < nDBCount)
        {
            ScDBData* pDB = (*pDBCollection)[nDBIndex];
            if (pDB && pDB->HasAutoFilter())
            {
                nRangeIndex = 0;
                while (!bSubTotal && nRangeIndex < nRangeCount)
                {
                    const ScRange* pRange = pRangeList->GetObject( nRangeIndex );
                    if( pRange )
                    {
                        ScRange aDBArea;
                        pDB->GetArea(aDBArea);
                        if (aDBArea.Intersects(*pRange))
                            bSubTotal = sal_True;
                    }
                    ++nRangeIndex;
                }
            }
            ++nDBIndex;
        }
    }
    return bSubTotal;
}

void __EXPORT ScInputWindow::Select()
{
	ScModule* pScMod = SC_MOD();
	ToolBox::Select();

	switch ( GetCurItemId() )
	{
		case SID_INPUT_FUNCTION:
			{
				//!	new method at ScModule to query if function autopilot is open
				SfxViewFrame* pViewFrm = SfxViewFrame::Current();
				if ( pViewFrm && !pViewFrm->GetChildWindow( SID_OPENDLG_FUNCTION ) )
				{
					pViewFrm->GetDispatcher()->Execute( SID_OPENDLG_FUNCTION,
											  SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD );

					//	die Toolbox wird sowieso disabled, also braucht auch nicht umgeschaltet
					//	zu werden, egal ob's geklappt hat oder nicht
//					SetOkCancelMode();
				}
			}
			break;

		case SID_INPUT_CANCEL:
			pScMod->InputCancelHandler();
			SetSumAssignMode();
			break;

		case SID_INPUT_OK:
			pScMod->InputEnterHandler();
			SetSumAssignMode();
			aTextWindow.Invalidate();		// sonst bleibt Selektion stehen
			break;

		case SID_INPUT_SUM:
			{
				ScTabViewShell* pViewSh = PTR_CAST( ScTabViewShell, SfxViewShell::Current() );
				if ( pViewSh )
				{
					const ScMarkData& rMark = pViewSh->GetViewData()->GetMarkData();
					if ( rMark.IsMarked() || rMark.IsMultiMarked() )
					{
                        ScRangeList aMarkRangeList;
                        rMark.FillRangeListWithMarks( &aMarkRangeList, FALSE );
                        ScDocument* pDoc = pViewSh->GetViewData()->GetDocument();

                        // check if one of the marked ranges is empty
                        bool bEmpty = false;
                        const ULONG nCount = aMarkRangeList.Count();
                        for ( ULONG i = 0; i < nCount; ++i )
                        {
                            const ScRange aRange( *aMarkRangeList.GetObject( i ) );
                            if ( pDoc->IsBlockEmpty( aRange.aStart.Tab(),
                                    aRange.aStart.Col(), aRange.aStart.Row(),
                                    aRange.aEnd.Col(), aRange.aEnd.Row() ) )
                            {
                                bEmpty = true;
                                break;
                            }
                        }

                        if ( bEmpty )
                        {
                            ScRangeList aRangeList;
					        const BOOL bDataFound = pViewSh->GetAutoSumArea( aRangeList );
                            if ( bDataFound )
                            {
                                const sal_Bool bSubTotal( UseSubTotal( &aRangeList ) );
                                pViewSh->EnterAutoSum( aRangeList, bSubTotal );	// Block mit Summen fuellen
                            }
                        }
                        else
                        {
                            const sal_Bool bSubTotal( UseSubTotal( &aMarkRangeList ) );
                            for ( ULONG i = 0; i < nCount; ++i )
                            {
                                const ScRange aRange( *aMarkRangeList.GetObject( i ) );
                                const bool bSetCursor = ( i == nCount - 1 ? true : false );
                                const bool bContinue = ( i != 0  ? true : false );
                                if ( !pViewSh->AutoSum( aRange, bSubTotal, bSetCursor, bContinue ) )
                                {
                                    pViewSh->MarkRange( aRange, FALSE, FALSE );
                                    pViewSh->SetCursor( aRange.aEnd.Col(), aRange.aEnd.Row() );
                                    const ScRangeList aRangeList;
                                    const String aFormula = pViewSh->GetAutoSumFormula( aRangeList, bSubTotal );
                                    SetFuncString( aFormula );
                                    break;
                                }
                            }
                        }
					}
					else									// nur in Eingabezeile einfuegen
					{
                        ScRangeList aRangeList;
					    const BOOL bDataFound = pViewSh->GetAutoSumArea( aRangeList );
                        const sal_Bool bSubTotal( UseSubTotal( &aRangeList ) );
                        const String aFormula = pViewSh->GetAutoSumFormula( aRangeList, bSubTotal );
						SetFuncString( aFormula );

                        if ( bDataFound && pScMod->IsEditMode() )
						{
							ScInputHandler* pHdl = pScMod->GetInputHdl( pViewSh );
							if ( pHdl )
							{
								pHdl->InitRangeFinder( aFormula );

								//!	SetSelection am InputHandler ???
								//!	bSelIsRef setzen ???
								const xub_StrLen nOpen = aFormula.Search('(');
								const xub_StrLen nLen = aFormula.Len();
								if ( nOpen != STRING_NOTFOUND && nLen > nOpen )
								{
                                    sal_uInt8 nAdd(1);
                                    if (bSubTotal)
                                        nAdd = 3;
									ESelection aSel(0,nOpen+nAdd,0,nLen-1);
									EditView* pTableView = pHdl->GetTableView();
									if (pTableView)
										pTableView->SetSelection(aSel);
									EditView* pTopView = pHdl->GetTopView();
									if (pTopView)
										pTopView->SetSelection(aSel);
								}
							}
						}
					}
				}
			}
			break;

		case SID_INPUT_EQUAL:
		{
			aTextWindow.StartEditEngine();
			if ( pScMod->IsEditMode() )			// nicht, wenn z.B. geschuetzt
			{
				aTextWindow.GrabFocus();
				aTextWindow.SetTextString( '=' );

				EditView* pView = aTextWindow.GetEditView();
				if (pView)
				{
					pView->SetSelection( ESelection(0,1, 0,1) );
					pScMod->InputChanged(pView);
					SetOkCancelMode();
					pView->SetEditEngineUpdateMode(TRUE);
				}
			}
			break;
		}
	}
}

void __EXPORT ScInputWindow::Resize()
{
	ToolBox::Resize();

	long nWidth = GetSizePixel().Width();
	long nLeft  = aTextWindow.GetPosPixel().X();
	Size aSize  = aTextWindow.GetSizePixel();

	aSize.Width() = Max( ((long)(nWidth - nLeft - 5)), (long)0 );
	aTextWindow.SetSizePixel( aSize );
	aTextWindow.Invalidate();
}

void ScInputWindow::SetFuncString( const String& rString, BOOL bDoEdit )
{
	//!	new method at ScModule to query if function autopilot is open
	SfxViewFrame* pViewFrm = SfxViewFrame::Current();
	EnableButtons( pViewFrm && !pViewFrm->GetChildWindow( SID_OPENDLG_FUNCTION ) );
	aTextWindow.StartEditEngine();

	ScModule* pScMod = SC_MOD();
	if ( pScMod->IsEditMode() )
	{
		if ( bDoEdit )
			aTextWindow.GrabFocus();
		aTextWindow.SetTextString( rString );
		EditView* pView = aTextWindow.GetEditView();
		if (pView)
		{
			xub_StrLen nLen = rString.Len();

			if ( nLen > 0 )
			{
				nLen--;
				pView->SetSelection( ESelection( 0, nLen, 0, nLen ) );
			}

			pScMod->InputChanged(pView);
			if ( bDoEdit )
				SetOkCancelMode();			// nicht, wenn gleich hinterher Enter/Cancel

			pView->SetEditEngineUpdateMode(TRUE);
		}
	}
}

void ScInputWindow::SetPosString( const String& rStr )
{
	aWndPos.SetPos( rStr );
}

void ScInputWindow::SetTextString( const String& rString )
{
	if (rString.Len() <= 32767)
		aTextWindow.SetTextString(rString);
	else
	{
		String aNew = rString;
		aNew.Erase(32767);
		aTextWindow.SetTextString(aNew);
	}
}

void ScInputWindow::SetOkCancelMode()
{
	//!	new method at ScModule to query if function autopilot is open
	SfxViewFrame* pViewFrm = SfxViewFrame::Current();
	EnableButtons( pViewFrm && !pViewFrm->GetChildWindow( SID_OPENDLG_FUNCTION ) );

	ScModule* pScMod = SC_MOD();
    SfxImageManager* pImgMgr = SfxImageManager::GetImageManager( pScMod );
	if (!bIsOkCancelMode)
	{
        BOOL bDark = GetSettings().GetStyleSettings().GetFaceColor().IsDark();

		RemoveItem( 3 ); // SID_INPUT_SUM und SID_INPUT_EQUAL entfernen
		RemoveItem( 3 );
		InsertItem( SID_INPUT_CANCEL, IMAGE( SID_INPUT_CANCEL ), 0, 3 );
		InsertItem( SID_INPUT_OK,	  IMAGE( SID_INPUT_OK ),	 0, 4 );
		SetItemText	( SID_INPUT_CANCEL, aTextCancel );
		SetHelpId	( SID_INPUT_CANCEL, HID_INSWIN_CANCEL );
		SetItemText	( SID_INPUT_OK,		aTextOk );
		SetHelpId	( SID_INPUT_OK,		HID_INSWIN_OK );
		bIsOkCancelMode = TRUE;
	}
}

void ScInputWindow::SetSumAssignMode()
{
	//!	new method at ScModule to query if function autopilot is open
	SfxViewFrame* pViewFrm = SfxViewFrame::Current();
	EnableButtons( pViewFrm && !pViewFrm->GetChildWindow( SID_OPENDLG_FUNCTION ) );

	ScModule* pScMod = SC_MOD();
    SfxImageManager* pImgMgr = SfxImageManager::GetImageManager( pScMod );
	if (bIsOkCancelMode)
	{
        BOOL bDark = GetSettings().GetStyleSettings().GetFaceColor().IsDark();

		// SID_INPUT_CANCEL, und SID_INPUT_OK entfernen
		RemoveItem( 3 );
		RemoveItem( 3 );
		InsertItem( SID_INPUT_SUM, 	 IMAGE( SID_INPUT_SUM ), 	 0, 3 );
		InsertItem( SID_INPUT_EQUAL, IMAGE( SID_INPUT_EQUAL ),	 0, 4 );
		SetItemText	( SID_INPUT_SUM,   aTextSum );
		SetHelpId	( SID_INPUT_SUM,   HID_INSWIN_SUMME );
		SetItemText	( SID_INPUT_EQUAL, aTextEqual );
		SetHelpId	( SID_INPUT_EQUAL, HID_INSWIN_FUNC );
		bIsOkCancelMode = FALSE;

		SetFormulaMode(FALSE);		// kein editieren -> keine Formel
	}
}

void ScInputWindow::SetFormulaMode( BOOL bSet )
{
	aWndPos.SetFormulaMode(bSet);
	aTextWindow.SetFormulaMode(bSet);
}

void __EXPORT ScInputWindow::SetText( const String& rString )
{
	ToolBox::SetText(rString);
}

String __EXPORT ScInputWindow::GetText() const
{
	return ToolBox::GetText();
}


//UNUSED2008-05  EditView* ScInputWindow::ActivateEdit( const String&     rText,
//UNUSED2008-05                                         const ESelection& rSel )
//UNUSED2008-05  {
//UNUSED2008-05      if ( !aTextWindow.IsInputActive() )
//UNUSED2008-05      {
//UNUSED2008-05          aTextWindow.StartEditEngine();
//UNUSED2008-05          aTextWindow.GrabFocus();
//UNUSED2008-05          aTextWindow.SetTextString( rText );
//UNUSED2008-05          aTextWindow.GetEditView()->SetSelection( rSel );
//UNUSED2008-05      }
//UNUSED2008-05
//UNUSED2008-05      return aTextWindow.GetEditView();
//UNUSED2008-05  }

BOOL ScInputWindow::IsInputActive()
{
	return aTextWindow.IsInputActive();
}

EditView* ScInputWindow::GetEditView()
{
	return aTextWindow.GetEditView();
}

void ScInputWindow::MakeDialogEditView()
{
	aTextWindow.MakeDialogEditView();
}

void ScInputWindow::StopEditEngine( BOOL bAll )
{
	aTextWindow.StopEditEngine( bAll );
}

void ScInputWindow::TextGrabFocus()
{
	aTextWindow.GrabFocus();
}

void ScInputWindow::TextInvalidate()
{
	aTextWindow.Invalidate();
}

void ScInputWindow::SwitchToTextWin()
{
	// used for shift-ctrl-F2

	aTextWindow.StartEditEngine();
	if ( SC_MOD()->IsEditMode() )
	{
		aTextWindow.GrabFocus();
		EditView* pView = aTextWindow.GetEditView();
		if (pView)
		{
			xub_StrLen nLen = pView->GetEditEngine()->GetTextLen(0);
			ESelection aSel( 0, nLen, 0, nLen );
			pView->SetSelection( aSel );				// set cursor to end of text
		}
	}
}

void ScInputWindow::PosGrabFocus()
{
	aWndPos.GrabFocus();
}

void ScInputWindow::EnableButtons( BOOL bEnable )
{
	//	when enabling buttons, always also enable the input window itself
	if ( bEnable && !IsEnabled() )
		Enable();

	EnableItem( SID_INPUT_FUNCTION,									  bEnable );
	EnableItem( bIsOkCancelMode ? SID_INPUT_CANCEL : SID_INPUT_SUM,   bEnable );
	EnableItem( bIsOkCancelMode ? SID_INPUT_OK     : SID_INPUT_EQUAL, bEnable );
//	Invalidate();
}

void ScInputWindow::StateChanged( StateChangedType nType )
{
	ToolBox::StateChanged( nType );

	if ( nType == STATE_CHANGE_INITSHOW ) Resize();
}

void ScInputWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( rDCEvt.GetType() == DATACHANGED_SETTINGS && (rDCEvt.GetFlags() & SETTINGS_STYLE) )
	{
		//	update item images

		ScModule*		 pScMod  = SC_MOD();
        SfxImageManager* pImgMgr = SfxImageManager::GetImageManager( pScMod );
        BOOL bDark = GetSettings().GetStyleSettings().GetFaceColor().IsDark();
		// IMAGE macro uses pScMod, pImgMgr, bDark

		SetItemImage( SID_INPUT_FUNCTION, IMAGE( SID_INPUT_FUNCTION ) );
		if ( bIsOkCancelMode )
		{
			SetItemImage( SID_INPUT_CANCEL, IMAGE( SID_INPUT_CANCEL ) );
			SetItemImage( SID_INPUT_OK,     IMAGE( SID_INPUT_OK ) );
		}
		else
		{
			SetItemImage( SID_INPUT_SUM,   IMAGE( SID_INPUT_SUM ) );
			SetItemImage( SID_INPUT_EQUAL, IMAGE( SID_INPUT_EQUAL ) );
		}
	}

    ToolBox::DataChanged( rDCEvt );
}

//========================================================================
// 							Eingabefenster
//========================================================================

ScTextWnd::ScTextWnd( Window* pParent )
	:	Window		 ( pParent, WinBits(WB_HIDE | WB_BORDER) ),
		DragSourceHelper( this ),
		pEditEngine	 ( NULL ),
		pEditView	 ( NULL ),
		bIsInsertMode( TRUE ),
		bFormulaMode ( FALSE ),
        bInputMode   ( FALSE )
{
	EnableRTL( FALSE );		// #106269# EditEngine can't be used with VCL EnableRTL

	bIsRTL = GetSettings().GetLayoutRTL();

	//	#79096# always use application font, so a font with cjk chars can be installed
	Font aAppFont = GetFont();
	aTextFont = aAppFont;
	aTextFont.SetSize( PixelToLogic( aAppFont.GetSize(), MAP_TWIP ) );	// AppFont ist in Pixeln

	const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();

	Color aBgColor= rStyleSettings.GetWindowColor();
	Color aTxtColor= rStyleSettings.GetWindowTextColor();

	aTextFont.SetTransparent ( TRUE );
	aTextFont.SetFillColor   ( aBgColor );
	//aTextFont.SetColor		 ( COL_FIELDTEXT );
	aTextFont.SetColor		 (aTxtColor);
	aTextFont.SetWeight		 ( WEIGHT_NORMAL );

	SetSizePixel		( Size(1,TBX_WINDOW_HEIGHT) );
	SetBackground		( aBgColor );
	SetLineColor		( COL_BLACK );
	SetMapMode		    ( MAP_TWIP );
	SetPointer		    ( POINTER_TEXT );
}

__EXPORT ScTextWnd::~ScTextWnd()
{
	delete pEditView;
	delete pEditEngine;
    for( AccTextDataVector::reverse_iterator aIt = maAccTextDatas.rbegin(), aEnd = maAccTextDatas.rend(); aIt != aEnd; ++aIt )
        (*aIt)->Dispose();
}

void __EXPORT ScTextWnd::Paint( const Rectangle& rRec )
{
	if (pEditView)
		pEditView->Paint( rRec );
	else
	{
		SetFont( aTextFont );

		long nDiff =  GetOutputSizePixel().Height()
					- LogicToPixel( Size( 0, GetTextHeight() ) ).Height();
//		if (nDiff<2) nDiff=2;		// mind. 1 Pixel

		long nStartPos = TEXT_STARTPOS;
		if ( bIsRTL )
		{
			//	right-align
			nStartPos += GetOutputSizePixel().Width() - 2*TEXT_STARTPOS -
						LogicToPixel( Size( GetTextWidth( aString ), 0 ) ).Width();

			//	LayoutMode isn't changed as long as ModifyRTLDefaults doesn't include SvxFrameDirectionItem
		}

		DrawText( PixelToLogic( Point( nStartPos, nDiff/2 ) ), aString );
	}
}

void __EXPORT ScTextWnd::Resize()
{
	if (pEditView)
	{
		Size aSize = GetOutputSizePixel();
		long nDiff =  aSize.Height()
					- LogicToPixel( Size( 0, GetTextHeight() ) ).Height();

#ifdef OS2_DOCH_NICHT
		nDiff-=2;		// wird durch 2 geteilt
						// passt sonst nicht zur normalen Textausgabe
#endif

		aSize.Width() -= 2 * TEXT_STARTPOS - 1;

		pEditView->SetOutputArea(
			PixelToLogic( Rectangle( Point( TEXT_STARTPOS, (nDiff > 0) ? nDiff/2 : 1 ),
									 aSize ) ) );
	}
}

void __EXPORT ScTextWnd::MouseMove( const MouseEvent& rMEvt )
{
	if (pEditView)
		pEditView->MouseMove( rMEvt );
}

void __EXPORT ScTextWnd::MouseButtonDown( const MouseEvent& rMEvt )
{
	if (!HasFocus())
	{
		StartEditEngine();
		if ( SC_MOD()->IsEditMode() )
			GrabFocus();
	}

	if (pEditView)
	{
		pEditView->SetEditEngineUpdateMode( TRUE );
		pEditView->MouseButtonDown( rMEvt );
	}
}

void __EXPORT ScTextWnd::MouseButtonUp( const MouseEvent& rMEvt )
{
	if (pEditView)
		if (pEditView->MouseButtonUp( rMEvt ))
		{
			if ( rMEvt.IsMiddle() &&
		         	GetSettings().GetMouseSettings().GetMiddleButtonAction() == MOUSE_MIDDLE_PASTESELECTION )
		    {
		    	//	EditView may have pasted from selection
		    	SC_MOD()->InputChanged( pEditView );
		    }
			else
				SC_MOD()->InputSelection( pEditView );
		}
}

void __EXPORT ScTextWnd::Command( const CommandEvent& rCEvt )
{
    bInputMode = TRUE;
	USHORT nCommand = rCEvt.GetCommand();
	if ( pEditView /* && ( nCommand == COMMAND_STARTDRAG || nCommand == COMMAND_VOICE ) */ )
	{
		ScModule* pScMod = SC_MOD();
		ScTabViewShell* pStartViewSh = ScTabViewShell::GetActiveViewShell();

		// #109441# don't modify the font defaults here - the right defaults are
		// already set in StartEditEngine when the EditEngine is created

		// #63263# verhindern, dass die EditView beim View-Umschalten wegkommt
		pScMod->SetInEditCommand( TRUE );
		pEditView->Command( rCEvt );
		pScMod->SetInEditCommand( FALSE );

		//	#48929# COMMAND_STARTDRAG heiss noch lange nicht, dass der Inhalt geaendert wurde
		//	darum in dem Fall kein InputChanged
		//!	erkennen, ob mit Move gedraggt wurde, oder Drag&Move irgendwie verbieten

		if ( nCommand == COMMAND_STARTDRAG )
		{
			//	ist auf eine andere View gedraggt worden?
			ScTabViewShell* pEndViewSh = ScTabViewShell::GetActiveViewShell();
			if ( pEndViewSh != pStartViewSh && pStartViewSh != NULL )
			{
				ScViewData* pViewData = pStartViewSh->GetViewData();
				ScInputHandler* pHdl = pScMod->GetInputHdl( pStartViewSh );
				if ( pHdl && pViewData->HasEditView( pViewData->GetActivePart() ) )
				{
					pHdl->CancelHandler();
					pViewData->GetView()->ShowCursor();		// fehlt bei KillEditView, weil nicht aktiv
				}
			}
		}
		else if ( nCommand == COMMAND_CURSORPOS )
		{
			//	don't call InputChanged for COMMAND_CURSORPOS
		}
        else if ( nCommand == COMMAND_INPUTLANGUAGECHANGE )
        {
            // #i55929# Font and font size state depends on input language if nothing is selected,
            // so the slots have to be invalidated when the input language is changed.

            SfxViewFrame* pViewFrm = SfxViewFrame::Current();
            if (pViewFrm)
            {
                SfxBindings& rBindings = pViewFrm->GetBindings();
                rBindings.Invalidate( SID_ATTR_CHAR_FONT );
                rBindings.Invalidate( SID_ATTR_CHAR_FONTHEIGHT );
            }
        }
		else
			SC_MOD()->InputChanged( pEditView );
	}
	else
		Window::Command(rCEvt);		//	sonst soll sich die Basisklasse drum kuemmern...

    bInputMode = FALSE;
}

void ScTextWnd::StartDrag( sal_Int8 /* nAction */, const Point& rPosPixel )
{
	if ( pEditView )
	{
		CommandEvent aDragEvent( rPosPixel, COMMAND_STARTDRAG, TRUE );
		pEditView->Command( aDragEvent );

		//	handling of d&d to different view (CancelHandler) can't be done here,
		//	because the call returns before d&d is complete.
	}
}

void __EXPORT ScTextWnd::KeyInput(const KeyEvent& rKEvt)
{
    bInputMode = TRUE;
	if (!SC_MOD()->InputKeyEvent( rKEvt ))
	{
		BOOL bUsed = FALSE;
		ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
		if ( pViewSh )
			bUsed = pViewSh->SfxKeyInput(rKEvt);	// nur Acceleratoren, keine Eingabe
		if (!bUsed)
			Window::KeyInput( rKEvt );
	}
    bInputMode = FALSE;
}

void __EXPORT ScTextWnd::GetFocus()
{
    ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
    if ( pViewSh )
        pViewSh->SetFormShellAtTop( FALSE );     // focus in input line -> FormShell no longer on top
}

void __EXPORT ScTextWnd::LoseFocus()
{
}

String __EXPORT ScTextWnd::GetText() const
{
	//	ueberladen, um per Testtool an den Text heranzukommen

	if ( pEditEngine )
		return pEditEngine->GetText();
	else
		return GetTextString();
}

void ScTextWnd::SetFormulaMode( BOOL bSet )
{
	if ( bSet != bFormulaMode )
	{
		bFormulaMode = bSet;
		UpdateAutoCorrFlag();
	}
}

void ScTextWnd::UpdateAutoCorrFlag()
{
	if ( pEditEngine )
	{
		ULONG nControl = pEditEngine->GetControlWord();
		ULONG nOld = nControl;
		if ( bFormulaMode )
			nControl &= ~EE_CNTRL_AUTOCORRECT;		// keine Autokorrektur in Formeln
		else
			nControl |= EE_CNTRL_AUTOCORRECT;		// sonst schon
		if ( nControl != nOld )
			pEditEngine->SetControlWord( nControl );
	}
}

void lcl_ExtendEditFontAttribs( SfxItemSet& rSet )
{
	const SfxPoolItem& rFontItem = rSet.Get( EE_CHAR_FONTINFO );
	rSet.Put( rFontItem, EE_CHAR_FONTINFO_CJK );
	rSet.Put( rFontItem, EE_CHAR_FONTINFO_CTL );
	const SfxPoolItem& rHeightItem = rSet.Get( EE_CHAR_FONTHEIGHT );
	rSet.Put( rHeightItem, EE_CHAR_FONTHEIGHT_CJK );
	rSet.Put( rHeightItem, EE_CHAR_FONTHEIGHT_CTL );
	const SfxPoolItem& rWeightItem = rSet.Get( EE_CHAR_WEIGHT );
	rSet.Put( rWeightItem, EE_CHAR_WEIGHT_CJK );
	rSet.Put( rWeightItem, EE_CHAR_WEIGHT_CTL );
	const SfxPoolItem& rItalicItem = rSet.Get( EE_CHAR_ITALIC );
	rSet.Put( rItalicItem, EE_CHAR_ITALIC_CJK );
	rSet.Put( rItalicItem, EE_CHAR_ITALIC_CTL );
	const SfxPoolItem& rLangItem = rSet.Get( EE_CHAR_LANGUAGE );
	rSet.Put( rLangItem, EE_CHAR_LANGUAGE_CJK );
	rSet.Put( rLangItem, EE_CHAR_LANGUAGE_CTL );
}

void lcl_ModifyRTLDefaults( SfxItemSet& rSet )
{
	rSet.Put( SvxAdjustItem( SVX_ADJUST_RIGHT, EE_PARA_JUST ) );

	//	always using rtl writing direction would break formulas
	//rSet.Put( SvxFrameDirectionItem( FRMDIR_HORI_RIGHT_TOP, EE_PARA_WRITINGDIR ) );

	//	PaperSize width is limited to USHRT_MAX in RTL mode (because of EditEngine's
	//	USHORT values in EditLine), so the text may be wrapped and line spacing must be
	//	increased to not see the beginning of the next line.
	SvxLineSpacingItem aItem( SVX_LINESPACE_TWO_LINES, EE_PARA_SBL );
	aItem.SetPropLineSpace( 200 );
	rSet.Put( aItem );
}

void lcl_ModifyRTLVisArea( EditView* pEditView )
{
	Rectangle aVisArea = pEditView->GetVisArea();
	Size aPaper = pEditView->GetEditEngine()->GetPaperSize();
	long nDiff = aPaper.Width() - aVisArea.Right();
	aVisArea.Left()  += nDiff;
	aVisArea.Right() += nDiff;
	pEditView->SetVisArea(aVisArea);
}

void ScTextWnd::StartEditEngine()
{
	//	#31147# Bei "eigener Modalitaet" (Doc-modale Dialoge) nicht aktivieren
	SfxObjectShell* pObjSh = SfxObjectShell::Current();
	if ( pObjSh && pObjSh->IsInModalMode() )
		return;

	if ( !pEditView || !pEditEngine )
	{
		ScFieldEditEngine* pNew;
		ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
		if ( pViewSh )
		{
			const ScDocument* pDoc = pViewSh->GetViewData()->GetDocument();
			pNew = new ScFieldEditEngine( pDoc->GetEnginePool(), pDoc->GetEditPool() );
		}
		else
			pNew = new ScFieldEditEngine( EditEngine::CreatePool(),	NULL, TRUE );
		pNew->SetExecuteURL( FALSE );
		pEditEngine = pNew;

		pEditEngine->SetUpdateMode( FALSE );
		pEditEngine->SetPaperSize( Size( bIsRTL ? USHRT_MAX : THESIZE, 300 ) );
		pEditEngine->SetWordDelimiters(
						ScEditUtil::ModifyDelimiters( pEditEngine->GetWordDelimiters() ) );

		UpdateAutoCorrFlag();

		{
			SfxItemSet* pSet = new SfxItemSet( pEditEngine->GetEmptyItemSet() );
			pEditEngine->SetFontInfoInItemSet( *pSet, aTextFont );
			lcl_ExtendEditFontAttribs( *pSet );
			// turn off script spacing to match DrawText output
			pSet->Put( SvxScriptSpaceItem( FALSE, EE_PARA_ASIANCJKSPACING ) );
			if ( bIsRTL )
				lcl_ModifyRTLDefaults( *pSet );
			pEditEngine->SetDefaults( pSet );
		}

		//	#57254# Wenn in der Zelle URL-Felder enthalten sind, muessen die auch in
		//	die Eingabezeile uebernommen werden, weil sonst die Positionen nicht stimmen.

		BOOL bFilled = FALSE;
		ScInputHandler* pHdl = SC_MOD()->GetInputHdl();
		if ( pHdl )			//!	Testen, ob's der richtige InputHdl ist?
			bFilled = pHdl->GetTextAndFields( *pEditEngine );

		pEditEngine->SetUpdateMode( TRUE );

		//	aString ist die Wahrheit...
		if ( bFilled && pEditEngine->GetText() == aString )
			Invalidate();						// Repaint fuer (hinterlegte) Felder
		else
			pEditEngine->SetText(aString);		// dann wenigstens den richtigen Text

		pEditView = new EditView( pEditEngine, this );
		pEditView->SetInsertMode(bIsInsertMode);

		// Text aus Clipboard wird als ASCII einzeilig uebernommen
		ULONG n = pEditView->GetControlWord();
		pEditView->SetControlWord( n | EV_CNTRL_SINGLELINEPASTE	);

		pEditEngine->InsertView( pEditView, EE_APPEND );

		Resize();

		if ( bIsRTL )
			lcl_ModifyRTLVisArea( pEditView );

        pEditEngine->SetModifyHdl(LINK(this, ScTextWnd, NotifyHdl));

        if (!maAccTextDatas.empty())
            maAccTextDatas.back()->StartEdit();

		//	as long as EditEngine and DrawText sometimes differ for CTL text,
		//	repaint now to have the EditEngine's version visible
//        SfxObjectShell* pObjSh = SfxObjectShell::Current();
		if ( pObjSh && pObjSh->ISA(ScDocShell) )
		{
			ScDocument* pDoc = ((ScDocShell*)pObjSh)->GetDocument();	// any document
			BYTE nScript = pDoc->GetStringScriptType( aString );
			if ( nScript & SCRIPTTYPE_COMPLEX )
				Invalidate();
		}
    }

	SC_MOD()->SetInputMode( SC_INPUT_TOP );

	SfxViewFrame* pViewFrm = SfxViewFrame::Current();
	if (pViewFrm)
		pViewFrm->GetBindings().Invalidate( SID_ATTR_INSERT );
}

IMPL_LINK(ScTextWnd, NotifyHdl, EENotify*, EMPTYARG)
{
    if (pEditView && !bInputMode)
	{
		ScInputHandler* pHdl = SC_MOD()->GetInputHdl();

		//	#105354# Use the InputHandler's InOwnChange flag to prevent calling InputChanged
		//	while an InputHandler method is modifying the EditEngine content

		if ( pHdl && !pHdl->IsInOwnChange() )
			pHdl->InputChanged( pEditView, TRUE );	// #i20282# InputChanged must know if called from modify handler
	}

    return 0;
}

void ScTextWnd::StopEditEngine( BOOL bAll )
{
	if (pEditView)
	{
        if (!maAccTextDatas.empty())
            maAccTextDatas.back()->EndEdit();

		ScModule* pScMod = SC_MOD();

		if (!bAll)
			pScMod->InputSelection( pEditView );
		aString = pEditEngine->GetText();
		bIsInsertMode = pEditView->IsInsertMode();
		BOOL bSelection = pEditView->HasSelection();
        pEditEngine->SetModifyHdl(Link());
		DELETEZ(pEditView);
		DELETEZ(pEditEngine);

		if ( pScMod->IsEditMode() && !bAll )
			pScMod->SetInputMode(SC_INPUT_TABLE);

		SfxViewFrame* pViewFrm = SfxViewFrame::Current();
		if (pViewFrm)
			pViewFrm->GetBindings().Invalidate( SID_ATTR_INSERT );

		if (bSelection)
			Invalidate();			// damit Selektion nicht stehenbleibt
	}
}

void ScTextWnd::SetTextString( const String& rNewString )
{
	if ( rNewString != aString )
	{
        bInputMode = TRUE;

		//	Position der Aenderung suchen, nur Rest painten

		long nInvPos = 0;
		long nStartPos = 0;
		long nTextSize = 0;

		if (!pEditEngine)
		{
			BOOL bPaintAll;
			if ( bIsRTL )
				bPaintAll = TRUE;
			else
			{
				//	test if CTL script type is involved
				BYTE nOldScript = 0;
				BYTE nNewScript = 0;
				SfxObjectShell* pObjSh = SfxObjectShell::Current();
				if ( pObjSh && pObjSh->ISA(ScDocShell) )
				{
					//	any document can be used (used only for its break iterator)
					ScDocument* pDoc = ((ScDocShell*)pObjSh)->GetDocument();
					nOldScript = pDoc->GetStringScriptType( aString );
					nNewScript = pDoc->GetStringScriptType( rNewString );
				}
				bPaintAll = ( nOldScript & SCRIPTTYPE_COMPLEX ) || ( nNewScript & SCRIPTTYPE_COMPLEX );
			}

			if ( bPaintAll )
			{
				// if CTL is involved, the whole text has to be redrawn
				Invalidate();
			}
			else
			{
				xub_StrLen nDifPos;
				if (rNewString.Len() > aString.Len())
					nDifPos = rNewString.Match(aString);
				else
					nDifPos = aString.Match(rNewString);

				long nSize1 = GetTextWidth(aString);
				long nSize2 = GetTextWidth(rNewString);
				if ( nSize1>0 && nSize2>0 )
					nTextSize = Max( nSize1, nSize2 );
				else
					nTextSize = GetOutputSize().Width();		// Ueberlauf

				if (nDifPos == STRING_MATCH)
					nDifPos = 0;

												// -1 wegen Rundung und "A"
				Point aLogicStart = PixelToLogic(Point(TEXT_STARTPOS-1,0));
				nStartPos = aLogicStart.X();
				nInvPos = nStartPos;
				if (nDifPos)
					nInvPos += GetTextWidth(aString,0,nDifPos);

				USHORT nFlags = 0;
				if ( nDifPos == aString.Len() )			// only new characters appended
					nFlags = INVALIDATE_NOERASE;		// then background is already clear

				Invalidate( Rectangle( nInvPos, 0,
										nStartPos+nTextSize, GetOutputSize().Height()-1 ),
							nFlags );
			}
		}
		else
		{
			pEditEngine->SetText(rNewString);
		}

		aString = rNewString;

        if (!maAccTextDatas.empty())
            maAccTextDatas.back()->TextChanged();

        bInputMode = FALSE;
    }
}

const String& ScTextWnd::GetTextString() const
{
	return aString;
}

BOOL ScTextWnd::IsInputActive()
{
	return HasFocus();
}

EditView* ScTextWnd::GetEditView()
{
	return pEditView;
}

void ScTextWnd::MakeDialogEditView()
{
	if ( pEditView ) return;

	ScFieldEditEngine* pNew;
	ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
	if ( pViewSh )
	{
		const ScDocument* pDoc = pViewSh->GetViewData()->GetDocument();
		pNew = new ScFieldEditEngine( pDoc->GetEnginePool(), pDoc->GetEditPool() );
	}
	else
		pNew = new ScFieldEditEngine( EditEngine::CreatePool(),	NULL, TRUE );
	pNew->SetExecuteURL( FALSE );
	pEditEngine = pNew;

	pEditEngine->SetUpdateMode( FALSE );
	pEditEngine->SetWordDelimiters( pEditEngine->GetWordDelimiters() += '=' );
	pEditEngine->SetPaperSize( Size( bIsRTL ? USHRT_MAX : THESIZE, 300 ) );

	SfxItemSet* pSet = new SfxItemSet( pEditEngine->GetEmptyItemSet() );
	pEditEngine->SetFontInfoInItemSet( *pSet, aTextFont );
	lcl_ExtendEditFontAttribs( *pSet );
	if ( bIsRTL )
		lcl_ModifyRTLDefaults( *pSet );
	pEditEngine->SetDefaults( pSet );
	pEditEngine->SetUpdateMode( TRUE );

	pEditView	= new EditView( pEditEngine, this );
	pEditEngine->InsertView( pEditView, EE_APPEND );

	Resize();

	if ( bIsRTL )
		lcl_ModifyRTLVisArea( pEditView );

    if (!maAccTextDatas.empty())
        maAccTextDatas.back()->StartEdit();
}

void ScTextWnd::ImplInitSettings()
{
	bIsRTL = GetSettings().GetLayoutRTL();

	const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();

	Color aBgColor= rStyleSettings.GetWindowColor();
	Color aTxtColor= rStyleSettings.GetWindowTextColor();

	aTextFont.SetFillColor   ( aBgColor );
	aTextFont.SetColor		 (aTxtColor);
	SetBackground			( aBgColor );
	Invalidate();
}

::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > ScTextWnd::CreateAccessible()
{
    return new ScAccessibleEditObject(GetAccessibleParentWindow()->GetAccessible(), NULL, this,
        rtl::OUString(String(ScResId(STR_ACC_EDITLINE_NAME))),
        rtl::OUString(String(ScResId(STR_ACC_EDITLINE_DESCR))), EditLine);
}

void ScTextWnd::InsertAccessibleTextData( ScAccessibleEditLineTextData& rTextData )
{
    OSL_ENSURE( ::std::find( maAccTextDatas.begin(), maAccTextDatas.end(), &rTextData ) == maAccTextDatas.end(),
        "ScTextWnd::InsertAccessibleTextData - passed object already registered" );
    maAccTextDatas.push_back( &rTextData );
}

void ScTextWnd::RemoveAccessibleTextData( ScAccessibleEditLineTextData& rTextData )
{
    AccTextDataVector::iterator aEnd = maAccTextDatas.end();
    AccTextDataVector::iterator aIt = ::std::find( maAccTextDatas.begin(), aEnd, &rTextData );
    OSL_ENSURE( aIt != aEnd, "ScTextWnd::RemoveAccessibleTextData - passed object not registered" );
    if( aIt != aEnd )
        maAccTextDatas.erase( aIt );
}

// -----------------------------------------------------------------------

void ScTextWnd::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
		 (rDCEvt.GetFlags() & SETTINGS_STYLE) )
	{
		ImplInitSettings();
		Invalidate();
	}
	else
		Window::DataChanged( rDCEvt );
}


//========================================================================
// 							Positionsfenster
//========================================================================

ScPosWnd::ScPosWnd( Window* pParent ) :
	ComboBox	( pParent, WinBits(WB_HIDE | WB_DROPDOWN) ),
	pAccel		( NULL ),
    nTipVisible ( 0 ),
	bFormulaMode( FALSE )
{
	Size aSize( GetTextWidth( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("GW99999:GW99999")) ),
				GetTextHeight() );
	aSize.Width() += 25;	// ??
	aSize.Height() = CalcWindowSizePixel(11);		// Funktionen: 10 MRU + "andere..."
	SetSizePixel( aSize );

	FillRangeNames();

	StartListening( *SFX_APP() );		// fuer Navigator-Bereichsnamen-Updates
}

__EXPORT ScPosWnd::~ScPosWnd()
{
	EndListening( *SFX_APP() );

    HideTip();

	delete pAccel;
}

void ScPosWnd::SetFormulaMode( BOOL bSet )
{
	if ( bSet != bFormulaMode )
	{
		bFormulaMode = bSet;

		if ( bSet )
			FillFunctions();
		else
			FillRangeNames();

        HideTip();
	}
}

void ScPosWnd::SetPos( const String& rPosStr )
{
	if ( aPosStr != rPosStr )
	{
		aPosStr = rPosStr;
		SetText(aPosStr);
	}
}

void ScPosWnd::FillRangeNames()
{
	Clear();

	SfxObjectShell* pObjSh = SfxObjectShell::Current();
	if ( pObjSh && pObjSh->ISA(ScDocShell) )
	{
		ScDocument* pDoc = ((ScDocShell*)pObjSh)->GetDocument();

		//	per Hand sortieren, weil Funktionen nicht sortiert werden:

		ScRangeName* pRangeNames = pDoc->GetRangeName();
		USHORT nCount = pRangeNames->GetCount();
		if ( nCount > 0 )
		{
			USHORT nValidCount = 0;
			ScRange aDummy;
			USHORT i;
			for ( i=0; i<nCount; i++ )
			{
				ScRangeData* pData = (*pRangeNames)[i];
				if (pData->IsValidReference(aDummy))
					nValidCount++;
			}
			if ( nValidCount )
			{
				ScRangeData** ppSortArray = new ScRangeData* [ nValidCount ];
				USHORT j;
				for ( i=0, j=0; i<nCount; i++ )
				{
					ScRangeData* pData = (*pRangeNames)[i];
					if (pData->IsValidReference(aDummy))
						ppSortArray[j++] = pData;
				}
#ifndef ICC
				qsort( (void*)ppSortArray, nValidCount, sizeof(ScRangeData*),
					&ScRangeData_QsortNameCompare );
#else
				qsort( (void*)ppSortArray, nValidCount, sizeof(ScRangeData*),
					ICCQsortNameCompare );
#endif
				for ( j=0; j<nValidCount; j++ )
					InsertEntry( ppSortArray[j]->GetName() );
				delete [] ppSortArray;
			}
		}
	}
	SetText(aPosStr);
}

void ScPosWnd::FillFunctions()
{
	Clear();

	String aFirstName;
	const ScAppOptions& rOpt = SC_MOD()->GetAppOptions();
	USHORT nMRUCount = rOpt.GetLRUFuncListCount();
	const USHORT* pMRUList = rOpt.GetLRUFuncList();
	if (pMRUList)
	{
		const ScFunctionList* pFuncList = ScGlobal::GetStarCalcFunctionList();
		ULONG nListCount = pFuncList->GetCount();
		for (USHORT i=0; i<nMRUCount; i++)
		{
			USHORT nId = pMRUList[i];
			for (ULONG j=0; j<nListCount; j++)
			{
				const ScFuncDesc* pDesc = pFuncList->GetFunction( j );
				if ( pDesc->nFIndex == nId && pDesc->pFuncName )
				{
					InsertEntry( *pDesc->pFuncName );
					if (!aFirstName.Len())
						aFirstName = *pDesc->pFuncName;
					break;	// nicht weitersuchen
				}
			}
		}
	}

	//!	Eintrag "Andere..." fuer Funktions-Autopilot wieder aufnehmen,
	//!	wenn der Funktions-Autopilot mit dem bisher eingegebenen Text arbeiten kann!

//	InsertEntry( ScGlobal::GetRscString(STR_FUNCTIONLIST_MORE) );

	SetText(aFirstName);
}

void __EXPORT ScPosWnd::Notify( SfxBroadcaster&, const SfxHint& rHint )
{
	if ( !bFormulaMode )
	{
		//	muss die Liste der Bereichsnamen updgedated werden?

		if ( rHint.ISA(SfxSimpleHint) )
		{
			ULONG nHintId = ((SfxSimpleHint&)rHint).GetId();
			if ( nHintId == SC_HINT_AREAS_CHANGED || nHintId == SC_HINT_NAVIGATOR_UPDATEALL)
				FillRangeNames();
		}
		else if ( rHint.ISA(SfxEventHint) )
		{
			ULONG nEventId = ((SfxEventHint&)rHint).GetEventId();
			if ( nEventId == SFX_EVENT_ACTIVATEDOC )
				FillRangeNames();
		}
	}
}

void ScPosWnd::HideTip()
{
    if ( nTipVisible )
    {
        Help::HideTip( nTipVisible );
        nTipVisible = 0;
    }
}

ScNameInputType lcl_GetInputType( const String& rText )
{
    ScNameInputType eRet = SC_NAME_INPUT_BAD_NAME;      // the more general error

    ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
    if ( pViewSh )
    {
        ScViewData* pViewData = pViewSh->GetViewData();
        ScDocument* pDoc = pViewData->GetDocument();
        SCTAB nTab = pViewData->GetTabNo();
        formula::FormulaGrammar::AddressConvention eConv = pDoc->GetAddressConvention();

        // test in same order as in SID_CURRENTCELL execute

        ScRange aRange;
        ScAddress aAddress;
        ScRangeUtil aRangeUtil;
        SCTAB nNameTab;
        sal_Int32 nNumeric;

        if ( aRange.Parse( rText, pDoc, eConv ) & SCA_VALID )
            eRet = SC_NAME_INPUT_NAMEDRANGE;
        else if ( aAddress.Parse( rText, pDoc, eConv ) & SCA_VALID )
            eRet = SC_NAME_INPUT_CELL;
        else if ( aRangeUtil.MakeRangeFromName( rText, pDoc, nTab, aRange, RUTL_NAMES, eConv ) )
            eRet = SC_NAME_INPUT_NAMEDRANGE;
        else if ( aRangeUtil.MakeRangeFromName( rText, pDoc, nTab, aRange, RUTL_DBASE, eConv ) )
            eRet = SC_NAME_INPUT_DATABASE;
        else if ( ByteString( rText, RTL_TEXTENCODING_ASCII_US ).IsNumericAscii() &&
                  ( nNumeric = rText.ToInt32() ) > 0 && nNumeric <= MAXROW+1 )
            eRet = SC_NAME_INPUT_ROW;
        else if ( pDoc->GetTable( rText, nNameTab ) )
            eRet = SC_NAME_INPUT_SHEET;
        else if ( ScRangeData::IsNameValid( rText, pDoc ) )     // nothing found, create new range?
        {
            if ( pViewData->GetSimpleArea( aRange ) == SC_MARK_SIMPLE )
                eRet = SC_NAME_INPUT_DEFINE;
            else
                eRet = SC_NAME_INPUT_BAD_SELECTION;
        }
        else
            eRet = SC_NAME_INPUT_BAD_NAME;
    }

    return eRet;
}

void ScPosWnd::Modify()
{
    ComboBox::Modify();

    HideTip();

    if ( !IsTravelSelect() && !bFormulaMode )
    {
        // determine the action that would be taken for the current input

        ScNameInputType eType = lcl_GetInputType( GetText() );      // uses current view
        USHORT nStrId = 0;
        switch ( eType )
        {
            case SC_NAME_INPUT_CELL:
                nStrId = STR_NAME_INPUT_CELL;
                break;
            case SC_NAME_INPUT_RANGE:
            case SC_NAME_INPUT_NAMEDRANGE:
                nStrId = STR_NAME_INPUT_RANGE;      // named range or range reference
                break;
            case SC_NAME_INPUT_DATABASE:
                nStrId = STR_NAME_INPUT_DBRANGE;
                break;
            case SC_NAME_INPUT_ROW:
                nStrId = STR_NAME_INPUT_ROW;
                break;
            case SC_NAME_INPUT_SHEET:
                nStrId = STR_NAME_INPUT_SHEET;
                break;
            case SC_NAME_INPUT_DEFINE:
                nStrId = STR_NAME_INPUT_DEFINE;
                break;
            default:
                // other cases (error): no tip help
                break;
        }

        if ( nStrId )
        {
            // show the help tip at the text cursor position

            Window* pWin = GetSubEdit();
            if (!pWin)
                pWin = this;
            Point aPos;
            Cursor* pCur = pWin->GetCursor();
            if (pCur)
                aPos = pWin->LogicToPixel( pCur->GetPos() );
            aPos = pWin->OutputToScreenPixel( aPos );
            Rectangle aRect( aPos, aPos );

            String aText = ScGlobal::GetRscString( nStrId );
            USHORT nAlign = QUICKHELP_LEFT|QUICKHELP_BOTTOM;
            nTipVisible = Help::ShowTip(pWin, aRect, aText, nAlign);
        }
    }
}

void __EXPORT ScPosWnd::Select()
{
	ComboBox::Select();		//	in VCL gibt GetText() erst danach den ausgewaehlten Eintrag

    HideTip();

	if (!IsTravelSelect())
		DoEnter();
}

void ScPosWnd::DoEnter()
{
	String aText = GetText();
	if ( aText.Len() )
	{
		if ( bFormulaMode )
		{
			ScModule* pScMod = SC_MOD();
			if ( aText == ScGlobal::GetRscString(STR_FUNCTIONLIST_MORE) )
			{
				//	Funktions-Autopilot
				//!	mit dem bisher eingegebenen Text weiterarbeiten !!!

				//!	new method at ScModule to query if function autopilot is open
				SfxViewFrame* pViewFrm = SfxViewFrame::Current();
				if ( pViewFrm && !pViewFrm->GetChildWindow( SID_OPENDLG_FUNCTION ) )
					pViewFrm->GetDispatcher()->Execute( SID_OPENDLG_FUNCTION,
											  SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD );
			}
			else
			{
				ScTabViewShell* pViewSh = PTR_CAST( ScTabViewShell, SfxViewShell::Current() );
				ScInputHandler* pHdl = pScMod->GetInputHdl( pViewSh );
				if (pHdl)
					pHdl->InsertFunction( aText );
			}
		}
		else
		{
            // depending on the input, select something or create a new named range

            ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
            if ( pViewSh )
            {
                ScNameInputType eType = lcl_GetInputType( aText );
                if ( eType == SC_NAME_INPUT_BAD_NAME || eType == SC_NAME_INPUT_BAD_SELECTION )
                {
                    USHORT nId = ( eType == SC_NAME_INPUT_BAD_NAME ) ? STR_NAME_ERROR_NAME : STR_NAME_ERROR_SELECTION;
                    pViewSh->ErrorMessage( nId );
                }
                else if ( eType == SC_NAME_INPUT_DEFINE )
                {
                    ScViewData* pViewData = pViewSh->GetViewData();
                    ScDocShell* pDocShell = pViewData->GetDocShell();
                    ScDocument* pDoc = pDocShell->GetDocument();
                    ScRangeName* pNames = pDoc->GetRangeName();
                    ScRange aSelection;
                    USHORT nIndex = 0;
                    if ( pNames && !pNames->SearchName( aText, nIndex ) &&
                            (pViewData->GetSimpleArea( aSelection ) == SC_MARK_SIMPLE) )
                    {
                        ScRangeName aNewRanges( *pNames );
                        ScAddress aCursor( pViewData->GetCurX(), pViewData->GetCurY(), pViewData->GetTabNo() );
                        String aContent;
                        aSelection.Format( aContent, SCR_ABS_3D, pDoc, pDoc->GetAddressConvention() );
                        ScRangeData* pNew = new ScRangeData( pDoc, aText, aContent, aCursor );
                        if ( aNewRanges.Insert(pNew) )
                        {
                            ScDocFunc aFunc(*pDocShell);
                            aFunc.ModifyRangeNames( aNewRanges, FALSE );
                            pViewSh->UpdateInputHandler(TRUE);
                        }
                        else
                            delete pNew;        // shouldn't happen
                    }
                }
                else
                {
                    // for all selection types, excecute the SID_CURRENTCELL slot

                    SfxStringItem aPosItem( SID_CURRENTCELL, aText );
                    SfxBoolItem aUnmarkItem( FN_PARAM_1, TRUE );        // remove existing selection

                    pViewSh->GetViewData()->GetDispatcher().Execute( SID_CURRENTCELL,
                                        SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD,
                                        &aPosItem, &aUnmarkItem, 0L );
                }
            }
		}
	}
	else
		SetText( aPosStr );

	ReleaseFocus_Impl();
}

long __EXPORT ScPosWnd::Notify( NotifyEvent& rNEvt )
{
	long nHandled = 0;

	if ( rNEvt.GetType() == EVENT_KEYINPUT )
	{
		const KeyEvent* pKEvt = rNEvt.GetKeyEvent();

		switch ( pKEvt->GetKeyCode().GetCode() )
		{
			case KEY_RETURN:
				DoEnter();
				nHandled = 1;
				break;

			case KEY_ESCAPE:
                if (nTipVisible)
                {
                    // escape when the tip help is shown: only hide the tip
                    HideTip();
                }
                else
                {
                    if (!bFormulaMode)
                        SetText( aPosStr );
                    ReleaseFocus_Impl();
                }
				nHandled = 1;
				break;
		}
	}

	if ( !nHandled )
		nHandled = ComboBox::Notify( rNEvt );

    if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
        HideTip();

	return nHandled;
}

void ScPosWnd::ReleaseFocus_Impl()
{
    HideTip();

	SfxViewShell* pCurSh = SfxViewShell::Current();
	ScInputHandler* pHdl = SC_MOD()->GetInputHdl( PTR_CAST( ScTabViewShell, pCurSh ) );
	if ( pHdl && pHdl->IsTopMode() )
	{
		//	Focus wieder in die Eingabezeile?

		ScInputWindow* pInputWin = pHdl->GetInputWindow();
		if (pInputWin)
		{
			pInputWin->TextGrabFocus();
			return;
		}
	}

	//	Focus auf die aktive View

	if ( pCurSh )
	{
		Window* pShellWnd = pCurSh->GetWindow();

		if ( pShellWnd )
			pShellWnd->GrabFocus();
	}
}






