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



//------------------------------------------------------------------

// INCLUDE ---------------------------------------------------------------
#include "scitems.hxx"
#include <vcl/msgbox.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/dispatch.hxx>

#include "tabvwsh.hxx"
#include "sc.hrc"
#include "globstr.hrc"
#include "global.hxx"
#include "scmod.hxx"
#include "docsh.hxx"
#include "document.hxx"
#include "uiitems.hxx"
#include "pivot.hxx"
#include "namedlg.hxx"
#include "solvrdlg.hxx"
#include "optsolver.hxx"
#include "tabopdlg.hxx"
#include "autoform.hxx"         // Core
#include "autofmt.hxx"          // Dialog
#include "consdlg.hxx"
//CHINA001 #include "sortdlg.hxx"
#include "filtdlg.hxx"
#include "dbnamdlg.hxx"
#include "pvlaydlg.hxx"
#include "areasdlg.hxx"
#include "condfrmt.hxx"
#include "rangeutl.hxx"
#include "crnrdlg.hxx"
#include "formula.hxx"
#include "cell.hxx"             // Input Status Edit-Zellen
#include "acredlin.hxx"
#include "highred.hxx"
#include "simpref.hxx"
#include "funcdesc.hxx"
#include "dpobject.hxx"

//------------------------------------------------------------------

void ScTabViewShell::SetCurRefDlgId( USHORT nNew )
{
	//	CurRefDlgId is stored in ScModule to find if a ref dialog is open,
	//	and in the view to identify the view that has opened the dialog
	nCurRefDlgId = nNew;
}

