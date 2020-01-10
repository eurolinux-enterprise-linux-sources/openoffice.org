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

// System - Includes ---------------------------------------------------------



// INCLUDE -------------------------------------------------------------------
#include <rangelst.hxx>
#include <sfx2/dispatch.hxx>
#include <vcl/waitobj.hxx>

#include "uiitems.hxx"
#include "dbcolect.hxx"
#include "reffact.hxx"
#include "viewdata.hxx"
#include "document.hxx"
#include "docsh.hxx"
#include "scresid.hxx"

#include "foptmgr.hxx"

#include "globstr.hrc"
#include "filter.hrc"

#define _FILTDLG_CXX
#include "filtdlg.hxx"
#undef _FILTDLG_CXX
#include <vcl/msgbox.hxx>

// DEFINE --------------------------------------------------------------------

#define ERRORBOX(rid)	ErrorBox( this, WinBits( WB_OK|WB_DEF_OK), \
				 				  ScGlobal::GetRscString(rid) ).Execute()


//============================================================================
//	class ScFilterDlg

//----------------------------------------------------------------------------

ScFilterDlg::ScFilterDlg( SfxBindings* pB, SfxChildWindow* pCW, Window* pParent,
						  const SfxItemSet&	rArgSet )

	:	ScAnyRefDlg	( pB, pCW, pParent, RID_SCDLG_FILTER ),
		//
        aFlCriteria     ( this, ScResId( FL_CRITERIA ) ),
		aLbConnect1		( this, ScResId( LB_OP1 ) ),
		aLbField1		( this, ScResId( LB_FIELD1 ) ),
		aLbCond1		( this, ScResId( LB_COND1 ) ),
		aEdVal1			( this, ScResId( ED_VAL1 ) ),
		aLbConnect2		( this, ScResId( LB_OP2 ) ),
		aLbField2		( this, ScResId( LB_FIELD2 ) ),
		aLbCond2		( this, ScResId( LB_COND2 ) ),
		aEdVal2			( this, ScResId( ED_VAL2 ) ),
		aLbConnect3		( this, ScResId( LB_OP3 ) ),
		aLbField3		( this, ScResId( LB_FIELD3 ) ),
		aLbCond3		( this, ScResId( LB_COND3 ) ),
		aEdVal3			( this, ScResId( ED_VAL3 ) ),
		aLbConnect4		( this, ScResId( LB_OP4 ) ),
		aLbField4		( this, ScResId( LB_FIELD4 ) ),
		aLbCond4		( this, ScResId( LB_COND4 ) ),
		aEdVal4			( this, ScResId( ED_VAL4 ) ),
		aFtConnect		( this, ScResId( FT_OP ) ),
		aFtField		( this, ScResId( FT_FIELD ) ),
		aFtCond			( this, ScResId( FT_COND ) ),
		aFtVal			( this, ScResId( FT_VAL ) ),
		aFlSeparator    ( this, ScResId( FL_SEPARATOR ) ),
		aScrollBar      ( this, ScResId( LB_SCROLL ) ),
        aFlOptions      ( this, ScResId( FL_OPTIONS ) ),
        aBtnMore        ( this, ScResId( BTN_MORE ) ),
        aBtnHelp        ( this, ScResId( BTN_HELP ) ),
        aBtnOk          ( this, ScResId( BTN_OK ) ),
        aBtnCancel      ( this, ScResId( BTN_CANCEL ) ),
		_INIT_COMMON_FILTER_RSCOBJS
		aStrEmpty		( ScResId( SCSTR_EMPTY ) ),
		aStrNotEmpty	( ScResId( SCSTR_NOTEMPTY ) ),
		aStrRow			( ScResId( SCSTR_ROW ) ),
		aStrColumn		( ScResId( SCSTR_COLUMN ) ),
		//
		pOptionsMgr		( NULL ),
		nWhichQuery		( rArgSet.GetPool()->GetWhich( SID_QUERY ) ),
		theQueryData	( ((const ScQueryItem&)
						   rArgSet.Get( nWhichQuery )).GetQueryData() ),
		pOutItem		( NULL ),
		pViewData		( NULL ),
		pDoc			( NULL ),
		nSrcTab			( 0 ),
		nFieldCount		( 0 ),
		bRefInputMode	( FALSE ),
		pTimer			( NULL )
{
	for (USHORT i=0; i<=MAXCOL; i++)
		pEntryLists[i] = NULL;
	for (SCSIZE i=0;i<MAXQUERY;i++)
	{
		 bRefreshExceptQuery[i]=FALSE; 
	}
	aBtnMore.SetMoreText( String(ScResId( SCSTR_MOREBTN_MOREOPTIONS )) );
	aBtnMore.SetLessText( String(ScResId( SCSTR_MOREBTN_FEWEROPTIONS )) );
	Init( rArgSet );
	FreeResource();

	// Hack: RefInput-Kontrolle
	pTimer = new Timer;
	pTimer->SetTimeout( 50 ); // 50ms warten
	pTimer->SetTimeoutHdl( LINK( this, ScFilterDlg, TimeOutHdl ) );
}


//----------------------------------------------------------------------------

