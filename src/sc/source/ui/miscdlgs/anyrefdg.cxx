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



//----------------------------------------------------------------------------

#include "rangelst.hxx"
#include <sfx2/app.hxx>
#include <sfx2/viewsh.hxx>
#include <vcl/wrkwin.hxx>
#include <vcl/mnemonic.hxx>
#include <tools/shl.hxx>
#include <svtools/taskbar.hxx>
#include <sfx2/topfrm.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>


#define ANYREFDG_CXX
#include "anyrefdg.hxx"
#undef ANYREFDG_CXX

#include "sc.hrc"
#include "inputhdl.hxx"
#include "scmod.hxx"
#include "scresid.hxx"
#include "inputwin.hxx"
#include "tabvwsh.hxx"
#include "docsh.hxx"
#include "rfindlst.hxx"
#include "compiler.hxx"
#include "cell.hxx"
#include "global.hxx"
#include "inputopt.hxx"
#include "rangeutl.hxx"
    

ScFormulaReferenceHelper::ScFormulaReferenceHelper(IAnyRefDialog* _pDlg,SfxBindings* _pBindings) 
 : m_pDlg(_pDlg)
 , pRefEdit (NULL)
 , m_pWindow(NULL)
 , m_pBindings(_pBindings)
 , pAccel( NULL )
 , pHiddenMarks(NULL)
 , nRefTab(0)
 , bHighLightRef( FALSE )
 , bAccInserted( FALSE )
{
    ScInputOptions aInputOption=SC_MOD()->GetInputOptions();
	bEnableColorRef=aInputOption.GetRangeFinder();
}
// -----------------------------------------------------------------------------
ScFormulaReferenceHelper::~ScFormulaReferenceHelper()
{
    if (bAccInserted)
		Application::RemoveAccel( pAccel.get() );

    // common cleanup for ScAnyRefDlg and ScFormulaDlg is done here

    HideReference();
    enableInput( TRUE );

    ScInputHandler* pInputHdl = SC_MOD()->GetInputHdl();
    if ( pInputHdl )
        pInputHdl->ResetDelayTimer();   // stop the timer for disabling the input line
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::enableInput( BOOL bEnable )
{
    TypeId aType(TYPE(ScDocShell));
    ScDocShell* pDocShell = (ScDocShell*)SfxObjectShell::GetFirst(&aType);
    while( pDocShell )
    {
	    SfxViewFrame* pFrame = SfxViewFrame::GetFirst( pDocShell );
	    while( pFrame )
	    {
		    //	#71577# enable everything except InPlace, including bean frames
            if ( !pFrame->GetFrame()->IsInPlace() )
		    {
			    SfxViewShell* p = pFrame->GetViewShell();
			    ScTabViewShell* pViewSh = PTR_CAST(ScTabViewShell,p);
			    if(pViewSh!=NULL)
			    {
				    Window *pWin=pViewSh->GetWindow();
				    if(pWin)
				    {
					    Window *pParent=pWin->GetParent();
					    if(pParent)
					    {
						    pParent->EnableInput(bEnable,TRUE /* FALSE */);
						    if(TRUE /*bChilds*/)
							    pViewSh->EnableRefInput(bEnable);
					    }
				    }
			    }
		    }
		    pFrame = SfxViewFrame::GetNext( *pFrame, pDocShell );
	    }

	    pDocShell = (ScDocShell*)SfxObjectShell::GetNext(*pDocShell, &aType);
    }
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ShowSimpleReference( const XubString& rStr )
{
    if( /*!pRefEdit &&*/ bEnableColorRef )
	{
		bHighLightRef=TRUE;
		ScViewData* pViewData=ScDocShell::GetViewData();
		if ( pViewData )
		{
			ScDocument* pDoc=pViewData->GetDocument();
			ScTabViewShell*	pTabViewShell=pViewData->GetViewShell();

			ScRangeList aRangeList;

			pTabViewShell->DoneRefMode( FALSE );
			pTabViewShell->ClearHighlightRanges();

			if( ParseWithNames( aRangeList, rStr, pDoc ) )
			{
				ScRange* pRangeEntry = aRangeList.First();

				USHORT nIndex=0;
				while(pRangeEntry != NULL)
				{
					ColorData aColName = ScRangeFindList::GetColorName(nIndex++);
					pTabViewShell->AddHighlightRange(*pRangeEntry, aColName);

					pRangeEntry = aRangeList.Next();
				}
			}
		}
	}
}
// -----------------------------------------------------------------------------
bool ScFormulaReferenceHelper::ParseWithNames( ScRangeList& rRanges, const String& rStr, ScDocument* pDoc )
{
    bool bError = false;
    rRanges.RemoveAll();

    ScAddress::Details aDetails(pDoc->GetAddressConvention(), 0, 0);
    ScRangeUtil aRangeUtil;
    xub_StrLen nTokenCnt = rStr.GetTokenCount();
    for( xub_StrLen nToken = 0; nToken < nTokenCnt; ++nToken )
    {
        ScRange aRange;
        String aRangeStr( rStr.GetToken( nToken ) );

        USHORT nFlags = aRange.ParseAny( aRangeStr, pDoc, aDetails );
        if ( nFlags & SCA_VALID )
        {
            if ( (nFlags & SCA_TAB_3D) == 0 )
                aRange.aStart.SetTab( nRefTab );
            if ( (nFlags & SCA_TAB2_3D) == 0 )
                aRange.aEnd.SetTab( aRange.aStart.Tab() );
            rRanges.Append( aRange );
        }
        else if ( aRangeUtil.MakeRangeFromName( aRangeStr, pDoc, nRefTab, aRange, RUTL_NAMES, aDetails ) )
            rRanges.Append( aRange );
        else
            bError = true;        
    }

    return !bError;
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ShowFormulaReference( const XubString& rStr )
{
    if( /*!pRefEdit &&*/ bEnableColorRef)
	{
		bHighLightRef=TRUE;
		ScViewData* pViewData=ScDocShell::GetViewData();
		if ( pViewData && pRefComp.get() )
		{
			ScTabViewShell*	pTabViewShell=pViewData->GetViewShell();
			SCCOL nCol = pViewData->GetCurX();
			SCROW nRow = pViewData->GetCurY();
			SCTAB nTab = pViewData->GetTabNo();
			ScAddress aPos( nCol, nRow, nTab );

			ScTokenArray* pScTokA=pRefComp->CompileString(rStr);
			//pRefComp->CompileTokenArray();

			if(pTabViewShell!=NULL && pScTokA!=NULL)
			{
				pTabViewShell->DoneRefMode( FALSE );
				pTabViewShell->ClearHighlightRanges();

				pScTokA->Reset();
				const ScToken* pToken = static_cast<const ScToken*>(pScTokA->GetNextReference());

				USHORT nIndex=0;

				while(pToken!=NULL)
				{
					BOOL bDoubleRef=(pToken->GetType()==formula::svDoubleRef);


					if(pToken->GetType()==formula::svSingleRef || bDoubleRef)
					{
						ScRange aRange;
						if(bDoubleRef)
						{
							ScComplexRefData aRef( pToken->GetDoubleRef() );
							aRef.CalcAbsIfRel( aPos );
							aRange.aStart.Set( aRef.Ref1.nCol, aRef.Ref1.nRow, aRef.Ref1.nTab );
							aRange.aEnd.Set( aRef.Ref2.nCol, aRef.Ref2.nRow, aRef.Ref2.nTab );
						}
						else
						{
							ScSingleRefData aRef( pToken->GetSingleRef() );
							aRef.CalcAbsIfRel( aPos );
							aRange.aStart.Set( aRef.nCol, aRef.nRow, aRef.nTab );
							aRange.aEnd = aRange.aStart;
						}
						ColorData aColName=ScRangeFindList::GetColorName(nIndex++);
						pTabViewShell->AddHighlightRange(aRange, aColName);
					}

					pToken = static_cast<const ScToken*>(pScTokA->GetNextReference());
				}
			}
			if(pScTokA!=NULL) delete pScTokA;
		}
	}
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::HideReference( BOOL bDoneRefMode )
{
	ScViewData* pViewData=ScDocShell::GetViewData();

    if( pViewData && /*!pRefEdit &&*/ bHighLightRef && bEnableColorRef)
	{
		ScTabViewShell*	pTabViewShell=pViewData->GetViewShell();

		if(pTabViewShell!=NULL)
		{
			//	bDoneRefMode is FALSE when called from before SetReference.
			//	In that case, RefMode was just started and must not be ended now.

			if ( bDoneRefMode )
				pTabViewShell->DoneRefMode( FALSE );
			pTabViewShell->ClearHighlightRanges();
		}
		bHighLightRef=FALSE;
	}
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ShowReference( const XubString& rStr )
{
    if( /*!pRefEdit &&*/ bEnableColorRef )
	{
		if(	rStr.Search('(')!=STRING_NOTFOUND ||
			rStr.Search('+')!=STRING_NOTFOUND ||
			rStr.Search('*')!=STRING_NOTFOUND ||
			rStr.Search('-')!=STRING_NOTFOUND ||
			rStr.Search('/')!=STRING_NOTFOUND ||
			rStr.Search('&')!=STRING_NOTFOUND ||
			rStr.Search('<')!=STRING_NOTFOUND ||
			rStr.Search('>')!=STRING_NOTFOUND ||
			rStr.Search('=')!=STRING_NOTFOUND ||
			rStr.Search('^')!=STRING_NOTFOUND)
		{
			ShowFormulaReference(rStr);
		}
		else
		{
			ShowSimpleReference(rStr);
		}
	}
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ReleaseFocus( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
    if( !pRefEdit && pEdit )
    {
        m_pDlg->RefInputStart( pEdit, pButton );
//        if( pRefEdit )
//            pRefEdit->SilentGrabFocus();
    }

    ScTabViewShell* pViewShell = ScTabViewShell::GetActiveViewShell();
    if( pViewShell )
    {
        pViewShell->ActiveGrabFocus();
        if( pRefEdit )
        {
            const ScViewData* pViewData = pViewShell->GetViewData();
            ScDocument* pDoc = pViewData->GetDocument();
			ScRangeList aRangeList;
            if( ParseWithNames( aRangeList, pRefEdit->GetText(), pDoc ) )
			{
                const ScRange* pRange = aRangeList.GetObject( 0 );
                if( pRange )
                {
                    pViewShell->SetTabNo( pRange->aStart.Tab() );
                    pViewShell->MoveCursorAbs(  pRange->aStart.Col(),
                        pRange->aStart.Row(), SC_FOLLOW_JUMP, FALSE, FALSE );
                    pViewShell->MoveCursorAbs( pRange->aEnd.Col(),
                        pRange->aEnd.Row(), SC_FOLLOW_JUMP, TRUE, FALSE );
                    m_pDlg->SetReference( *pRange, pDoc );
                }
            }
        }
    }
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::Init()
{
    ScViewData* pViewData=ScDocShell::GetViewData();    //! use pScViewShell?
	if ( pViewData )
	{
		ScDocument* pDoc = pViewData->GetDocument();
		SCCOL nCol = pViewData->GetCurX();
		SCROW nRow = pViewData->GetCurY();
		SCTAB nTab = pViewData->GetTabNo();
		ScAddress aCursorPos( nCol, nRow, nTab );

		String rStrExp;
		pRefCell.reset( new ScFormulaCell( pDoc, aCursorPos, rStrExp ) );
		pRefComp.reset( new ScCompiler( pDoc, aCursorPos) );
        pRefComp->SetGrammar( pDoc->GetGrammar() );
		pRefComp->SetCompileForFAP(TRUE);

        nRefTab = nTab;
	} // if ( pViewData )
}
// -----------------------------------------------------------------------------
IMPL_LINK( ScFormulaReferenceHelper, AccelSelectHdl, Accelerator *, pSelAccel )
{
    if ( !pSelAccel )
		return 0;

    switch ( pSelAccel->GetCurKeyCode().GetCode() )
	{
		case KEY_RETURN:
		case KEY_ESCAPE:
            if( pRefEdit )
                pRefEdit->GrabFocus();
            m_pDlg->RefInputDone( TRUE );
        break;
	}
	return TRUE;
}
//----------------------------------------------------------------------------
void ScFormulaReferenceHelper::RefInputDone( BOOL bForced )
{
	if (pRefEdit && (bForced || !pRefBtn))
	{
		if (bAccInserted)			// Accelerator wieder abschalten
		{
			Application::RemoveAccel( pAccel.get() );
			bAccInserted = FALSE;
		}

		// Fenstertitel anpassen
		m_pWindow->SetText(sOldDialogText);

		// Fenster wieder gross
		m_pWindow->SetOutputSizePixel(aOldDialogSize);

		// pEditCell an alte Position
		pRefEdit->SetPosSizePixel(aOldEditPos, aOldEditSize);

        // set button position and image
        if( pRefBtn )
        {
            pRefBtn->SetPosPixel( aOldButtonPos );
            pRefBtn->SetStartImage();
        }

		// Alle anderen: Show();
		USHORT nChildren = m_pWindow->GetChildCount();
		for ( USHORT i = 0; i < nChildren; i++ )
			if (pHiddenMarks[i])
			{
				m_pWindow->GetChild(i)->GetWindow( WINDOW_CLIENT )->Show();
			}
		delete [] pHiddenMarks;

		pRefEdit = NULL;
        pRefBtn = NULL;
	}
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::RefInputStart( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
	if (!pRefEdit)
	{
		pRefEdit = pEdit;
		pRefBtn  = pButton;

		// Neuen Fenstertitel basteln
		String sNewDialogText;
		sOldDialogText = m_pWindow->GetText();
		sNewDialogText  = sOldDialogText;
		sNewDialogText.AppendAscii(RTL_CONSTASCII_STRINGPARAM( ": " ));

		// Alle Elemente ausser EditCell und Button verstecken
		USHORT nChildren = m_pWindow->GetChildCount();
		pHiddenMarks = new BOOL [nChildren];
		for (USHORT i = 0; i < nChildren; i++)
		{
			pHiddenMarks[i] = FALSE;
			Window* pWin = m_pWindow->GetChild(i);
			pWin = pWin->GetWindow( WINDOW_CLIENT );
			if (pWin == (Window*)pRefEdit)
			{
				sNewDialogText += m_pWindow->GetChild(i-1)->GetWindow( WINDOW_CLIENT )->GetText();
			}
			else if (pWin == (Window*)pRefBtn)
				;	// do nothing
			else if (pWin->IsVisible())
			{
				pHiddenMarks[i] = TRUE;
				pWin->Hide();
			}
		}

		// Alte Daten merken
		aOldDialogSize = m_pWindow->GetOutputSizePixel();
		aOldEditPos = pRefEdit->GetPosPixel();
		aOldEditSize = pRefEdit->GetSizePixel();
		if (pRefBtn)
			aOldButtonPos = pRefBtn->GetPosPixel();

		// Edit-Feld verschieben und anpassen
		Size aNewDlgSize(aOldDialogSize.Width(), aOldEditSize.Height());
		Size aNewEditSize(aNewDlgSize);
        long nOffset = 0;
		if (pRefBtn)
		{
			aNewEditSize.Width() -= pRefBtn->GetSizePixel().Width();
			aNewEditSize.Width() -= aOldButtonPos.X() - (aOldEditPos.X()+aOldEditSize.Width());

            long nHeight = pRefBtn->GetSizePixel().Height();
            if ( nHeight > aOldEditSize.Height() )
            {
                aNewDlgSize.Height() = nHeight;
                nOffset = (nHeight-aOldEditSize.Height()) / 2;
            }
            aNewEditSize.Width() -= nOffset;
		}
		pRefEdit->SetPosSizePixel(Point(nOffset, nOffset), aNewEditSize);

        // set button position and image
        if( pRefBtn )
        {
            pRefBtn->SetPosPixel( Point( aOldDialogSize.Width() - pRefBtn->GetSizePixel().Width(), 0 ) );
            pRefBtn->SetEndImage();
        }

		// Fenster verkleinern
		m_pWindow->SetOutputSizePixel(aNewDlgSize);

		// Fenstertitel anpassen
		m_pWindow->SetText( MnemonicGenerator::EraseAllMnemonicChars( sNewDialogText ) );

//        if ( pButton )      // ueber den Button: Enter und Escape abfangen
//        {
			if (!pAccel.get())
			{
				pAccel.reset( new Accelerator );
				pAccel->InsertItem( 1, KeyCode( KEY_RETURN ) );
				pAccel->InsertItem( 2, KeyCode( KEY_ESCAPE ) );
				pAccel->SetSelectHdl( LINK( this, ScFormulaReferenceHelper, AccelSelectHdl ) );
			}
			Application::InsertAccel( pAccel.get() );
			bAccInserted = TRUE;
//        }
	}
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ToggleCollapsed( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
    if( pEdit )
    {
        if( pRefEdit == pEdit )                 // is this the active ref edit field?
        {
            pRefEdit->GrabFocus();              // before RefInputDone()
            m_pDlg->RefInputDone( TRUE );               // finish ref input
        }
        else
        {
            m_pDlg->RefInputDone( TRUE );               // another active ref edit?
            m_pDlg->RefInputStart( pEdit, pButton );    // start ref input
            // pRefEdit might differ from pEdit after RefInputStart() (i.e. ScFormulaDlg)
            if( pRefEdit )
                pRefEdit->GrabFocus();
        }
    }
}
// -----------------------------------------------------------------------------
BOOL ScFormulaReferenceHelper::DoClose( USHORT nId )
{
	SfxApplication* pSfxApp = SFX_APP();

	SetDispatcherLock( FALSE );			//! here and in dtor ?

	SfxViewFrame* pViewFrm = SfxViewFrame::Current();
	if ( pViewFrm && pViewFrm->HasChildWindow(FID_INPUTLINE_STATUS) )
	{
		//	Die Eingabezeile wird per ToolBox::Disable disabled, muss darum auch
		//	per ToolBox::Enable wieder aktiviert werden (vor dem Enable des AppWindow),
		//	damit die Buttons auch wieder enabled gezeichnet werden.
		SfxChildWindow* pChild = pViewFrm->GetChildWindow(FID_INPUTLINE_STATUS);
		if (pChild)
		{
			ScInputWindow* pWin = (ScInputWindow*)pChild->GetWindow();
			pWin->Enable();
		}
	}

	// find parent view frame to close dialog
	SfxViewFrame* pMyViewFrm = NULL;
	if ( m_pBindings )
	{
		SfxDispatcher* pMyDisp = m_pBindings->GetDispatcher();
		if (pMyDisp)
			pMyViewFrm = pMyDisp->GetFrame();
	}
	SC_MOD()->SetRefDialog( nId, FALSE, pMyViewFrm );

	pSfxApp->Broadcast( SfxSimpleHint( FID_KILLEDITVIEW ) );

    ScTabViewShell* pScViewShell = ScTabViewShell::GetActiveViewShell();
	if ( pScViewShell )
		pScViewShell->UpdateInputHandler(TRUE);

	return TRUE;
}
void ScFormulaReferenceHelper::SetDispatcherLock( BOOL bLock )
{
	//	lock / unlock only the dispatchers of Calc documents

	TypeId aType(TYPE(ScDocShell));
	ScDocShell* pDocShell = (ScDocShell*)SfxObjectShell::GetFirst(&aType);
	while( pDocShell )
	{
		SfxViewFrame* pFrame = SfxViewFrame::GetFirst( pDocShell );
		while( pFrame )
		{
			SfxDispatcher* pDisp = pFrame->GetDispatcher();
			if (pDisp)
				pDisp->Lock( bLock );

			pFrame = SfxViewFrame::GetNext( *pFrame, pDocShell );
		}
		pDocShell = (ScDocShell*)SfxObjectShell::GetNext(*pDocShell, &aType);
	}

	//	if a new view is created while the dialog is open,
	//	that view's dispatcher is locked when trying to create the dialog
	//	for that view (ScTabViewShell::CreateRefDialog)
}
// -----------------------------------------------------------------------------
void ScFormulaReferenceHelper::ViewShellChanged(ScTabViewShell* /* pScViewShell */)
{
	enableInput( FALSE );

	EnableSpreadsheets();
}
void ScFormulaReferenceHelper::EnableSpreadsheets(BOOL bFlag, BOOL bChilds)
{
	TypeId aType(TYPE(ScDocShell));
	ScDocShell* pDocShell = (ScDocShell*)SfxObjectShell::GetFirst(&aType);
	while( pDocShell )
	{
		SfxViewFrame* pFrame = SfxViewFrame::GetFirst( pDocShell );
		while( pFrame )
		{
			//	#71577# enable everything except InPlace, including bean frames
            if ( !pFrame->GetFrame()->IsInPlace() )
			{
				SfxViewShell* p = pFrame->GetViewShell();
				ScTabViewShell* pViewSh = PTR_CAST(ScTabViewShell,p);
				if(pViewSh!=NULL)
				{
					Window *pWin=pViewSh->GetWindow();
					if(pWin)
					{
						Window *pParent=pWin->GetParent();
						if(pParent)
						{
							pParent->EnableInput(bFlag,FALSE);
							if(bChilds)
								pViewSh->EnableRefInput(bFlag);
						}
					}
				}
			}
			pFrame = SfxViewFrame::GetNext( *pFrame, pDocShell );
		}

		pDocShell = (ScDocShell*)SfxObjectShell::GetNext(*pDocShell, &aType);
	}
}

//----------------------------------------------------------------------------



void lcl_InvalidateWindows()
{
	TypeId aType(TYPE(ScDocShell));
	ScDocShell* pDocShell = (ScDocShell*)SfxObjectShell::GetFirst(&aType);
	while( pDocShell )
	{
		SfxViewFrame* pFrame = SfxViewFrame::GetFirst( pDocShell );
		while( pFrame )
		{
			//	#71577# enable everything except InPlace, including bean frames
            if ( !pFrame->GetFrame()->IsInPlace() )
			{
				SfxViewShell* p = pFrame->GetViewShell();
				ScTabViewShell* pViewSh = PTR_CAST(ScTabViewShell,p);
				if(pViewSh!=NULL)
				{
					Window *pWin=pViewSh->GetWindow();
					if(pWin)
					{
						Window *pParent=pWin->GetParent();
						if(pParent)
							pParent->Invalidate();
					}
				}
			}
			pFrame = SfxViewFrame::GetNext( *pFrame, pDocShell );
		}

		pDocShell = (ScDocShell*)SfxObjectShell::GetNext(*pDocShell, &aType);
	}
}
//----------------------------------------------------------------------------

void lcl_HideAllReferences()
{
	TypeId aScType = TYPE(ScTabViewShell);
	SfxViewShell* pSh = SfxViewShell::GetFirst( &aScType );
	while ( pSh )
	{
		((ScTabViewShell*)pSh)->ClearHighlightRanges();
		pSh = SfxViewShell::GetNext( *pSh, &aScType );
	}
}

//============================================================================
//	class ScAnyRefDlg
//----------------------------------------------------------------------------

ScAnyRefDlg::ScAnyRefDlg( SfxBindings* pB, SfxChildWindow* pCW,
						  Window* pParent, USHORT nResId)
	:	SfxModelessDialog ( pB, pCW, pParent, ScResId( nResId ) ),
        m_aHelper(this,pB),
		pMyBindings( pB ),
        pActiveWin(NULL)
{
    m_aHelper.SetWindow(this);
	if(GetHelpId()==0)				//Hack, da im SfxModelessDialog die HelpId
		SetHelpId(GetUniqueId());	//fuer einen ModelessDialog entfernt und
									//in eine UniqueId gewandelt wird, machen
									//wir das an dieser Stelle rueckgaengig.
	aTimer.SetTimeout(200);
	aTimer.SetTimeoutHdl(LINK( this, ScAnyRefDlg, UpdateFocusHdl));

	SC_MOD()->InputEnterHandler();
//    ScTabViewShell* pScViewShell = ScTabViewShell::GetActiveViewShell();

    ScTabViewShell* pScViewShell = NULL;

	// title has to be from the view that opened the dialog,
	// even if it's not the current view

	SfxObjectShell* pParentDoc = NULL;
	if ( pMyBindings )
	{
		SfxDispatcher* pMyDisp = pMyBindings->GetDispatcher();
		if (pMyDisp)
		{
			SfxViewFrame* pMyViewFrm = pMyDisp->GetFrame();
			if (pMyViewFrm)
            {
                pScViewShell = PTR_CAST( ScTabViewShell, pMyViewFrm->GetViewShell() );
                if( pScViewShell )
		            pScViewShell->UpdateInputHandler(TRUE);
				pParentDoc = pMyViewFrm->GetObjectShell();
            }
		}
	}
	if ( !pParentDoc && pScViewShell )					// use current only if above fails
		pParentDoc = pScViewShell->GetObjectShell();
	if ( pParentDoc )
		aDocName = pParentDoc->GetTitle();

    ScInputHandler*	pInputHdl = SC_MOD()->GetInputHdl(pScViewShell);

	DBG_ASSERT( pInputHdl, "Missing input handler :-/" );

	if ( pInputHdl )
		pInputHdl->NotifyChange( NULL );

	m_aHelper.enableInput( FALSE );

	m_aHelper.EnableSpreadsheets();

    m_aHelper.Init();

	m_aHelper.SetDispatcherLock( TRUE );
	//@Test
	//SFX_APPWINDOW->Disable(TRUE);	  //@BugID 54702
}

//----------------------------------------------------------------------------

ScAnyRefDlg::~ScAnyRefDlg()
{
	lcl_HideAllReferences();

	SetModalInputMode(FALSE);
	SetDispatcherLock( FALSE );			//! here and in DoClose ?

    ScTabViewShell* pScViewShell = ScTabViewShell::GetActiveViewShell();
    if( pScViewShell )
		pScViewShell->UpdateInputHandler(TRUE);

	//SFX_APPWINDOW->Enable(TRUE,TRUE);
	lcl_InvalidateWindows();
}

//----------------------------------------------------------------------------

SfxBindings& ScAnyRefDlg::GetBindings()
{
	//!	SfxModelessDialog should allow access to pBindings pointer

	return *pMyBindings;
}

//----------------------------------------------------------------------------

void ScAnyRefDlg::SwitchToDocument()
{
	ScTabViewShell* pCurrent = ScTabViewShell::GetActiveViewShell();
	if (pCurrent)
	{
		SfxObjectShell* pObjSh = pCurrent->GetObjectShell();
		if ( pObjSh && pObjSh->GetTitle() == aDocName )
		{
			//	right document already visible -> nothing to do
			return;
		}
	}

	TypeId aScType = TYPE(ScTabViewShell);
	SfxViewShell* pSh = SfxViewShell::GetFirst( &aScType );
	while ( pSh )
	{
		SfxObjectShell* pObjSh = pSh->GetObjectShell();
		if ( pObjSh && pObjSh->GetTitle() == aDocName )
		{
			//	switch to first TabViewShell for document
			((ScTabViewShell*)pSh)->SetActive();
			return;
		}
		pSh = SfxViewShell::GetNext( *pSh, &aScType );
	}
}

//----------------------------------------------------------------------------

BOOL ScAnyRefDlg::IsDocAllowed(SfxObjectShell* pDocSh) const	// pDocSh may be 0
{
	//	default: allow only same document (overridden in function dialog)
	String aCmpName;
	if ( pDocSh )
		aCmpName = pDocSh->GetTitle();

	//	if aDocName isn't initialized, allow
	return ( aDocName.Len() == 0 || aDocName == aCmpName );
}

//----------------------------------------------------------------------------

BOOL __EXPORT ScAnyRefDlg::IsRefInputMode() const
{
	return IsVisible(); // nur wer sichtbar ist kann auch Referenzen bekommen
}

//----------------------------------------------------------------------------

BOOL __EXPORT ScAnyRefDlg::DoClose( USHORT nId )
{
    m_aHelper.DoClose(nId);
	return TRUE;
}

void ScAnyRefDlg::SetDispatcherLock( BOOL bLock )
{
	m_aHelper.SetDispatcherLock( bLock );
}

//----------------------------------------------------------------------------

void ScAnyRefDlg::ViewShellChanged(ScTabViewShell*  pScViewShell )
{
	m_aHelper.ViewShellChanged(pScViewShell);
}

//----------------------------------------------------------------------------

void ScAnyRefDlg::AddRefEntry()
{
	//	wenn nicht ueberladen, gibt es keine Mehrfach-Referenzen
}

//----------------------------------------------------------------------------

BOOL __EXPORT ScAnyRefDlg::IsTableLocked() const
{
	// per Default kann bei Referenzeingabe auch die Tabelle umgeschaltet werden

	return FALSE;
}

//----------------------------------------------------------------------------
//
//	RefInputStart/Done: Zoom-In (AutoHide) auf einzelnes Feld
//	(per Button oder Bewegung)
//
//----------------------------------------------------------------------------

void ScAnyRefDlg::RefInputStart( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
    m_aHelper.RefInputStart( pEdit, pButton );
}


void ScAnyRefDlg::ToggleCollapsed( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
    m_aHelper.ToggleCollapsed( pEdit, pButton );
}
long ScAnyRefDlg::PreNotify( NotifyEvent& rNEvt )
{
	USHORT nSwitch=rNEvt.GetType();
	if(nSwitch==EVENT_GETFOCUS)
	{
		pActiveWin=rNEvt.GetWindow();
	}
	return SfxModelessDialog::PreNotify(rNEvt);
}

void ScAnyRefDlg::StateChanged( StateChangedType nStateChange )
{
	SfxModelessDialog::StateChanged( nStateChange );

	if(nStateChange == STATE_CHANGE_VISIBLE)
	{
		if(IsVisible())
		{
			m_aHelper.enableInput( FALSE );
			m_aHelper.EnableSpreadsheets();
			m_aHelper.SetDispatcherLock( TRUE );
			aTimer.Start();
		}
		else
		{
			m_aHelper.enableInput( TRUE );
			m_aHelper.SetDispatcherLock( FALSE );			//! here and in DoClose ?
		}
	}
}

IMPL_LINK( ScAnyRefDlg, UpdateFocusHdl, Timer*, EMPTYARG )
{
	if (pActiveWin)
	{
		pActiveWin->GrabFocus();
	}
	return 0;
}
// -----------------------------------------------------------------------------
bool ScAnyRefDlg::ParseWithNames( ScRangeList& rRanges, const String& rStr, ScDocument* pDoc )
{
    return m_aHelper.ParseWithNames( rRanges, rStr, pDoc );
}
// -----------------------------------------------------------------------------
void ScAnyRefDlg::HideReference( BOOL bDoneRefMode )
{
    m_aHelper.HideReference( bDoneRefMode );
}
// -----------------------------------------------------------------------------
void ScAnyRefDlg::ShowReference( const XubString& rStr )
{
    m_aHelper.ShowReference( rStr );
}
// -----------------------------------------------------------------------------
void ScAnyRefDlg::ReleaseFocus( formula::RefEdit* pEdit, formula::RefButton* pButton )
{
    m_aHelper.ReleaseFocus( pEdit,pButton );
}
//----------------------------------------------------------------------------
void ScAnyRefDlg::RefInputDone( BOOL bForced )
{
    m_aHelper.RefInputDone( bForced );
}