SfxModelessDialog* ScTabViewShell::CreateRefDialog(
						SfxBindings* pB, SfxChildWindow* pCW, SfxChildWinInfo* pInfo,
						Window* pParent, USHORT nSlotId )
{
	//	Dialog nur aufmachen, wenn ueber ScModule::SetRefDialog gerufen, damit
	//	z.B. nach einem Absturz offene Ref-Dialoge nicht wiederkommen (#42341#).

	if ( SC_MOD()->GetCurRefDlgId() != nSlotId )
		return NULL;

	if ( nCurRefDlgId != nSlotId )
	{
		//	the dialog has been opened in a different view
		//	-> lock the dispatcher for this view (modal mode)

		GetViewData()->GetDispatcher().Lock( TRUE );	// lock is reset when closing dialog
		return NULL;
	}

	SfxModelessDialog* pResult = 0;

	if(pCW)
		pCW->SetHideNotDelete(TRUE);

	switch( nSlotId )
	{
		case FID_DEFINE_NAME:
		pResult = new ScNameDlg( pB, pCW, pParent, GetViewData(),
								 ScAddress( GetViewData()->GetCurX(),
											GetViewData()->GetCurY(),
											GetViewData()->GetTabNo() ) );
		break;

		case SID_DEFINE_COLROWNAMERANGES:
		{
			pResult = new ScColRowNameRangesDlg( pB, pCW, pParent, GetViewData() );
		}
		break;

		case SID_DEFINE_DBNAME:
		{
			//	wenn auf einem bestehenden Bereich aufgerufen, den markieren
			GetDBData( TRUE, SC_DB_OLD );
			const ScMarkData& rMark = GetViewData()->GetMarkData();
			if ( !rMark.IsMarked() && !rMark.IsMultiMarked() )
				MarkDataArea( FALSE );

			pResult = new ScDbNameDlg( pB, pCW, pParent, GetViewData() );
		}
		break;

		case SID_SPECIAL_FILTER:
		{
			ScQueryParam	aQueryParam;
			SfxItemSet		aArgSet( GetPool(),
									 SCITEM_QUERYDATA,
									 SCITEM_QUERYDATA );

			ScDBData* pDBData = GetDBData();
			pDBData->GetQueryParam( aQueryParam );

			ScQueryItem aItem( SCITEM_QUERYDATA, GetViewData(), &aQueryParam );
			ScRange aAdvSource;
			if (pDBData->GetAdvancedQuerySource(aAdvSource))
				aItem.SetAdvancedQuerySource( &aAdvSource );

			aArgSet.Put( aItem );

			// aktuelle Tabelle merken (wg. RefInput im Dialog)
			GetViewData()->SetRefTabNo( GetViewData()->GetTabNo() );

			pResult = new ScSpecialFilterDlg( pB, pCW, pParent, aArgSet );
		}
		break;

		case SID_FILTER:
		{

			ScQueryParam	aQueryParam;
			SfxItemSet		aArgSet( GetPool(),
									 SCITEM_QUERYDATA,
									 SCITEM_QUERYDATA );

			ScDBData* pDBData = GetDBData();
			pDBData->GetQueryParam( aQueryParam );

			aArgSet.Put( ScQueryItem( SCITEM_QUERYDATA,
									  GetViewData(),
									  &aQueryParam ) );

			// aktuelle Tabelle merken (wg. RefInput im Dialog)
			GetViewData()->SetRefTabNo( GetViewData()->GetTabNo() );

			pResult = new ScFilterDlg( pB, pCW, pParent, aArgSet );
		}
		break;

		case SID_OPENDLG_TABOP:
		{
			ScViewData*  pViewData	= GetViewData();
			ScRefAddress  aCurPos	( pViewData->GetCurX(),
									  pViewData->GetCurY(),
									  pViewData->GetTabNo(),
									  FALSE, FALSE, FALSE );

			pResult = new ScTabOpDlg( pB, pCW, pParent, pViewData->GetDocument(), aCurPos );
		}
		break;

		case SID_OPENDLG_SOLVE:
		{
			ScViewData*  pViewData	= GetViewData();
            ScAddress aCurPos(  pViewData->GetCurX(),
                                pViewData->GetCurY(),
                                pViewData->GetTabNo());
			pResult = new ScSolverDlg( pB, pCW, pParent, pViewData->GetDocument(), aCurPos );
		}
		break;

        case SID_OPENDLG_OPTSOLVER:
        {
            ScViewData* pViewData = GetViewData();
            ScAddress aCurPos( pViewData->GetCurX(), pViewData->GetCurY(), pViewData->GetTabNo());
            pResult = new ScOptSolverDlg( pB, pCW, pParent, pViewData->GetDocShell(), aCurPos );
        }
        break;

		case SID_OPENDLG_PIVOTTABLE:
		{
			//	all settings must be in pDialogDPObject

            if( pDialogDPObject )
            {
                GetViewData()->SetRefTabNo( GetViewData()->GetTabNo() );
                pResult = new ScDPLayoutDlg( pB, pCW, pParent, *pDialogDPObject );
            }
		}
		break;

		case SID_OPENDLG_EDIT_PRINTAREA:
		{
			pResult = new ScPrintAreasDlg( pB, pCW, pParent );
		}
		break;

		case SID_OPENDLG_CONDFRMT:
		{
			ScViewData* pViewData = GetViewData();

			ScDocument* pDoc = pViewData->GetDocument();
			const ScConditionalFormat* pForm = pDoc->GetCondFormat(
				pViewData->GetCurX(), pViewData->GetCurY(), pViewData->GetTabNo() );

			// aktuelle Tabelle merken (wg. RefInput im Dialog)
			pViewData->SetRefTabNo( pViewData->GetTabNo() );

			pResult = new ScConditionalFormatDlg( pB, pCW, pParent, pDoc, pForm );
		}
		break;

		case SID_OPENDLG_FUNCTION:
		{
			//	Dialog schaut selber, was in der Zelle steht

			pResult = new ScFormulaDlg( pB, pCW, pParent, GetViewData(),ScGlobal::GetStarCalcFunctionMgr() );
		}
		break;

		case FID_CHG_SHOW:
		{
			//	Dialog schaut selber, was in der Zelle steht

			pResult = new ScHighlightChgDlg( pB, pCW, pParent, GetViewData() );
		}
		break;

		case WID_SIMPLE_REF:
		{
			//	Dialog schaut selber, was in der Zelle steht

            ScViewData* pViewData = GetViewData();
            pViewData->SetRefTabNo( pViewData->GetTabNo() );
            pResult = new ScSimpleRefDlg( pB, pCW, pParent, pViewData );
		}
		break;


		default:
		DBG_ERROR( "ScTabViewShell::CreateRefDialog: unbekannte ID" );
		break;
	}

	if (pResult)
	{
		//	Die Dialoge gehen immer mit eingeklapptem Zusaetze-Button auf,
		//	darum muss die Groesse ueber das Initialize gerettet werden
		//	(oder den Zusaetze-Status mit speichern !!!)

		Size aSize = pResult->GetSizePixel();
		pResult->Initialize( pInfo );
		pResult->SetSizePixel(aSize);
	}

	return pResult;
}