__EXPORT ScFilterDlg::~ScFilterDlg()
{
	for (USHORT i=0; i<=MAXCOL; i++)
		delete pEntryLists[i];

	delete pOptionsMgr;
	delete pOutItem;

	// Hack: RefInput-Kontrolle
	pTimer->Stop();
	delete pTimer;
}


//----------------------------------------------------------------------------

void __EXPORT ScFilterDlg::Init( const SfxItemSet& rArgSet )
{
	const ScQueryItem& rQueryItem = (const ScQueryItem&)
									rArgSet.Get( nWhichQuery );

	aBtnOk.SetClickHdl		( LINK( this, ScFilterDlg, EndDlgHdl ) );
	aBtnCancel.SetClickHdl	( LINK( this, ScFilterDlg, EndDlgHdl ) );
	aBtnMore.SetClickHdl	( LINK( this, ScFilterDlg, MoreClickHdl ) );
	aBtnHeader.SetClickHdl	( LINK( this, ScFilterDlg, CheckBoxHdl ) );
	aBtnCase.SetClickHdl	( LINK( this, ScFilterDlg, CheckBoxHdl ) );
	//
	aLbField1.SetSelectHdl	( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbField2.SetSelectHdl	( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbField3.SetSelectHdl	( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbField4.SetSelectHdl	( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbConnect1.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbConnect2.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbConnect3.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbConnect4.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );

	aLbCond1.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbCond2.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbCond3.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );
	aLbCond4.SetSelectHdl( LINK( this, ScFilterDlg, LbSelectHdl ) );

	pViewData 	= rQueryItem.GetViewData();
	pDoc	  	= pViewData ? pViewData->GetDocument() : NULL;
	nSrcTab		= pViewData ? pViewData->GetTabNo() : static_cast<SCTAB>(0);

	// fuer leichteren Zugriff:
	aFieldLbArr	 [0] = &aLbField1;
	aFieldLbArr	 [1] = &aLbField2;
	aFieldLbArr	 [2] = &aLbField3;
	aFieldLbArr	 [3] = &aLbField4;
	aValueEdArr	 [0] = &aEdVal1;
	aValueEdArr	 [1] = &aEdVal2;
	aValueEdArr	 [2] = &aEdVal3;
	aValueEdArr	 [3] = &aEdVal4;
	aCondLbArr	 [0] = &aLbCond1;
	aCondLbArr	 [1] = &aLbCond2;
	aCondLbArr 	 [2] = &aLbCond3;
	aCondLbArr 	 [3] = &aLbCond4;
	aConnLbArr   [0] = &aLbConnect1;
	aConnLbArr   [1] = &aLbConnect2;
	aConnLbArr   [2] = &aLbConnect3;
	aConnLbArr   [3] = &aLbConnect4;

	// Optionen initialisieren lassen:

	pOptionsMgr	 = new ScFilterOptionsMgr(
							this,
							pViewData,
							theQueryData,
							aBtnMore,
							aBtnCase,
							aBtnRegExp,
							aBtnHeader,
							aBtnUnique,
							aBtnCopyResult,
							aBtnDestPers,
							aLbCopyArea,
							aEdCopyArea,
							aRbCopyArea,
							aFtDbAreaLabel,
							aFtDbArea,
                            aFlOptions,
							aStrNoName,
							aStrUndefined );

	// Feldlisten einlesen und Eintraege selektieren:

	FillFieldLists();

	for ( SCSIZE i=0; i<4; i++ )
	{
		String	aValStr;
		USHORT	nCondPos	 = 0;
		USHORT	nFieldSelPos = 0;

		ScQueryEntry& rEntry = theQueryData.GetEntry(i);
		if ( rEntry.bDoQuery )
		{
			nCondPos	 = (USHORT)rEntry.eOp;
			nFieldSelPos = GetFieldSelPos( static_cast<SCCOL>(rEntry.nField) );
			if ( rEntry.nVal == SC_EMPTYFIELDS && !rEntry.bQueryByString && *rEntry.pStr == EMPTY_STRING )
			{
				aValStr = aStrEmpty;
				aCondLbArr[i]->Disable();
			}
			else if ( rEntry.nVal == SC_NONEMPTYFIELDS && !rEntry.bQueryByString && *rEntry.pStr == EMPTY_STRING )
			{
				aValStr = aStrNotEmpty;
				aCondLbArr[i]->Disable();
			}
			else
				aValStr = *rEntry.pStr;
		}
		else if ( i == 0 )
		{
			nFieldSelPos = GetFieldSelPos( pViewData->GetCurX() );
			rEntry.nField = nFieldSelPos ? (theQueryData.nCol1 +
				static_cast<SCCOL>(nFieldSelPos) - 1) : static_cast<SCCOL>(0);
			rEntry.bDoQuery=TRUE;
			bRefreshExceptQuery[i]=TRUE;

		}
		aFieldLbArr[i]->SelectEntryPos( nFieldSelPos );
		aCondLbArr [i]->SelectEntryPos( nCondPos );
		aValueEdArr[i]->SetText( aValStr );
		aValueEdArr[i]->SetModifyHdl( LINK( this, ScFilterDlg, ValModifyHdl ) );
		UpdateValueList( static_cast<USHORT>(i+1) );
	}

	aScrollBar.SetEndScrollHdl( LINK( this, ScFilterDlg, ScrollHdl ) );
	aScrollBar.SetScrollHdl( LINK( this, ScFilterDlg, ScrollHdl ) );

	aScrollBar.SetRange( Range( 0, 4 ) );
	aScrollBar.SetLineSize( 1 );
	aLbConnect1.Hide(); 
	// Disable/Enable Logik:

	   (aLbField1.GetSelectEntryPos() != 0)
	&& (aLbField2.GetSelectEntryPos() != 0)
		? aLbConnect2.SelectEntryPos( (USHORT)theQueryData.GetEntry(1).eConnect )  
		: aLbConnect2.SetNoSelection();

	   (aLbField2.GetSelectEntryPos() != 0)
	&& (aLbField3.GetSelectEntryPos() != 0)
		? aLbConnect3.SelectEntryPos( (USHORT)theQueryData.GetEntry(2).eConnect )  
		: aLbConnect3.SetNoSelection();

	   (aLbField3.GetSelectEntryPos() != 0)
	&& (aLbField4.GetSelectEntryPos() != 0)
		? aLbConnect4.SelectEntryPos( (USHORT)theQueryData.GetEntry(3).eConnect )  
		: aLbConnect4.SetNoSelection();
	if ( aLbField1.GetSelectEntryPos() == 0 )
	{
		aLbConnect2.Disable(); 
		aLbField2.Disable();
		aLbCond2.Disable();
		aEdVal2.Disable();
	}
	else if ( aLbConnect2.GetSelectEntryCount() == 0 )  
	{
		aLbField2.Disable();
		aLbCond2.Disable();
		aEdVal2.Disable();
	}

	if ( aLbField2.GetSelectEntryPos() == 0 )
	{
		aLbConnect3.Disable();  
		aLbField3.Disable();
		aLbCond3.Disable();
		aEdVal3.Disable();
	}
	else if ( aLbConnect3.GetSelectEntryCount() == 0 ) 
	{
		aLbField3.Disable();
		aLbCond3.Disable();
		aEdVal3.Disable();
	}
	if ( aLbField3.GetSelectEntryPos() == 0 )
	{
		aLbConnect4.Disable(); 
		aLbField4.Disable();
		aLbCond4.Disable();
		aEdVal4.Disable();
	}
	else if ( aLbConnect4.GetSelectEntryCount() == 0 )  
	{
		aLbField4.Disable();
		aLbCond4.Disable();
		aEdVal4.Disable();
	}

	if(pDoc!=NULL &&
		pDoc->GetChangeTrack()!=NULL) aBtnCopyResult.Disable();
	// Modal-Modus einschalten
//	SetDispatcherLock( TRUE );
	//@BugID 54702 Enablen/Disablen nur noch in Basisklasse
//	SFX_APPWINDOW->Disable(FALSE);		//! allgemeine Methode im ScAnyRefDlg
}


//----------------------------------------------------------------------------

BOOL __EXPORT ScFilterDlg::Close()
{
    if (pViewData)
        pViewData->GetDocShell()->CancelAutoDBRange();

	return DoClose( ScFilterDlgWrapper::GetChildWindowId() );
}


//----------------------------------------------------------------------------
// Uebergabe eines mit der Maus selektierten Tabellenbereiches, der dann als
// neue Selektion im Referenz-Edit angezeigt wird.

void ScFilterDlg::SetReference( const ScRange& rRef, ScDocument* pDocP )
{
	if ( bRefInputMode )	// Nur moeglich, wenn im Referenz-Editmodus
	{
		if ( rRef.aStart != rRef.aEnd )
			RefInputStart( &aEdCopyArea );
		String aRefStr;
        rRef.aStart.Format( aRefStr, SCA_ABS_3D, pDocP, pDocP->GetAddressConvention() );
		aEdCopyArea.SetRefString( aRefStr );
	}
}


//----------------------------------------------------------------------------

void ScFilterDlg::SetActive()
{
	if ( bRefInputMode )
	{
		aEdCopyArea.GrabFocus();
		if ( aEdCopyArea.GetModifyHdl().IsSet() )
			((Link&)aEdCopyArea.GetModifyHdl()).Call( &aEdCopyArea );
	}
	else
		GrabFocus();

	RefInputDone();
}

//----------------------------------------------------------------------------

void ScFilterDlg::FillFieldLists()
{
	aLbField1.Clear();
	aLbField2.Clear();
	aLbField3.Clear();
	aLbField4.Clear();
	aLbField1.InsertEntry( aStrNone, 0 );
	aLbField2.InsertEntry( aStrNone, 0 );
	aLbField3.InsertEntry( aStrNone, 0 );
	aLbField4.InsertEntry( aStrNone, 0 );

	if ( pDoc )
	{
		String	aFieldName;
		SCTAB	nTab		= nSrcTab;
		SCCOL	nFirstCol	= theQueryData.nCol1;
		SCROW	nFirstRow	= theQueryData.nRow1;
		SCCOL	nMaxCol		= theQueryData.nCol2;
		SCCOL	col = 0;
		USHORT	i=1;

		for ( col=nFirstCol; col<=nMaxCol; col++ )
		{
			pDoc->GetString( col, nFirstRow, nTab, aFieldName );
			if ( !aBtnHeader.IsChecked() || (aFieldName.Len() == 0) )
			{
				aFieldName  = aStrColumn;
				aFieldName += ' ';
                aFieldName += ScColToAlpha( col );
			}
			aLbField1.InsertEntry( aFieldName, i );
			aLbField2.InsertEntry( aFieldName, i );
			aLbField3.InsertEntry( aFieldName, i );
			aLbField4.InsertEntry( aFieldName, i );
			i++;
		}
		nFieldCount = i;
	}
}


//----------------------------------------------------------------------------

void ScFilterDlg::UpdateValueList( USHORT nList )
{
	if ( pDoc && nList>0 && nList<=4 )
	{
		ComboBox*	pValList		= aValueEdArr[nList-1];
		USHORT		nFieldSelPos	= aFieldLbArr[nList-1]->GetSelectEntryPos();
		USHORT		nListPos		= 0;
		String		aCurValue  		= pValList->GetText();

		pValList->Clear();
		pValList->InsertEntry( aStrNotEmpty, 0 );
		pValList->InsertEntry( aStrEmpty, 1 );
		nListPos = 2;

		if ( nFieldSelPos )
		{
			WaitObject aWaiter( this );		// auch wenn nur die ListBox gefuellt wird

			SCCOL nColumn = theQueryData.nCol1 + static_cast<SCCOL>(nFieldSelPos) - 1;
			if (!pEntryLists[nColumn])
			{
				SCTAB nTab		 = nSrcTab;
				SCROW nFirstRow = theQueryData.nRow1;
				SCROW nLastRow	 = theQueryData.nRow2;

				//	erstmal ohne die erste Zeile

				pEntryLists[nColumn] = new TypedScStrCollection( 128, 128 );
				pEntryLists[nColumn]->SetCaseSensitive( aBtnCase.IsChecked() );
				pDoc->GetFilterEntriesArea( nColumn, nFirstRow+1, nLastRow,
											nTab, *pEntryLists[nColumn] );

				//	Eintrag fuer die erste Zeile
				//!	Eintrag (pHdrEntry) ohne Collection erzeugen?

				nHeaderPos[nColumn] = USHRT_MAX;
				TypedScStrCollection aHdrColl( 1, 1 );
				pDoc->GetFilterEntriesArea( nColumn, nFirstRow, nFirstRow,
											nTab, aHdrColl );
				TypedStrData* pHdrEntry = aHdrColl[0];
				if ( pHdrEntry )
				{
					TypedStrData* pNewEntry = new TypedStrData(*pHdrEntry);
					if ( pEntryLists[nColumn]->Insert( pNewEntry ) )
					{
						nHeaderPos[nColumn] = pEntryLists[nColumn]->IndexOf( pNewEntry );
						DBG_ASSERT( nHeaderPos[nColumn] != USHRT_MAX,
									"Header-Eintrag nicht wiedergefunden" );
					}
					else
						delete pNewEntry;			// war schon drin
				}
			}

			TypedScStrCollection* pColl = pEntryLists[nColumn];
			USHORT nValueCount = pColl->GetCount();
			if ( nValueCount > 0 )
			{
				for ( USHORT i=0; i<nValueCount; i++ )
				{
					pValList->InsertEntry( (*pColl)[i]->GetString(), nListPos );
					nListPos++;
				}
			}
		}
		pValList->SetText( aCurValue );
	}

	UpdateHdrInValueList( nList );
}

void ScFilterDlg::UpdateHdrInValueList( USHORT nList )
{
	//!	GetText / SetText ??

	if ( pDoc && nList>0 && nList<=4 )
	{
		USHORT nFieldSelPos = aFieldLbArr[nList-1]->GetSelectEntryPos();
		if ( nFieldSelPos )
		{
			SCCOL nColumn = theQueryData.nCol1 + static_cast<SCCOL>(nFieldSelPos) - 1;
			if ( pEntryLists[nColumn] )
			{
				USHORT nPos = nHeaderPos[nColumn];
				if ( nPos != USHRT_MAX )
				{
					ComboBox* pValList = aValueEdArr[nList-1];
					USHORT nListPos = nPos + 2;					// nach "leer" und "nicht leer"

					TypedStrData* pHdrEntry = (*pEntryLists[nColumn])[nPos];
					if ( pHdrEntry )
					{
						String aHdrStr = pHdrEntry->GetString();
						BOOL bWasThere = ( pValList->GetEntry(nListPos) == aHdrStr );
						BOOL bInclude = !aBtnHeader.IsChecked();

						if (bInclude)			// Eintrag aufnehmen
						{
							if (!bWasThere)
								pValList->InsertEntry(aHdrStr, nListPos);
						}
						else					// Eintrag weglassen
						{
							if (bWasThere)
								pValList->RemoveEntry(nListPos);
						}
					}
					else
					{
						DBG_ERROR("Eintag in Liste nicht gefunden");
					}
				}
			}
			else
			{
				DBG_ERROR("Spalte noch nicht initialisiert");
			}
		}
	}
}

//----------------------------------------------------------------------------

void ScFilterDlg::ClearValueList( USHORT nList )
{
	if ( nList>0 && nList<=4 )
	{
		ComboBox* pValList = aValueEdArr[nList-1];
		pValList->Clear();
		pValList->InsertEntry( aStrNotEmpty, 0 );
		pValList->InsertEntry( aStrEmpty, 1 );
		pValList->SetText( EMPTY_STRING );
	}
}


//----------------------------------------------------------------------------

USHORT ScFilterDlg::GetFieldSelPos( SCCOL nField )
{
	if ( nField >= theQueryData.nCol1 && nField <= theQueryData.nCol2 )
		return static_cast<USHORT>(nField - theQueryData.nCol1 + 1);
	else
		return 0;
}

//----------------------------------------------------------------------------

ScQueryItem* ScFilterDlg::GetOutputItem()
{
	ScAddress		theCopyPos;
	ScQueryParam 	theParam( theQueryData );
    BOOL            bCopyPosOk = FALSE;

	if ( aBtnCopyResult.IsChecked() )
	{
		String theCopyStr( aEdCopyArea.GetText() );
		xub_StrLen nColonPos = theCopyStr.Search( ':' );

		if ( STRING_NOTFOUND != nColonPos )
			theCopyStr.Erase( nColonPos );

		USHORT nResult = theCopyPos.Parse( theCopyStr, pDoc, pDoc->GetAddressConvention() );
		bCopyPosOk = ( SCA_VALID == (nResult & SCA_VALID) );
	}

	if ( aBtnCopyResult.IsChecked() && bCopyPosOk )
	{
		theParam.bInplace   = FALSE;
		theParam.nDestTab	= theCopyPos.Tab();
		theParam.nDestCol	= theCopyPos.Col();
		theParam.nDestRow	= theCopyPos.Row();
	}
	else
	{
		theParam.bInplace   = TRUE;
		theParam.nDestTab	= 0;
		theParam.nDestCol	= 0;
		theParam.nDestRow	= 0;
	}

	theParam.bHasHeader		= aBtnHeader.IsChecked();
	theParam.bByRow			= TRUE;
	theParam.bDuplicate		= !aBtnUnique.IsChecked();
	theParam.bCaseSens 		= aBtnCase.IsChecked();
	theParam.bRegExp		= aBtnRegExp.IsChecked();
	theParam.bDestPers		= aBtnDestPers.IsChecked();

	//	nur die drei eingestellten - alles andere zuruecksetzen

	DELETEZ( pOutItem );
	pOutItem = new ScQueryItem( nWhichQuery, &theParam );

	return pOutItem;
}


//----------------------------------------------------------------------------

BOOL ScFilterDlg::IsRefInputMode() const
{
	return bRefInputMode;
}


//----------------------------------------------------------------------------
// Handler:
// ========

IMPL_LINK( ScFilterDlg, EndDlgHdl, Button*, pBtn )
{
	if ( pBtn == &aBtnOk )
	{
		BOOL bAreaInputOk = TRUE;

		if ( aBtnCopyResult.IsChecked() )
		{
			if ( !pOptionsMgr->VerifyPosStr( aEdCopyArea.GetText() ) )
			{
				if ( !aBtnMore.GetState() )
					aBtnMore.SetState( TRUE );

				ERRORBOX( STR_INVALID_TABREF );
				aEdCopyArea.GrabFocus();
				bAreaInputOk = FALSE;
			}
		}

		if ( bAreaInputOk )
		{
			SetDispatcherLock( FALSE );
			SwitchToDocument();
			GetBindings().GetDispatcher()->Execute( FID_FILTER_OK,
									  SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD,
									  GetOutputItem(), 0L, 0L );
			Close();
		}
	}
	else if ( pBtn == &aBtnCancel )
	{
		Close();
	}

	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScFilterDlg, MoreClickHdl, MoreButton*, EMPTYARG )
{
	if ( aBtnMore.GetState() )
		pTimer->Start();
	else
	{
		pTimer->Stop();
		bRefInputMode = FALSE;
		//@BugID 54702 Enablen/Disablen nur noch in Basisklasse
		//SFX_APPWINDOW->Disable(FALSE);		//! allgemeine Methode im ScAnyRefDlg
	}
	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScFilterDlg, TimeOutHdl, Timer*, _pTimer )
{
    // alle 50ms nachschauen, ob RefInputMode noch stimmt

    if( _pTimer == pTimer && IsActive() )
        bRefInputMode = (aEdCopyArea.HasFocus() || aRbCopyArea.HasFocus());

    if ( aBtnMore.GetState() )
        pTimer->Start();

    return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScFilterDlg, LbSelectHdl, ListBox*, pLb )
{
	/*
	 * Behandlung der Enable/Disable-Logik,
	 * abhaengig davon, welche ListBox angefasst wurde:
	 */
	USHORT nOffset = GetSliderPos();

	if ( pLb == &aLbConnect1 )
	{
		aLbField1.Enable();
		aLbCond1.Enable();
		aEdVal1.Enable();

		USHORT 	nConnect1 = aLbConnect1.GetSelectEntryPos();
		USHORT nQE = nOffset;
		theQueryData.GetEntry(nQE).eConnect =(ScQueryConnect)nConnect1;
		bRefreshExceptQuery[nQE]=TRUE;
	}

	else if ( pLb == &aLbConnect2 )
	{
		aLbField2.Enable();
		aLbCond2.Enable();
		aEdVal2.Enable();

		USHORT 	nConnect2 = aLbConnect2.GetSelectEntryPos();
		USHORT nQE = 1+nOffset;
		theQueryData.GetEntry(nQE).eConnect =(ScQueryConnect)nConnect2;
		 bRefreshExceptQuery[nQE]=TRUE;
	}
	else if ( pLb == &aLbConnect3 )
	{
		aLbField3.Enable();
		aLbCond3.Enable();
		aEdVal3.Enable();

		USHORT 	nConnect3 = aLbConnect3.GetSelectEntryPos();
		USHORT nQE = 2+nOffset;
		theQueryData.GetEntry(nQE).eConnect = (ScQueryConnect)nConnect3;
		bRefreshExceptQuery[nQE]=TRUE;

	}
	else if ( pLb == &aLbConnect4 )
	{
		aLbField4.Enable();
		aLbCond4.Enable();
		aEdVal4.Enable();

		USHORT 	nConnect4 = aLbConnect4.GetSelectEntryPos();
		USHORT nQE = 3+nOffset;
		theQueryData.GetEntry(nQE).eConnect = (ScQueryConnect)nConnect4;
		bRefreshExceptQuery[nQE]=TRUE;

	}
	else if ( pLb == &aLbField1 )
	{
		if ( aLbField1.GetSelectEntryPos() == 0 )
		{
			aLbConnect2.SetNoSelection();
			aLbConnect3.SetNoSelection();
			aLbConnect4.SetNoSelection();
			aLbField2.SelectEntryPos( 0 );
			aLbField3.SelectEntryPos( 0 );
			aLbField4.SelectEntryPos( 0 );
			aLbCond2.SelectEntryPos( 0 );
			aLbCond3.SelectEntryPos( 0 );
			aLbCond4.SelectEntryPos( 0 );
			ClearValueList( 1 );
			ClearValueList( 2 );
			ClearValueList( 3 );
			ClearValueList( 4 );

			aLbConnect2.Disable();
			aLbConnect3.Disable();
			aLbConnect4.Disable();
			aLbField2.Disable();
			aLbField3.Disable();
			aLbField4.Disable();
			aLbCond2.Disable();
			aLbCond3.Disable();
			aLbCond4.Disable();
			aEdVal2.Disable();
			aEdVal3.Disable();
			aEdVal4.Disable();
			for (USHORT i= nOffset; i< MAXQUERY; i++)
			{
				theQueryData.GetEntry(i).bDoQuery = FALSE;
				bRefreshExceptQuery[i]=FALSE; 
				theQueryData.GetEntry(i).nField =  static_cast<SCCOL>(0);
			}
			bRefreshExceptQuery[nOffset] =TRUE;
		}
		else
		{
			UpdateValueList( 1 );
			if ( !aLbConnect2.IsEnabled() )
			{
				aLbConnect2.Enable();
			}
			theQueryData.GetEntry(nOffset).bDoQuery = TRUE;
			USHORT 	nField	= pLb->GetSelectEntryPos();
			theQueryData.GetEntry(nOffset).nField = theQueryData.nCol1 + static_cast<SCCOL>(nField) - 1 ;
		}
	}
	else if ( pLb == &aLbField2 )
	{
		if ( aLbField2.GetSelectEntryPos() == 0 )
		{
            aLbConnect3.SetNoSelection();
			aLbConnect4.SetNoSelection();
			aLbField3.SelectEntryPos( 0 );
			aLbField4.SelectEntryPos( 0 );
			aLbCond3.SelectEntryPos( 0 );
			aLbCond4.SelectEntryPos( 0 );
			ClearValueList( 2 );
			ClearValueList( 3 );
			ClearValueList( 4 );

			aLbConnect3.Disable();
			aLbConnect4.Disable();
			aLbField3.Disable();
			aLbField4.Disable();
			aLbCond3.Disable();
			aLbCond4.Disable();
			aEdVal3.Disable();
			aEdVal4.Disable();

			USHORT nTemp=nOffset+1;
			for (USHORT i= nTemp; i< MAXQUERY; i++)
			{
				theQueryData.GetEntry(i).bDoQuery = FALSE;
				bRefreshExceptQuery[i]=FALSE; 
				theQueryData.GetEntry(i).nField =  static_cast<SCCOL>(0);
			}
			bRefreshExceptQuery[nTemp]=TRUE; 
		}
		else
		{
			UpdateValueList( 2 );
			if ( !aLbConnect3.IsEnabled() )
			{
				aLbConnect3.Enable();
			}
			USHORT 	nField	= pLb->GetSelectEntryPos();
			USHORT nQ=1+nOffset;
			theQueryData.GetEntry(nQ).bDoQuery = TRUE;
			theQueryData.GetEntry(nQ).nField = theQueryData.nCol1 + static_cast<SCCOL>(nField) - 1 ;
		}
	}
	else if ( pLb == &aLbField3 )
	{
		if ( aLbField3.GetSelectEntryPos() == 0 )
		{
			aLbConnect4.SetNoSelection();
			aLbField4.SelectEntryPos( 0 );
			aLbCond4.SelectEntryPos( 0 );
			ClearValueList( 3 );
			ClearValueList( 4 );

			aLbConnect4.Disable();
			aLbField4.Disable();
			aLbCond4.Disable();
			aEdVal4.Disable();

			USHORT nTemp=nOffset+2;
			for (USHORT i= nTemp; i< MAXQUERY; i++)
			{
				theQueryData.GetEntry(i).bDoQuery = FALSE;
				bRefreshExceptQuery[i]=FALSE;  
				theQueryData.GetEntry(i).nField =  static_cast<SCCOL>(0);
			}
			bRefreshExceptQuery[nTemp]=TRUE; 
		}
		else
		{
			UpdateValueList( 3 );
			if ( !aLbConnect4.IsEnabled() )
			{
				aLbConnect4.Enable();
			}
			
			USHORT 	nField	= pLb->GetSelectEntryPos();
			USHORT nQ=2+nOffset;
			theQueryData.GetEntry(nQ).bDoQuery = TRUE;
			theQueryData.GetEntry(nQ).nField = theQueryData.nCol1 + static_cast<SCCOL>(nField) - 1 ;

		}
	}
	else if ( pLb == &aLbField4 )
	{
		if ( aLbField4.GetSelectEntryPos() == 0 )
		{
			ClearValueList( 4 );
			USHORT nTemp=nOffset+3;
			for (USHORT i= nTemp; i< MAXQUERY; i++)
			{
				theQueryData.GetEntry(i).bDoQuery = FALSE;
				bRefreshExceptQuery[i]=FALSE; 
				theQueryData.GetEntry(i).nField =  static_cast<SCCOL>(0);
			}
			bRefreshExceptQuery[nTemp]=TRUE; 
		}
		else
		{
			UpdateValueList( 4 );
			USHORT 	nField	= pLb->GetSelectEntryPos();
			USHORT nQ=3+nOffset;
			theQueryData.GetEntry(nQ).bDoQuery = TRUE;
			theQueryData.GetEntry(nQ).nField = theQueryData.nCol1 + static_cast<SCCOL>(nField) - 1 ;
		}

	}
	else if ( pLb == &aLbCond1)
	{
		theQueryData.GetEntry(nOffset).eOp=(ScQueryOp)pLb->GetSelectEntryPos();
	}
	else if ( pLb == &aLbCond2)
	{
		USHORT nQ=1+nOffset;
		theQueryData.GetEntry(nQ).eOp=(ScQueryOp)pLb->GetSelectEntryPos();
	}
	else if ( pLb == &aLbCond3)
	{
		USHORT nQ=2+nOffset;
		theQueryData.GetEntry(nQ).eOp=(ScQueryOp)pLb->GetSelectEntryPos();
	}
	else 
	{
		USHORT nQ=3+nOffset;
		theQueryData.GetEntry(nQ).eOp=(ScQueryOp)pLb->GetSelectEntryPos();
	}

	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScFilterDlg, CheckBoxHdl, CheckBox*, pBox )
{
	//	Spaltenkoepfe:
	//		FeldListen: Spaltexx <-> Spaltenkopf-String
	//		WertListen: Spaltenkopf-Wert entfaellt.
	//	Gross-/Kleinschreibung:
	//		WertListen: komplett neu

	if ( pBox == &aBtnHeader )				// Feldlisten und Wertlisten
	{
		USHORT nCurSel1 = aLbField1.GetSelectEntryPos();
		USHORT nCurSel2 = aLbField2.GetSelectEntryPos();
		USHORT nCurSel3 = aLbField3.GetSelectEntryPos();
		USHORT nCurSel4 = aLbField4.GetSelectEntryPos();
		FillFieldLists();
		aLbField1.SelectEntryPos( nCurSel1 );
		aLbField2.SelectEntryPos( nCurSel2 );
		aLbField3.SelectEntryPos( nCurSel3 );
		aLbField4.SelectEntryPos( nCurSel4 );

		UpdateHdrInValueList( 1 );
		UpdateHdrInValueList( 2 );
		UpdateHdrInValueList( 3 );
		UpdateHdrInValueList( 4 );
	}

	if ( pBox == &aBtnCase )			// Wertlisten komplett
	{
		for (USHORT i=0; i<=MAXCOL; i++)
			DELETEZ( pEntryLists[i] );

		UpdateValueList( 1 );		// aktueller Text wird gemerkt
		UpdateValueList( 2 );
		UpdateValueList( 3 );
		UpdateValueList( 4 );
	}

	return 0;
}


//----------------------------------------------------------------------------

IMPL_LINK( ScFilterDlg, ValModifyHdl, ComboBox*, pEd )
{
	USHORT   nOffset = GetSliderPos();
	USHORT   i=0;
	USHORT   nQE =i + nOffset;
	if ( pEd )
	{
		String    aStrVal	= pEd->GetText();
		ListBox*  pLbCond	= &aLbCond1;
		ListBox*  pLbField	= &aLbField1;
		if ( pEd == &aEdVal2 )
		{
			pLbCond  = &aLbCond2;
			pLbField = &aLbField2;
			i=1;
			nQE=i+nOffset;
		}
		if ( pEd == &aEdVal3 )
		{
			pLbCond = &aLbCond3;
			pLbField = &aLbField3;
			i=2;
			nQE=i+nOffset;
		}
		if ( pEd == &aEdVal4 )
		{
			pLbCond = &aLbCond4;
			pLbField = &aLbField4;
			i=3;
			nQE=i+nOffset;
		}

		if ( aStrEmpty == aStrVal || aStrNotEmpty == aStrVal )
		{
			pLbCond->SelectEntry( '=' );
			pLbCond->Disable();
		}
		else
			pLbCond->Enable();

		ScQueryEntry& rEntry = theQueryData.GetEntry( nQE );
		BOOL bDoThis = (pLbField->GetSelectEntryPos() != 0);
		rEntry.bDoQuery = bDoThis;

		if ( rEntry.bDoQuery || bRefreshExceptQuery[nQE] )
		{
			if ( aStrVal == aStrEmpty )
			{
				rEntry.pStr->Erase();
				rEntry.nVal = SC_EMPTYFIELDS;
				rEntry.bQueryByString = FALSE;
			}
			else if ( aStrVal == aStrNotEmpty )
			{
				rEntry.pStr->Erase();
				rEntry.nVal = SC_NONEMPTYFIELDS;
				rEntry.bQueryByString = FALSE;
			}
			else
			{
				*rEntry.pStr		  = aStrVal;
				rEntry.nVal			  = 0;
				rEntry.bQueryByString = TRUE;
			}

			USHORT 	nField	= pLbField->GetSelectEntryPos();
			rEntry.nField = nField ? (theQueryData.nCol1 +
				static_cast<SCCOL>(nField) - 1) : static_cast<SCCOL>(0);
		   
			ScQueryOp eOp  = (ScQueryOp)pLbCond->GetSelectEntryPos();
		   rEntry.eOp	  = eOp;

		}
	}		
	return 0;
}

//----------------------------------------------------------------------------
IMPL_LINK( ScFilterDlg, ScrollHdl, ScrollBar*, EMPTYARG )
{
	SliderMoved();
	return 0;
}

void ScFilterDlg::SliderMoved()
{
	USHORT nOffset = GetSliderPos();
	RefreshEditRow( nOffset);
}
USHORT ScFilterDlg::GetSliderPos()
{
	return (USHORT) aScrollBar.GetThumbPos();
}
void ScFilterDlg::RefreshEditRow( USHORT nOffset )
{		
	if (nOffset==0) 
		aConnLbArr[0]->Hide();
	else
		aConnLbArr[0]->Show();	

	for ( USHORT i=0; i<4; i++ )
	{
		String	aValStr;
		USHORT	nCondPos	 = 0;
		USHORT	nFieldSelPos = 0;
		USHORT  nQE = i+nOffset;

		ScQueryEntry& rEntry = theQueryData.GetEntry( nQE);
		if ( rEntry.bDoQuery || bRefreshExceptQuery[nQE] )
		{
			nCondPos	 = (USHORT)rEntry.eOp;
			if(rEntry.bDoQuery)
			   nFieldSelPos = GetFieldSelPos( static_cast<SCCOL>(rEntry.nField) );

			if ( rEntry.nVal == SC_EMPTYFIELDS && !rEntry.bQueryByString && *rEntry.pStr == EMPTY_STRING )
			{
				aValStr = aStrEmpty;
				aCondLbArr[i]->Disable();
			}
			else if ( rEntry.nVal == SC_NONEMPTYFIELDS && !rEntry.bQueryByString && *rEntry.pStr == EMPTY_STRING )
			{
				aValStr = aStrNotEmpty;
				aCondLbArr[i]->Disable();
			}
			else
			{
				aValStr = *rEntry.pStr;
				aCondLbArr[i]->Enable();
			}
			aFieldLbArr[i]->Enable();
			aValueEdArr[i]->Enable();

			if (nOffset==0)
			{
				if (i<3)
				{
					if(rEntry.bDoQuery)
						aConnLbArr[i+1]->Enable();
					else
						aConnLbArr[i+1]->Disable();
					USHORT nQENext = nQE+1;
					if(theQueryData.GetEntry(nQENext).bDoQuery || bRefreshExceptQuery[nQENext])
						aConnLbArr[i+1]->SelectEntryPos( (USHORT) theQueryData.GetEntry(nQENext).eConnect );
					else
						aConnLbArr[i+1]->SetNoSelection();
				}
			}
			else
			{
				if(theQueryData.GetEntry( nQE-1).bDoQuery)
					aConnLbArr[i]->Enable();
				else
					aConnLbArr[i]->Disable();

				if(rEntry.bDoQuery || bRefreshExceptQuery[nQE])
					aConnLbArr[i]->SelectEntryPos( (USHORT) rEntry.eConnect );
				else
					aConnLbArr[i]->SetNoSelection();
			}
			
		}
		else 
		{
			if (nOffset==0)
			{
				if(i<3)
				{
					aConnLbArr[i+1]->SetNoSelection();
					aConnLbArr[i+1]->Disable();
				}			
			}
			else
			{
				if(theQueryData.GetEntry( nQE-1).bDoQuery)
					aConnLbArr[i]->Enable();
				else
					aConnLbArr[i]->Disable();
				aConnLbArr[i]->SetNoSelection();
			}
			aFieldLbArr[i]->Disable();
			aCondLbArr[i]->Disable();
			aValueEdArr[i]->Disable();
		}				
		aFieldLbArr[i]->SelectEntryPos( nFieldSelPos );
		aCondLbArr [i]->SelectEntryPos( nCondPos );
		aValueEdArr[i]->SetText( aValStr );
		UpdateValueList( static_cast<USHORT>(i+1) );
	}
}
