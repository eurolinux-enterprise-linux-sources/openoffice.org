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

#include "dbfunc.hxx"
#include "scitems.hxx"
#include <sfx2/bindings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/sound.hxx>
#include <vcl/waitobj.hxx>
#include <svtools/zforlist.hxx>
#include <sfx2/app.hxx>
#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/sheet/DataPilotFieldSortMode.hpp>
#include <com/sun/star/sheet/MemberResultFlags.hpp>

#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/sheet/DataPilotTableHeaderData.hpp>
#include <com/sun/star/sheet/MemberResultFlags.hpp>
#include <com/sun/star/sheet/DataPilotFieldGroupBy.hpp>
#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>
#include <com/sun/star/sheet/XDrillDownDataSupplier.hpp>
#include <com/sun/star/sheet/XDimensionsSupplier.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameAccess.hpp>

#include "global.hxx"
#include "globstr.hrc"
#include "sc.hrc"
#include "undotab.hxx"
#include "undodat.hxx"
#include "dbcolect.hxx"
#include "rangenam.hxx"
#include "rangeutl.hxx"
#include "docsh.hxx"
#include "olinetab.hxx"
#include "consoli.hxx"
#include "olinefun.hxx"
#include "dpobject.hxx"
#include "dpsave.hxx"
#include "dpdimsave.hxx"
#include "dbdocfun.hxx"
#include "dpoutput.hxx"
#include "dptabsrc.hxx"
#include "editable.hxx"
#include "docpool.hxx"
#include "patattr.hxx"
#include "unonames.hxx"
#include "cell.hxx"

#include <hash_set>
#include <memory>

using namespace com::sun::star;
using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::beans::XPropertySet;
using ::std::auto_ptr;

// STATIC DATA -----------------------------------------------------------


//==================================================================

//
//			Outliner
//

//	Outline-Gruppierung erzeugen

void ScDBFunc::MakeOutline( BOOL bColumns, BOOL bRecord )
{
	ScRange aRange;
	if (GetViewData()->GetSimpleArea(aRange) == SC_MARK_SIMPLE)
	{
		ScDocShell* pDocSh = GetViewData()->GetDocShell();
		ScOutlineDocFunc aFunc(*pDocSh);
		aFunc.MakeOutline( aRange, bColumns, bRecord, FALSE );
	}
	else
		ErrorMessage(STR_NOMULTISELECT);
}

//	Outline-Gruppierung loeschen

void ScDBFunc::RemoveOutline( BOOL bColumns, BOOL bRecord )
{
	ScRange aRange;
	if (GetViewData()->GetSimpleArea(aRange) == SC_MARK_SIMPLE)
	{
		ScDocShell* pDocSh = GetViewData()->GetDocShell();
		ScOutlineDocFunc aFunc(*pDocSh);
		aFunc.RemoveOutline( aRange, bColumns, bRecord, FALSE );
	}
	else
		ErrorMessage(STR_NOMULTISELECT);
}

//	Menue-Status: Outlines loeschen

void ScDBFunc::TestRemoveOutline( BOOL& rCol, BOOL& rRow )
{
	BOOL bColFound = FALSE;
	BOOL bRowFound = FALSE;

	SCCOL nStartCol, nEndCol;
	SCROW nStartRow, nEndRow;
	SCTAB nStartTab, nEndTab;
	if (GetViewData()->GetSimpleArea(nStartCol,nStartRow,nStartTab,nEndCol,nEndRow,nEndTab) == SC_MARK_SIMPLE)
	{
		SCTAB nTab = nStartTab;
		ScDocument* pDoc = GetViewData()->GetDocument();
		ScOutlineTable* pTable = pDoc->GetOutlineTable( nTab );
		if (pTable)
		{
			ScOutlineArray* pArray;
			ScOutlineEntry* pEntry;
			SCCOLROW nStart;
			SCCOLROW nEnd;
			BOOL bColMarked = ( nStartRow == 0 && nEndRow == MAXROW );
			BOOL bRowMarked = ( nStartCol == 0 && nEndCol == MAXCOL );

			//	Spalten

			if ( !bRowMarked || bColMarked )		// nicht wenn ganze Zeilen markiert
			{
				pArray = pTable->GetColArray();
				ScSubOutlineIterator aColIter( pArray );
				while ((pEntry=aColIter.GetNext()) != NULL && !bColFound)
				{
					nStart = pEntry->GetStart();
					nEnd   = pEntry->GetEnd();
					if ( nStartCol<=static_cast<SCCOL>(nEnd) && nEndCol>=static_cast<SCCOL>(nStart) )
						bColFound = TRUE;
				}
			}

			//	Zeilen

			if ( !bColMarked || bRowMarked )		// nicht wenn ganze Spalten markiert
			{
				pArray = pTable->GetRowArray();
				ScSubOutlineIterator aRowIter( pArray );
				while ((pEntry=aRowIter.GetNext()) != NULL && !bRowFound)
				{
					nStart = pEntry->GetStart();
					nEnd   = pEntry->GetEnd();
					if ( nStartRow<=nEnd && nEndRow>=nStart )
						bRowFound = TRUE;
				}
			}
		}
	}

	rCol = bColFound;
	rRow = bRowFound;
}

void ScDBFunc::RemoveAllOutlines( BOOL bRecord )
{
	SCTAB nTab = GetViewData()->GetTabNo();
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScOutlineDocFunc aFunc(*pDocSh);

	HideCursor();
	BOOL bOk = aFunc.RemoveAllOutlines( nTab, bRecord, FALSE );
	ShowCursor();

	if (bOk)
		UpdateScrollBars();
}

//	Auto-Outlines

void ScDBFunc::AutoOutline( BOOL bRecord )
{
	SCTAB nTab = GetViewData()->GetTabNo();
	ScRange aRange( 0,0,nTab, MAXCOL,MAXROW,nTab );		// ganze Tabelle, wenn nichts markiert
	ScMarkData& rMark = GetViewData()->GetMarkData();
	if ( rMark.IsMarked() || rMark.IsMultiMarked() )
	{
		rMark.MarkToMulti();
		rMark.GetMultiMarkArea( aRange );
	}

	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScOutlineDocFunc aFunc(*pDocSh);
	aFunc.AutoOutline( aRange, bRecord, FALSE );
}

//	Outline-Ebene auswaehlen

void ScDBFunc::SelectLevel( BOOL bColumns, USHORT nLevel, BOOL bRecord, BOOL bPaint )
{
	SCTAB nTab = GetViewData()->GetTabNo();
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScOutlineDocFunc aFunc(*pDocSh);

	HideCursor();
	BOOL bOk = aFunc.SelectLevel( nTab, bColumns, nLevel, bRecord, bPaint, FALSE );
	ShowCursor();

	if (bOk)
		UpdateScrollBars();
}

//	einzelne Outline-Gruppe einblenden

void ScDBFunc::ShowOutline( BOOL bColumns, USHORT nLevel, USHORT nEntry, BOOL bRecord, BOOL bPaint )
{
	SCTAB nTab = GetViewData()->GetTabNo();
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScOutlineDocFunc aFunc(*pDocSh);

	HideCursor();
	BOOL bOk = aFunc.ShowOutline( nTab, bColumns, nLevel, nEntry, bRecord, bPaint, FALSE );
	ShowCursor();

	if ( bOk && bPaint )
		UpdateScrollBars();
}

//	einzelne Outline-Gruppe ausblenden

void ScDBFunc::HideOutline( BOOL bColumns, USHORT nLevel, USHORT nEntry, BOOL bRecord, BOOL bPaint )
{
	SCTAB nTab = GetViewData()->GetTabNo();
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScOutlineDocFunc aFunc(*pDocSh);

	HideCursor();
	BOOL bOk = aFunc.HideOutline( nTab, bColumns, nLevel, nEntry, bRecord, bPaint, FALSE );
	ShowCursor();

	if ( bOk && bPaint )
		UpdateScrollBars();
}

//	Menue-Status: markierten Bereich ein-/ausblenden

BOOL ScDBFunc::OutlinePossible(BOOL bHide)
{
	BOOL bEnable = FALSE;

	SCCOL nStartCol;
	SCROW nStartRow;
	SCTAB nStartTab;
	SCCOL nEndCol;
	SCROW nEndRow;
	SCTAB nEndTab;

	if (GetViewData()->GetSimpleArea(nStartCol,nStartRow,nStartTab,nEndCol,nEndRow,nEndTab) == SC_MARK_SIMPLE)
	{
		ScDocument* pDoc = GetViewData()->GetDocument();
		SCTAB nTab = GetViewData()->GetTabNo();
		ScOutlineTable* pTable = pDoc->GetOutlineTable( nTab );
		if (pTable)
		{
			ScOutlineArray* pArray;
			ScOutlineEntry* pEntry;
			SCCOLROW nStart;
			SCCOLROW nEnd;

			//	Spalten

			pArray = pTable->GetColArray();
			ScSubOutlineIterator aColIter( pArray );
			while ((pEntry=aColIter.GetNext()) != NULL && !bEnable)
			{
				nStart = pEntry->GetStart();
				nEnd   = pEntry->GetEnd();
				if ( bHide )
				{
					if ( nStartCol<=static_cast<SCCOL>(nEnd) && nEndCol>=static_cast<SCCOL>(nStart) )
						if (!pEntry->IsHidden())
							bEnable = TRUE;
				}
				else
				{
					if ( nStart>=nStartCol && nEnd<=nEndCol )
						if (pEntry->IsHidden())
							bEnable = TRUE;
				}
			}

			//	Zeilen

			pArray = pTable->GetRowArray();
			ScSubOutlineIterator aRowIter( pArray );
			while ((pEntry=aRowIter.GetNext()) != NULL)
			{
				nStart = pEntry->GetStart();
				nEnd   = pEntry->GetEnd();
				if ( bHide )
				{
					if ( nStartRow<=nEnd && nEndRow>=nStart )
						if (!pEntry->IsHidden())
							bEnable = TRUE;
				}
				else
				{
					if ( nStart>=nStartRow && nEnd<=nEndRow )
						if (pEntry->IsHidden())
							bEnable = TRUE;
				}
			}
		}
	}

	return bEnable;
}

//	markierten Bereich einblenden

void ScDBFunc::ShowMarkedOutlines( BOOL bRecord )
{
	ScRange aRange;
	if (GetViewData()->GetSimpleArea(aRange) == SC_MARK_SIMPLE)
	{
		ScDocShell* pDocSh = GetViewData()->GetDocShell();
		ScOutlineDocFunc aFunc(*pDocSh);
		HideCursor();
		BOOL bDone = aFunc.ShowMarkedOutlines( aRange, bRecord, FALSE );
		ShowCursor();
		if (bDone)
			UpdateScrollBars();
	}
	else
		ErrorMessage(STR_NOMULTISELECT);
}

//	markierten Bereich ausblenden

void ScDBFunc::HideMarkedOutlines( BOOL bRecord )
{
	ScRange aRange;
	if (GetViewData()->GetSimpleArea(aRange) == SC_MARK_SIMPLE)
	{
		ScDocShell* pDocSh = GetViewData()->GetDocShell();
		ScOutlineDocFunc aFunc(*pDocSh);
		HideCursor();
		BOOL bDone = aFunc.HideMarkedOutlines( aRange, bRecord, FALSE );
		ShowCursor();
		if (bDone)
			UpdateScrollBars();
	}
	else
		ErrorMessage(STR_NOMULTISELECT);
}

//	--------------------------------------------------------------------------

//
//			Teilergebnisse
//

void ScDBFunc::DoSubTotals( const ScSubTotalParam& rParam, BOOL bRecord,
							const ScSortParam* pForceNewSort )
{
	BOOL bDo = !rParam.bRemoveOnly;							// FALSE = nur loeschen

	ScDocShell* pDocSh = GetViewData()->GetDocShell();
	ScDocument* pDoc = pDocSh->GetDocument();
	ScMarkData& rMark = GetViewData()->GetMarkData();
	SCTAB nTab = GetViewData()->GetTabNo();
	if (bRecord && !pDoc->IsUndoEnabled())
		bRecord = FALSE;

	ScDBData* pDBData = pDoc->GetDBAtArea( nTab, rParam.nCol1, rParam.nRow1,
												rParam.nCol2, rParam.nRow2 );
	if (!pDBData)
	{
		DBG_ERROR( "SubTotals: keine DBData" );
		return;
	}

	ScEditableTester aTester( pDoc, nTab, 0,rParam.nRow1+1, MAXCOL,MAXROW );
	if (!aTester.IsEditable())
	{
		ErrorMessage(aTester.GetMessageId());
		return;
	}

	if (pDoc->HasAttrib( rParam.nCol1, rParam.nRow1+1, nTab,
						 rParam.nCol2, rParam.nRow2, nTab, HASATTR_MERGED | HASATTR_OVERLAPPED ))
	{
		ErrorMessage(STR_MSSG_INSERTCELLS_0);	// nicht in zusammengefasste einfuegen
		return;
	}

	WaitObject aWait( GetViewData()->GetDialogParent() );
	BOOL bOk = TRUE;
	BOOL bDelete = FALSE;
	if (rParam.bReplace)
		if (pDoc->TestRemoveSubTotals( nTab, rParam ))
		{
			bDelete = TRUE;
			bOk = ( MessBox( GetViewData()->GetDialogParent(), WinBits(WB_YES_NO | WB_DEF_YES),
				// "StarCalc" "Daten loeschen?"
				ScGlobal::GetRscString( STR_MSSG_DOSUBTOTALS_0 ),
				ScGlobal::GetRscString( STR_MSSG_DOSUBTOTALS_1 ) ).Execute()
				== RET_YES );
		}

	if (bOk)
	{
		ScDocShellModificator aModificator( *pDocSh );

		ScSubTotalParam aNewParam( rParam );		// Bereichsende wird veraendert
		ScDocument*		pUndoDoc = NULL;
		ScOutlineTable*	pUndoTab = NULL;
		ScRangeName*	pUndoRange = NULL;
		ScDBCollection* pUndoDB = NULL;
		SCTAB 			nTabCount = 0;				// fuer Referenz-Undo

		if (bRecord)										// alte Daten sichern
		{
			BOOL bOldFilter = bDo && rParam.bDoSort;

			nTabCount = pDoc->GetTableCount();
			pUndoDoc = new ScDocument( SCDOCMODE_UNDO );
			ScOutlineTable* pTable = pDoc->GetOutlineTable( nTab );
			if (pTable)
			{
				pUndoTab = new ScOutlineTable( *pTable );

				SCCOLROW nOutStartCol;							// Zeilen/Spaltenstatus
				SCCOLROW nOutStartRow;
				SCCOLROW nOutEndCol;
				SCCOLROW nOutEndRow;
				pTable->GetColArray()->GetRange( nOutStartCol, nOutEndCol );
				pTable->GetRowArray()->GetRange( nOutStartRow, nOutEndRow );

				pUndoDoc->InitUndo( pDoc, nTab, nTab, TRUE, TRUE );
				pDoc->CopyToDocument( static_cast<SCCOL>(nOutStartCol), 0, nTab, static_cast<SCCOL>(nOutEndCol), MAXROW, nTab, IDF_NONE, FALSE, pUndoDoc );
				pDoc->CopyToDocument( 0, nOutStartRow, nTab, MAXCOL, nOutEndRow, nTab, IDF_NONE, FALSE, pUndoDoc );
			}
			else
				pUndoDoc->InitUndo( pDoc, nTab, nTab, FALSE, bOldFilter );

			//	Datenbereich sichern - incl. Filter-Ergebnis
			pDoc->CopyToDocument( 0,rParam.nRow1+1,nTab, MAXCOL,rParam.nRow2,nTab,
									IDF_ALL, FALSE, pUndoDoc );

			//	alle Formeln wegen Referenzen
			pDoc->CopyToDocument( 0,0,0, MAXCOL,MAXROW,nTabCount-1,
										IDF_FORMULA, FALSE, pUndoDoc );

			//	DB- und andere Bereiche
			ScRangeName* pDocRange = pDoc->GetRangeName();
			if (pDocRange->GetCount())
				pUndoRange = new ScRangeName( *pDocRange );
			ScDBCollection* pDocDB = pDoc->GetDBCollection();
			if (pDocDB->GetCount())
				pUndoDB = new ScDBCollection( *pDocDB );
		}

//		pDoc->SetOutlineTable( nTab, NULL );
		ScOutlineTable*	pOut = pDoc->GetOutlineTable( nTab );
		if (pOut)
			pOut->GetRowArray()->RemoveAll();		// nur Zeilen-Outlines loeschen

		if (rParam.bReplace)
			pDoc->RemoveSubTotals( nTab, aNewParam );
		BOOL bSuccess = TRUE;
		if (bDo)
		{
			// Sortieren
			if ( rParam.bDoSort || pForceNewSort )
			{
				pDBData->SetArea( nTab, aNewParam.nCol1,aNewParam.nRow1, aNewParam.nCol2,aNewParam.nRow2 );

				//	Teilergebnis-Felder vor die Sortierung setzen
				//	(doppelte werden weggelassen, kann darum auch wieder aufgerufen werden)

				ScSortParam aOldSort;
				pDBData->GetSortParam( aOldSort );
				ScSortParam aSortParam( aNewParam, pForceNewSort ? *pForceNewSort : aOldSort );
				Sort( aSortParam, FALSE, FALSE );
			}

			bSuccess = pDoc->DoSubTotals( nTab, aNewParam );
		}
		ScRange aDirtyRange( aNewParam.nCol1, aNewParam.nRow1, nTab,
			aNewParam.nCol2, aNewParam.nRow2, nTab );
		pDoc->SetDirty( aDirtyRange );

		if (bRecord)
		{
//			ScDBData* pUndoDBData = pDBData ? new ScDBData( *pDBData ) : NULL;
			pDocSh->GetUndoManager()->AddUndoAction(
				new ScUndoSubTotals( pDocSh, nTab,
										rParam, aNewParam.nRow2,
										pUndoDoc, pUndoTab, // pUndoDBData,
										pUndoRange, pUndoDB ) );
		}

		if (!bSuccess)
		{
			// "Kann keine Zeilen einfuegen"
			ErrorMessage(STR_MSSG_DOSUBTOTALS_2);
		}

													// merken
		pDBData->SetSubTotalParam( aNewParam );
		pDBData->SetArea( nTab, aNewParam.nCol1,aNewParam.nRow1, aNewParam.nCol2,aNewParam.nRow2 );
		pDoc->CompileDBFormula();

		DoneBlockMode();
		InitOwnBlockMode();
		rMark.SetMarkArea( ScRange( aNewParam.nCol1,aNewParam.nRow1,nTab,
									aNewParam.nCol2,aNewParam.nRow2,nTab ) );
        MarkDataChanged();

		pDocSh->PostPaint( 0,0,nTab, MAXCOL,MAXROW,nTab,
												PAINT_GRID | PAINT_LEFT | PAINT_TOP | PAINT_SIZE );

		aModificator.SetDocumentModified();

		SelectionChanged();
	}
}

//
//			Pivot
//

String lcl_MakePivotTabName( const String& rPrefix, SCTAB nNumber )
{
	String aName = rPrefix;
	aName += String::CreateFromInt32( nNumber );
	return aName;
}

bool ScDBFunc::MakePivotTable( const ScDPSaveData& rData, const ScRange& rDest, BOOL bNewTable,
								const ScDPObject& rSource, BOOL bApi )
{
	//	#70096# error message if no fields are set
	//	this must be removed when drag&drop of fields from a toolbox is available

	if ( rData.IsEmpty() && !bApi )
	{
		ErrorMessage(STR_PIVOT_NODATA);
		return false;
	}

	ScDocShell* pDocSh	= GetViewData()->GetDocShell();
	ScDocument* pDoc 	= GetViewData()->GetDocument();
	BOOL bUndo(pDoc->IsUndoEnabled());

	ScRange aDestRange = rDest;
	if ( bNewTable )
	{
		SCTAB nSrcTab = GetViewData()->GetTabNo();

		String aName( ScGlobal::GetRscString(STR_PIVOT_TABLE) );
		String aStr;

		pDoc->GetName( nSrcTab, aStr );
		aName += '_';
		aName += aStr;
		aName += '_';

		SCTAB nNewTab = nSrcTab+1;

		SCTAB i=1;
		while ( !pDoc->InsertTab( nNewTab, lcl_MakePivotTabName( aName, i ) ) && i <= MAXTAB )
			i++;

		BOOL bAppend = ( nNewTab+1 == pDoc->GetTableCount() );
		if (bUndo)
		{
			pDocSh->GetUndoManager()->AddUndoAction(
						new ScUndoInsertTab( pDocSh, nNewTab, bAppend, lcl_MakePivotTabName( aName, i ) ));
		}

		GetViewData()->InsertTab( nNewTab );
		SetTabNo( nNewTab, TRUE );

		aDestRange = ScRange( 0, 0, nNewTab );
	}

	ScDPObject* pDPObj = pDoc->GetDPAtCursor(
							aDestRange.aStart.Col(), aDestRange.aStart.Row(), aDestRange.aStart.Tab() );

	ScDPObject aObj( rSource );
	aObj.SetOutRange( aDestRange );
    if ( pDPObj && !rData.GetExistingDimensionData() )
    {
        // copy dimension data from old object - lost in the dialog
        //! change the dialog to keep the dimension data

        ScDPSaveData aNewData( rData );
        const ScDPSaveData* pOldData = pDPObj->GetSaveData();
        if ( pOldData )
        {
            const ScDPDimensionSaveData* pDimSave = pOldData->GetExistingDimensionData();
            aNewData.SetDimensionData( pDimSave );
        }
        aObj.SetSaveData( aNewData );
    }
    else
        aObj.SetSaveData( rData );

	BOOL bAllowMove = ( pDPObj != NULL );	// allow re-positioning when editing existing table

	ScDBDocFunc aFunc( *pDocSh );
	bool bSuccess = aFunc.DataPilotUpdate( pDPObj, &aObj, TRUE, FALSE, bAllowMove );

	CursorPosChanged();		// shells may be switched

	if ( bNewTable )
	{
		pDocSh->PostPaintExtras();
		SFX_APP()->Broadcast( SfxSimpleHint( SC_HINT_TABLES_CHANGED ) );
	}

	return bSuccess;
}

void ScDBFunc::DeletePivotTable()
{
	ScDocShell* pDocSh	  = GetViewData()->GetDocShell();
	ScDocument* pDoc	  = pDocSh->GetDocument();
	ScDPObject*	pDPObj    = pDoc->GetDPAtCursor( GetViewData()->GetCurX(),
												  GetViewData()->GetCurY(),
												  GetViewData()->GetTabNo() );
	if ( pDPObj )
	{
		ScDBDocFunc aFunc( *pDocSh );
		aFunc.DataPilotUpdate( pDPObj, NULL, TRUE, FALSE );
		CursorPosChanged();		// shells may be switched
	}
	else
		ErrorMessage(STR_PIVOT_NOTFOUND);
}

void ScDBFunc::RecalcPivotTable()
{
	ScDocShell* pDocSh  = GetViewData()->GetDocShell();
	ScDocument* pDoc	= GetViewData()->GetDocument();

	//	old pivot not used any more

	ScDPObject*	pDPObj	= pDoc->GetDPAtCursor( GetViewData()->GetCurX(),
												  GetViewData()->GetCurY(),
												  GetViewData()->GetTabNo() );
	if ( pDPObj )
	{
		ScDBDocFunc aFunc( *pDocSh );
		aFunc.DataPilotUpdate( pDPObj, pDPObj, TRUE, FALSE );
		CursorPosChanged();		// shells may be switched
	}
	else
		ErrorMessage(STR_PIVOT_NOTFOUND);
}

void ScDBFunc::GetSelectedMemberList( ScStrCollection& rEntries, long& rDimension )
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
            							GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( !pDPObj )
        return;

    long nStartDimension = -1;
    long nStartHierarchy = -1;
    long nStartLevel     = -1;

    ScRangeListRef xRanges;
    GetViewData()->GetMultiArea( xRanges );         // incl. cursor if nothing is selected
    ULONG nRangeCount = xRanges->Count();
    BOOL bContinue = TRUE;

    for (ULONG nRangePos=0; nRangePos<nRangeCount && bContinue; nRangePos++)
    {
        ScRange aRange = *xRanges->GetObject(nRangePos);
        SCCOL nStartCol = aRange.aStart.Col();
        SCROW nStartRow = aRange.aStart.Row();
        SCCOL nEndCol = aRange.aEnd.Col();
        SCROW nEndRow = aRange.aEnd.Row();
        SCTAB nTab = aRange.aStart.Tab();

        for (SCROW nRow=nStartRow; nRow<=nEndRow && bContinue; nRow++)
            for (SCCOL nCol=nStartCol; nCol<=nEndCol && bContinue; nCol++)
            {
                sheet::DataPilotTableHeaderData aData;
                pDPObj->GetHeaderPositionData(ScAddress(nCol, nRow, nTab), aData);
                if ( aData.Dimension < 0 )
                    bContinue = FALSE;              // not part of any dimension
                else
                {
                    if ( nStartDimension < 0 )      // first member?
                    {
                        nStartDimension = aData.Dimension;
                        nStartHierarchy = aData.Hierarchy;
                        nStartLevel     = aData.Level;
                    }
                    if ( aData.Dimension != nStartDimension ||
                         aData.Hierarchy != nStartHierarchy ||
                         aData.Level     != nStartLevel )
                    {
                        bContinue = FALSE;          // cannot mix dimensions
                    }
                }
                if ( bContinue )
                {
                    // accept any part of a member description, also subtotals,
                    // but don't stop if empty parts are contained
                    if ( aData.Flags & sheet::MemberResultFlags::HASMEMBER )
                    {
                        StrData* pNew = new StrData( aData.MemberName );
                        if ( !rEntries.Insert( pNew ) )
                            delete pNew;
                    }
                }
            }
    }

    rDimension = nStartDimension;   // dimension from which the found members came
    if (!bContinue)
        rEntries.FreeAll();         // remove all if not valid
}

BOOL ScDBFunc::HasSelectionForDateGroup( ScDPNumGroupInfo& rOldInfo, sal_Int32& rParts )
{
    // determine if the date group dialog has to be shown for the current selection

    BOOL bFound = FALSE;

    SCCOL nCurX = GetViewData()->GetCurX();
    SCROW nCurY = GetViewData()->GetCurY();
    SCTAB nTab = GetViewData()->GetTabNo();
    ScDocument* pDoc = GetViewData()->GetDocument();

    ScDPObject* pDPObj = pDoc->GetDPAtCursor( nCurX, nCurY, nTab );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );
            String aBaseDimName( aDimName );

            BOOL bInGroupDim = FALSE;
            BOOL bFoundParts = FALSE;

            ScDPDimensionSaveData* pDimData =
                const_cast<ScDPDimensionSaveData*>( pDPObj->GetSaveData()->GetExistingDimensionData() );
            if ( pDimData )
            {
                const ScDPSaveNumGroupDimension* pNumGroupDim = pDimData->GetNumGroupDim( aDimName );
                const ScDPSaveGroupDimension* pGroupDim = pDimData->GetNamedGroupDim( aDimName );
                if ( pNumGroupDim )
                {
                    //  existing num group dimension

                    if ( pNumGroupDim->GetDatePart() != 0 )
                    {
                        //  dimension has date info -> edit settings of this dimension
                        //  (parts are collected below)

                        rOldInfo = pNumGroupDim->GetDateInfo();
                        bFound = TRUE;
                    }
                    else if ( pNumGroupDim->GetInfo().DateValues )
                    {
                        //  Numerical grouping with DateValues flag is used for grouping
                        //  of days with a "Number of days" value.

                        rOldInfo = pNumGroupDim->GetInfo();
                        rParts = com::sun::star::sheet::DataPilotFieldGroupBy::DAYS;               // not found in CollectDateParts
                        bFoundParts = TRUE;
                        bFound = TRUE;
                    }
                    bInGroupDim = TRUE;
                }
                else if ( pGroupDim )
                {
                    //  existing additional group dimension

                    if ( pGroupDim->GetDatePart() != 0 )
                    {
                        //  dimension has date info -> edit settings of this dimension
                        //  (parts are collected below)

                        rOldInfo = pGroupDim->GetDateInfo();
                        aBaseDimName = pGroupDim->GetSourceDimName();
                        bFound = TRUE;
                    }
                    bInGroupDim = TRUE;
                }
            }
            if ( bFound && !bFoundParts )
            {
                // collect date parts from all group dimensions
                rParts = pDimData->CollectDateParts( aBaseDimName );
            }
            if ( !bFound && !bInGroupDim )
            {
                // create new date group dimensions if the selection is a single cell
                // in a normal dimension with date content

                ScRange aSelRange;
                if ( (GetViewData()->GetSimpleArea( aSelRange ) == SC_MARK_SIMPLE) &&
                        aSelRange.aStart == aSelRange.aEnd )
                {
                    SCCOL nSelCol = aSelRange.aStart.Col();
                    SCROW nSelRow = aSelRange.aStart.Row();
                    SCTAB nSelTab = aSelRange.aStart.Tab();
                    if ( pDoc->HasValueData( nSelCol, nSelRow, nSelTab ) )
                    {
                        ULONG nIndex = static_cast<const SfxUInt32Item*>(pDoc->GetAttr(
                                        nSelCol, nSelRow, nSelTab, ATTR_VALUE_FORMAT))->GetValue();
                        short nType = pDoc->GetFormatTable()->GetType(nIndex);
                        if ( nType == NUMBERFORMAT_DATE || nType == NUMBERFORMAT_TIME || nType == NUMBERFORMAT_DATETIME )
                        {
                            bFound = TRUE;
                            // use currently selected value for automatic limits
                            if( rOldInfo.AutoStart )
                                rOldInfo.Start = pDoc->GetValue( aSelRange.aStart );
                            if( rOldInfo.AutoEnd )
                                rOldInfo.End = pDoc->GetValue( aSelRange.aStart );
                        }
                    }
                }
            }
        }
    }

    return bFound;
}

BOOL ScDBFunc::HasSelectionForNumGroup( ScDPNumGroupInfo& rOldInfo )
{
    // determine if the numeric group dialog has to be shown for the current selection

    BOOL bFound = FALSE;

    SCCOL nCurX = GetViewData()->GetCurX();
    SCROW nCurY = GetViewData()->GetCurY();
    SCTAB nTab = GetViewData()->GetTabNo();
    ScDocument* pDoc = GetViewData()->GetDocument();

    ScDPObject* pDPObj = pDoc->GetDPAtCursor( nCurX, nCurY, nTab );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );

            BOOL bInGroupDim = FALSE;

            ScDPDimensionSaveData* pDimData =
                const_cast<ScDPDimensionSaveData*>( pDPObj->GetSaveData()->GetExistingDimensionData() );
            if ( pDimData )
            {
                const ScDPSaveNumGroupDimension* pNumGroupDim = pDimData->GetNumGroupDim( aDimName );
                if ( pNumGroupDim )
                {
                    //  existing num group dimension
                    //  -> edit settings of this dimension

                    rOldInfo = pNumGroupDim->GetInfo();
                    bFound = TRUE;
                }
                else if ( pDimData->GetNamedGroupDim( aDimName ) )
                    bInGroupDim = TRUE;                                    // in a group dimension
            }
            if ( !bFound && !bInGroupDim )
            {
                // create a new num group dimension if the selection is a single cell
                // in a normal dimension with numeric content

                ScRange aSelRange;
                if ( (GetViewData()->GetSimpleArea( aSelRange ) == SC_MARK_SIMPLE) &&
                        aSelRange.aStart == aSelRange.aEnd )
                {
                    if ( pDoc->HasValueData( aSelRange.aStart.Col(), aSelRange.aStart.Row(),
                                             aSelRange.aStart.Tab() ) )
                    {
                        bFound = TRUE;
                        // use currently selected value for automatic limits
                        if( rOldInfo.AutoStart )
                            rOldInfo.Start = pDoc->GetValue( aSelRange.aStart );
                        if( rOldInfo.AutoEnd )
                            rOldInfo.End = pDoc->GetValue( aSelRange.aStart );
                    }
                }
            }
        }
    }

    return bFound;
}

void ScDBFunc::DateGroupDataPilot( const ScDPNumGroupInfo& rInfo, sal_Int32 nParts )
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                                        GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );

            ScDPSaveData aData( *pDPObj->GetSaveData() );
            ScDPDimensionSaveData* pDimData = aData.GetDimensionData();     // created if not there

            // find original base
            String aBaseDimName = aDimName;
            if( const ScDPSaveGroupDimension* pBaseGroupDim = pDimData->GetNamedGroupDim( aDimName ) )
                aBaseDimName = pBaseGroupDim->GetSourceDimName();

            // remove all existing parts (the grouping is built completely new)

            /*  Remove numeric group dimension (exists once at most). No need
                to delete anything in save data (grouping was done inplace in
                an existing base dimension). */
            pDimData->RemoveNumGroupDimension( aBaseDimName );

            /*  Remove named group dimension(s). Collect deleted dimension
                names which may be reused while recreating the groups.
                Dimensions have to be removed from dimension save data and from
                save data too. */
            std::vector< String > aDeletedNames;
            const ScDPSaveGroupDimension* pExistingGroup = pDimData->GetGroupDimForBase( aBaseDimName );
            while ( pExistingGroup )
            {
                String aGroupDimName = pExistingGroup->GetGroupDimName();
                pDimData->RemoveGroupDimension( aGroupDimName );     // pExistingGroup is deleted

                // also remove SaveData settings for the dimension that no longer exists
                aData.RemoveDimensionByName( aGroupDimName );

                /*  The name can be used for the new group dimensions, although
                    it is still in use with the DataPilotSource. */
                aDeletedNames.push_back( aGroupDimName );

                // see if there are more group dimensions
                pExistingGroup = pDimData->GetGroupDimForBase( aBaseDimName );

                if ( pExistingGroup && pExistingGroup->GetGroupDimName() == aGroupDimName )
                {
                    // still get the same group dimension?
                    DBG_ERROR("couldn't remove group dimension");
                    pExistingGroup = NULL;      // avoid endless loop
                }
            }

            if ( nParts )
            {
                // create date group dimensions

                ScDPNumGroupInfo aEmpty;
                bool bFirst = true;
                sal_Int32 nMask = 1;
                for (USHORT nBit=0; nBit<32; nBit++)
                {
                    if ( nParts & nMask )
                    {
                        if ( bFirst )
                        {
                            // innermost part: create NumGroupDimension (replacing original values)
                            // Dimension name is left unchanged

                            if ( (nParts == sheet::DataPilotFieldGroupBy::DAYS) && (rInfo.Step >= 1.0) )
                            {
                                // only days, and a step value specified: use numerical grouping
                                // with DateValues flag, not date grouping

                                ScDPNumGroupInfo aNumInfo( rInfo );
                                aNumInfo.DateValues = sal_True;

                                ScDPSaveNumGroupDimension aNumGroupDim( aBaseDimName, aNumInfo );
                                pDimData->AddNumGroupDimension( aNumGroupDim );
                            }
                            else
                            {
                                ScDPSaveNumGroupDimension aNumGroupDim( aBaseDimName, rInfo, nMask );
                                pDimData->AddNumGroupDimension( aNumGroupDim );
                            }

                            bFirst = false;
                        }
                        else
                        {
                            // additional parts: create GroupDimension (shown as additional dimensions)
                            String aGroupDimName = pDimData->CreateDateGroupDimName( nMask, *pDPObj, true, &aDeletedNames );
                            ScDPSaveGroupDimension aGroupDim( aBaseDimName, aGroupDimName );
                            aGroupDim.SetDateInfo( rInfo, nMask );
                            pDimData->AddGroupDimension( aGroupDim );

                            // set orientation
                            ScDPSaveDimension* pSaveDimension = aData.GetDimensionByName( aGroupDimName );
                            if ( pSaveDimension->GetOrientation() == sheet::DataPilotFieldOrientation_HIDDEN )
                            {
                                ScDPSaveDimension* pOldDimension = aData.GetDimensionByName( aBaseDimName );
                                pSaveDimension->SetOrientation( pOldDimension->GetOrientation() );
                                long nPosition = 0;     //! before (immediate) base
                                aData.SetPosition( pSaveDimension, nPosition );
                            }
                        }
                    }
                    nMask *= 2;
                }
            }

            // apply changes
            ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
            ScDPObject* pNewObj = new ScDPObject( *pDPObj );
            pNewObj->SetSaveData( aData );
            aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
            delete pNewObj;

            // unmark cell selection
            Unmark();
        }
    }
}

void ScDBFunc::NumGroupDataPilot( const ScDPNumGroupInfo& rInfo )
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                                        GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );

            ScDPSaveData aData( *pDPObj->GetSaveData() );
            ScDPDimensionSaveData* pDimData = aData.GetDimensionData();     // created if not there

            ScDPSaveNumGroupDimension* pExisting = pDimData->GetNumGroupDimAcc( aDimName );
            if ( pExisting )
            {
                // modify existing group dimension
                pExisting->SetGroupInfo( rInfo );
            }
            else
            {
                // create new group dimension
                ScDPSaveNumGroupDimension aNumGroupDim( aDimName, rInfo );
                pDimData->AddNumGroupDimension( aNumGroupDim );
            }

            // apply changes
            ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
            ScDPObject* pNewObj = new ScDPObject( *pDPObj );
            pNewObj->SetSaveData( aData );
            aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
            delete pNewObj;

            // unmark cell selection
            Unmark();
        }
    }
}

void ScDBFunc::GroupDataPilot()
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                                        GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );

            ScDPSaveData aData( *pDPObj->GetSaveData() );
            ScDPDimensionSaveData* pDimData = aData.GetDimensionData();     // created if not there

            // find original base
            String aBaseDimName( aDimName );
            const ScDPSaveGroupDimension* pBaseGroupDim = pDimData->GetNamedGroupDim( aDimName );
            if ( pBaseGroupDim )
            {
                // any entry's SourceDimName is the original base
                aBaseDimName = pBaseGroupDim->GetSourceDimName();
            }

            // find existing group dimension
            // (using the selected dim, can be intermediate group dim)
            ScDPSaveGroupDimension* pGroupDimension = pDimData->GetGroupDimAccForBase( aDimName );

            // remove the selected items from their groups
            // (empty groups are removed, too)
            USHORT nEntryCount = aEntries.GetCount();
            USHORT nEntry;
            if ( pGroupDimension )
            {
                for (nEntry=0; nEntry<nEntryCount; nEntry++)
                {
                    String aEntryName = aEntries[nEntry]->GetString();
                    if ( pBaseGroupDim )
                    {
                        // for each selected (intermediate) group, remove all its items
                        // (same logic as for adding, below)
                        const ScDPSaveGroupItem* pBaseGroup = pBaseGroupDim->GetNamedGroup( aEntryName );
                        if ( pBaseGroup )
                            pBaseGroup->RemoveElementsFromGroups( *pGroupDimension );   // remove all elements
                        else
                            pGroupDimension->RemoveFromGroups( aEntryName );
                    }
                    else
                        pGroupDimension->RemoveFromGroups( aEntryName );
                }
            }

            ScDPSaveGroupDimension* pNewGroupDim = NULL;
            if ( !pGroupDimension )
            {
                // create a new group dimension
                String aGroupDimName = pDimData->CreateGroupDimName( aBaseDimName, *pDPObj, false, NULL );
                pNewGroupDim = new ScDPSaveGroupDimension( aBaseDimName, aGroupDimName );

                pGroupDimension = pNewGroupDim;     // make changes to the new dim if none existed

                if ( pBaseGroupDim )
                {
                    // If it's a higher-order group dimension, pre-allocate groups for all
                    // non-selected original groups, so the individual base members aren't
                    // used for automatic groups (this would make the original groups hard
                    // to find).
                    //! Also do this when removing groups?
                    //! Handle this case dynamically with automatic groups?

                    long nGroupCount = pBaseGroupDim->GetGroupCount();
                    for ( long nGroup = 0; nGroup < nGroupCount; nGroup++ )
                    {
                        const ScDPSaveGroupItem* pBaseGroup = pBaseGroupDim->GetGroupByIndex( nGroup );

                        StrData aStrData( pBaseGroup->GetGroupName() );
                        USHORT nCollIndex;
                        if ( !aEntries.Search( &aStrData, nCollIndex ) )    //! ignore case?
                        {
                            // add an additional group for each item that is not in the selection
                            ScDPSaveGroupItem aGroup( pBaseGroup->GetGroupName() );
                            aGroup.AddElementsFromGroup( *pBaseGroup );
                            pGroupDimension->AddGroupItem( aGroup );
                        }
                    }
                }
            }
            String aGroupDimName = pGroupDimension->GetGroupDimName();

            //! localized prefix string
            String aGroupName = pGroupDimension->CreateGroupName( String::CreateFromAscii("Group") );
            ScDPSaveGroupItem aGroup( aGroupName );
            for (nEntry=0; nEntry<nEntryCount; nEntry++)
            {
                String aEntryName = aEntries[nEntry]->GetString();
                if ( pBaseGroupDim )
                {
                    // for each selected (intermediate) group, add all its items
                    const ScDPSaveGroupItem* pBaseGroup = pBaseGroupDim->GetNamedGroup( aEntryName );
                    if ( pBaseGroup )
                        aGroup.AddElementsFromGroup( *pBaseGroup );
                    else
                        aGroup.AddElement( aEntryName );    // no group found -> automatic group, add the item itself
                }
                else
                    aGroup.AddElement( aEntryName );        // no group dimension, add all items directly
            }

            pGroupDimension->AddGroupItem( aGroup );

            if ( pNewGroupDim )
            {
                pDimData->AddGroupDimension( *pNewGroupDim );
                delete pNewGroupDim;        // AddGroupDimension copies the object
                // don't access pGroupDimension after here
            }
            pGroupDimension = pNewGroupDim = NULL;

            // set orientation
            ScDPSaveDimension* pSaveDimension = aData.GetDimensionByName( aGroupDimName );
            if ( pSaveDimension->GetOrientation() == sheet::DataPilotFieldOrientation_HIDDEN )
            {
                ScDPSaveDimension* pOldDimension = aData.GetDimensionByName( aDimName );
                pSaveDimension->SetOrientation( pOldDimension->GetOrientation() );
                long nPosition = 0;     //! before (immediate) base
                aData.SetPosition( pSaveDimension, nPosition );
            }

            // apply changes
            ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
            ScDPObject* pNewObj = new ScDPObject( *pDPObj );
            pNewObj->SetSaveData( aData );
            aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
            delete pNewObj;

            // unmark cell selection
            Unmark();
        }
    }
}

void ScDBFunc::UngroupDataPilot()
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                                        GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );

            ScDPSaveData aData( *pDPObj->GetSaveData() );
            ScDPDimensionSaveData* pDimData = aData.GetDimensionData();     // created if not there
            //! test first if DimensionData exists?

            BOOL bApply = FALSE;

            ScDPSaveGroupDimension* pGroupDim = pDimData->GetNamedGroupDimAcc( aDimName );
            const ScDPSaveNumGroupDimension* pNumGroupDim = pDimData->GetNumGroupDim( aDimName );
            if ( ( pGroupDim && pGroupDim->GetDatePart() != 0 ) ||
                 ( pNumGroupDim && pNumGroupDim->GetDatePart() != 0 ) )
            {
                // Date grouping: need to remove all affected group dimensions.
                // This is done using DateGroupDataPilot with nParts=0.

                DateGroupDataPilot( ScDPNumGroupInfo(), 0 );
                // bApply remains FALSE
                // dimension pointers become invalid
            }
            else if ( pGroupDim )
            {
                USHORT nEntryCount = aEntries.GetCount();
                for (USHORT nEntry=0; nEntry<nEntryCount; nEntry++)
                {
                    String aEntryName = aEntries[nEntry]->GetString();
                    pGroupDim->RemoveGroup( aEntryName );
                }
                // remove group dimension if empty
                bool bEmptyDim = pGroupDim->IsEmpty();
                if ( !bEmptyDim )
                {
                    // If all remaining groups in the dimension aren't shown, remove
                    // the dimension too, as if it was completely empty.
                    ScStrCollection aVisibleEntries;
                    pDPObj->GetMemberResultNames( aVisibleEntries, nSelectDimension );
                    bEmptyDim = pGroupDim->HasOnlyHidden( aVisibleEntries );
                }
                if ( bEmptyDim )
                {
                    pDimData->RemoveGroupDimension( aDimName );     // pGroupDim is deleted

                    // also remove SaveData settings for the dimension that no longer exists
                    aData.RemoveDimensionByName( aDimName );
                }
                bApply = TRUE;
            }
            else if ( pNumGroupDim )
            {
                // remove the numerical grouping
                pDimData->RemoveNumGroupDimension( aDimName );
                // SaveData settings can remain unchanged - the same dimension still exists
                bApply = TRUE;
            }

            if ( bApply )
            {
                // apply changes
                ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
                ScDPObject* pNewObj = new ScDPObject( *pDPObj );
                pNewObj->SetSaveData( aData );
                aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
                delete pNewObj;

                // unmark cell selection
                Unmark();
            }
        }
    }
}

void ScDBFunc::DataPilotInput( const ScAddress& rPos, const String& rString )
{
    String aNewName( rString );

    ScDocument* pDoc = GetViewData()->GetDocument();
    ScDPObject* pDPObj = pDoc->GetDPAtCursor( rPos.Col(), rPos.Row(), rPos.Tab() );
    if ( pDPObj )
    {
        String aOldText;
        pDoc->GetString( rPos.Col(), rPos.Row(), rPos.Tab(), aOldText );

        if ( aOldText == rString )
        {
            // nothing to do: silently exit
            return;
        }

        USHORT nErrorId = 0;

        ScDPSaveData aData( *pDPObj->GetSaveData() );
        BOOL bChange = FALSE;

        USHORT nOrient = sheet::DataPilotFieldOrientation_HIDDEN;
        long nField = pDPObj->GetHeaderDim( rPos, nOrient );
        if ( nField >= 0 )
        {
            // changing a field title

            if ( aData.GetExistingDimensionData() )
            {
                // only group dimensions can be renamed

                ScDPDimensionSaveData* pDimData = aData.GetDimensionData();
                ScDPSaveGroupDimension* pGroupDim = pDimData->GetNamedGroupDimAcc( aOldText );
                if ( pGroupDim )
                {
                    // valid name: not empty, no existing dimension (group or other)
                    if ( aNewName.Len() && !pDPObj->IsDimNameInUse( aNewName ) )
                    {
                        pGroupDim->Rename( aNewName );

                        // also rename in SaveData to preserve the field settings
                        ScDPSaveDimension* pSaveDim = aData.GetDimensionByName( aOldText );
                        pSaveDim->SetName( aNewName );

                        bChange = TRUE;
                    }
                    else
                        nErrorId = STR_INVALIDNAME;
                }
            }
        }
        else
        {
            // renaming a group (item)?
            // allow only on the item name itself - not on empty cells, not on subtotals

            sheet::DataPilotTableHeaderData aPosData;
            pDPObj->GetHeaderPositionData(rPos, aPosData);
            if ( ( aPosData.Flags & sheet::MemberResultFlags::HASMEMBER ) &&
                 ! ( aPosData.Flags & sheet::MemberResultFlags::SUBTOTAL ) &&
                 aOldText.Len() )
            {
                if ( aData.GetExistingDimensionData() )
                {
                    BOOL bIsDataLayout;
                    String aDimName = pDPObj->GetDimName( aPosData.Dimension, bIsDataLayout );

                    ScDPDimensionSaveData* pDimData = aData.GetDimensionData();
                    ScDPSaveGroupDimension* pGroupDim = pDimData->GetNamedGroupDimAcc( aDimName );
                    if ( pGroupDim )
                    {
                        // valid name: not empty, no existing group in this dimension
                        //! ignore case?
                        if ( aNewName.Len() && !pGroupDim->GetNamedGroup( aNewName ) )
                        {
                            ScDPSaveGroupItem* pGroup = pGroupDim->GetNamedGroupAcc( aOldText );
                            if ( pGroup )
                                pGroup->Rename( aNewName );     // rename the existing group
                            else
                            {
                                // create a new group to replace the automatic group
                                ScDPSaveGroupItem aGroup( aNewName );
                                aGroup.AddElement( aOldText );
                                pGroupDim->AddGroupItem( aGroup );
                            }

                            // in both cases also adjust savedata, to preserve member settings (show details)
                            ScDPSaveDimension* pSaveDim = aData.GetDimensionByName( aDimName );
                            ScDPSaveMember* pSaveMember = pSaveDim->GetExistingMemberByName( aOldText );
                            if ( pSaveMember )
                                pSaveMember->SetName( aNewName );

                            bChange = TRUE;
                        }
                        else
                            nErrorId = STR_INVALIDNAME;
                    }
                }
            }
        }

        if ( bChange )
        {
            // apply changes
            ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
            ScDPObject* pNewObj = new ScDPObject( *pDPObj );
            pNewObj->SetSaveData( aData );
            aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
            delete pNewObj;
        }
        else
        {
            if ( !nErrorId )
                nErrorId = STR_ERR_DATAPILOT_INPUT;
            ErrorMessage( nErrorId );
        }
    }
}

void lcl_MoveToEnd( ScDPSaveDimension& rDim, const String& rItemName )
{
    ScDPSaveMember* pNewMember = NULL;
    const ScDPSaveMember* pOldMember = rDim.GetExistingMemberByName( rItemName );
    if ( pOldMember )
        pNewMember = new ScDPSaveMember( *pOldMember );
    else
        pNewMember = new ScDPSaveMember( rItemName );
    rDim.AddMember( pNewMember );
    // AddMember takes ownership of the new pointer,
    // puts it to the end of the list even if it was in the list before.
}

BOOL ScDBFunc::DataPilotMove( const ScRange& rSource, const ScAddress& rDest )
{
    BOOL bRet = FALSE;
    ScDocument* pDoc = GetViewData()->GetDocument();
    ScDPObject* pDPObj = pDoc->GetDPAtCursor( rSource.aStart.Col(), rSource.aStart.Row(), rSource.aStart.Tab() );
    if ( pDPObj && pDPObj == pDoc->GetDPAtCursor( rDest.Col(), rDest.Row(), rDest.Tab() ) )
    {
        sheet::DataPilotTableHeaderData aDestData;
        pDPObj->GetHeaderPositionData( rDest, aDestData );
        bool bValid = ( aDestData.Dimension >= 0 );        // dropping onto a field

        // look through the source range
        std::hash_set< rtl::OUString, rtl::OUStringHash, std::equal_to<rtl::OUString> > aMembersSet;   // for lookup
        std::vector< rtl::OUString > aMembersVector;  // members in original order, for inserting
        aMembersVector.reserve( std::max( static_cast<SCSIZE>( rSource.aEnd.Col() - rSource.aStart.Col() + 1 ),
                                          static_cast<SCSIZE>( rSource.aEnd.Row() - rSource.aStart.Row() + 1 ) ) );
        for (SCROW nRow = rSource.aStart.Row(); bValid && nRow <= rSource.aEnd.Row(); ++nRow )
            for (SCCOL nCol = rSource.aStart.Col(); bValid && nCol <= rSource.aEnd.Col(); ++nCol )
            {
                sheet::DataPilotTableHeaderData aSourceData;
                pDPObj->GetHeaderPositionData( ScAddress( nCol, nRow, rSource.aStart.Tab() ), aSourceData );
                if ( aSourceData.Dimension == aDestData.Dimension && aSourceData.MemberName.getLength() )
                {
                    if ( aMembersSet.find( aSourceData.MemberName ) == aMembersSet.end() )
                    {
                        aMembersSet.insert( aSourceData.MemberName );
                        aMembersVector.push_back( aSourceData.MemberName );
                    }
                    // duplicates are ignored
                }
                else
                    bValid = false;     // empty (subtotal) or different field
            }

        if ( bValid )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( aDestData.Dimension, bIsDataLayout );
            if ( !bIsDataLayout )
            {
                ScDPSaveData aData( *pDPObj->GetSaveData() );
                ScDPSaveDimension* pDim = aData.GetDimensionByName( aDimName );

                // get all member names in source order
                uno::Sequence<rtl::OUString> aMemberNames;
                pDPObj->GetMembers( aDestData.Dimension, aMemberNames );

                bool bInserted = false;

                sal_Int32 nMemberCount = aMemberNames.getLength();
                for (sal_Int32 nMemberPos=0; nMemberPos<nMemberCount; ++nMemberPos)
                {
                    String aMemberStr( aMemberNames[nMemberPos] );

                    if ( !bInserted && aMemberNames[nMemberPos] == aDestData.MemberName )
                    {
                        // insert dragged items before this item
                        for ( std::vector<rtl::OUString>::const_iterator aIter = aMembersVector.begin();
                              aIter != aMembersVector.end(); ++aIter )
                            lcl_MoveToEnd( *pDim, *aIter );
                        bInserted = true;
                    }

                    if ( aMembersSet.find( aMemberStr ) == aMembersSet.end() )  // skip dragged items
                        lcl_MoveToEnd( *pDim, aMemberStr );
                }
                // insert dragged item at end if dest wasn't found (for example, empty)
                if ( !bInserted )
                    for ( std::vector<rtl::OUString>::const_iterator aIter = aMembersVector.begin();
                          aIter != aMembersVector.end(); ++aIter )
                        lcl_MoveToEnd( *pDim, *aIter );

                // Items that were in SaveData, but not in the source, end up at the start of the list.

                // set flag for manual sorting
                sheet::DataPilotFieldSortInfo aSortInfo;
                aSortInfo.Mode = sheet::DataPilotFieldSortMode::MANUAL;
                pDim->SetSortInfo( &aSortInfo );

                // apply changes
                ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
                ScDPObject* pNewObj = new ScDPObject( *pDPObj );
                pNewObj->SetSaveData( aData );
                aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );      //! bApi for drag&drop?
                delete pNewObj;

                Unmark();       // entry was moved - no use in leaving the old cell selected

                bRet = TRUE;
            }
        }
    }

    return bRet;
}

BOOL ScDBFunc::HasSelectionForDrillDown( USHORT& rOrientation )
{
    BOOL bRet = FALSE;

    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
            							GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );
            if ( !bIsDataLayout )
            {
                ScDPSaveData* pSaveData = pDPObj->GetSaveData();
            	ScDPSaveDimension* pDim = pSaveData->GetExistingDimensionByName( aDimName );
            	if ( pDim )
            	{
            	    USHORT nDimOrient = pDim->GetOrientation();
            	    ScDPSaveDimension* pInner = pSaveData->GetInnermostDimension( nDimOrient );
            	    if ( pDim == pInner )
            	    {
            	        rOrientation = nDimOrient;
            	        bRet = TRUE;
            	    }
            	}
            }
        }
    }

    return bRet;
}

void ScDBFunc::SetDataPilotDetails( BOOL bShow, const String* pNewDimensionName )
{
    ScDPObject* pDPObj = GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
            							GetViewData()->GetCurY(), GetViewData()->GetTabNo() );
    if ( pDPObj )
    {
        ScStrCollection aEntries;
        long nSelectDimension = -1;
        GetSelectedMemberList( aEntries, nSelectDimension );

        if ( aEntries.GetCount() > 0 )
        {
            BOOL bIsDataLayout;
            String aDimName = pDPObj->GetDimName( nSelectDimension, bIsDataLayout );
            if ( !bIsDataLayout )
            {
                ScDPSaveData aData( *pDPObj->GetSaveData() );
                ScDPSaveDimension* pDim = aData.GetDimensionByName( aDimName );

                if ( bShow && pNewDimensionName )
                {
                    //  add the new dimension with the same orientation, at the end

                    ScDPSaveDimension* pNewDim = aData.GetDimensionByName( *pNewDimensionName );
                    ScDPSaveDimension* pDuplicated = NULL;
                    if ( pNewDim->GetOrientation() == sheet::DataPilotFieldOrientation_DATA )
                    {
                        // Need to duplicate the dimension, create column/row in addition to data:
                        // The duplicated dimension inherits the existing settings, pNewDim is modified below.
                        pDuplicated = aData.DuplicateDimension( *pNewDimensionName );
                    }

                    USHORT nOrientation = pDim->GetOrientation();
                    pNewDim->SetOrientation( nOrientation );

                    long nPosition = LONG_MAX;
                    aData.SetPosition( pNewDim, nPosition );

                    ScDPSaveDimension* pDataLayout = aData.GetDataLayoutDimension();
                    if ( pDataLayout->GetOrientation() == nOrientation &&
                         aData.GetDataDimensionCount() <= 1 )
                    {
                        // If there is only one data dimension, the data layout dimension
                        // must still be the last one in its orientation.
                        aData.SetPosition( pDataLayout, nPosition );
                    }

                    if ( pDuplicated )
                    {
                        // The duplicated (data) dimension needs to be behind the original dimension
                        aData.SetPosition( pDuplicated, nPosition );
                    }

                    //  Hide details for all visible members (selected are changed below).
                    //! Use all members from source level instead (including non-visible)?

                    ScStrCollection aVisibleEntries;
                    pDPObj->GetMemberResultNames( aVisibleEntries, nSelectDimension );

                    USHORT nVisCount = aVisibleEntries.GetCount();
                    for (USHORT nVisPos=0; nVisPos<nVisCount; nVisPos++)
                    {
                        String aVisName = aVisibleEntries[nVisPos]->GetString();
                        ScDPSaveMember* pMember = pDim->GetMemberByName( aVisName );
                        pMember->SetShowDetails( FALSE );
                    }
                }

                USHORT nEntryCount = aEntries.GetCount();
                for (USHORT nEntry=0; nEntry<nEntryCount; nEntry++)
                {
                    String aEntryName = aEntries[nEntry]->GetString();
                    ScDPSaveMember* pMember = pDim->GetMemberByName( aEntryName );
                    pMember->SetShowDetails( bShow );
                }

                // apply changes
                ScDBDocFunc aFunc( *GetViewData()->GetDocShell() );
                ScDPObject* pNewObj = new ScDPObject( *pDPObj );
                pNewObj->SetSaveData( aData );
                aFunc.DataPilotUpdate( pDPObj, pNewObj, TRUE, FALSE );
                delete pNewObj;

                // unmark cell selection
                Unmark();
            }
        }
    }
}

void ScDBFunc::ShowDataPilotSourceData( ScDPObject& rDPObj, const Sequence<sheet::DataPilotFieldFilter>& rFilters )
{
    ScDocument* pDoc = GetViewData()->GetDocument();
    if (pDoc->GetDocumentShell()->IsReadOnly())
    {
        ErrorMessage(STR_READONLYERR);
        return;
    }

    Reference<sheet::XDimensionsSupplier> xDimSupplier = rDPObj.GetSource();
    Reference<container::XNameAccess> xDims = xDimSupplier->getDimensions();
    Reference<sheet::XDrillDownDataSupplier> xDDSupplier(xDimSupplier, UNO_QUERY);
    if (!xDDSupplier.is())
        return;

    Sequence< Sequence<Any> > aTabData = xDDSupplier->getDrillDownData(rFilters);
    sal_Int32 nRowSize = aTabData.getLength();
    if (nRowSize <= 1)
        // There is no data to show.  Bail out.
        return;

    sal_Int32 nColSize = aTabData[0].getLength();

    SCTAB nNewTab = GetViewData()->GetTabNo();

    auto_ptr<ScDocument> pInsDoc(new ScDocument(SCDOCMODE_CLIP));
    pInsDoc->ResetClip( pDoc, nNewTab );
    for (SCROW nRow = 0; nRow < nRowSize; ++nRow)
    {
        for (SCCOL nCol = 0; nCol < nColSize; ++nCol)
        {
            const Any& rAny = aTabData[nRow][nCol];
            rtl::OUString aStr;
            double fVal;
            if (rAny >>= aStr)
                pInsDoc->PutCell( ScAddress(nCol, nRow, nNewTab), new ScStringCell(String(aStr)) );
            else if (rAny >>= fVal)
                pInsDoc->SetValue(nCol, nRow, nNewTab, fVal);
        }
    }

    // set number format (important for dates)
    for (SCCOL nCol = 0; nCol < nColSize; ++nCol)
    {
        rtl::OUString aStr;
        if (!(aTabData[0][nCol] >>= aStr))
            continue;

        Reference<XPropertySet> xPropSet(xDims->getByName(aStr), UNO_QUERY);
        if (!xPropSet.is())
            continue;

        Any any = xPropSet->getPropertyValue( rtl::OUString::createFromAscii(SC_UNO_NUMBERFO) );
        sal_Int32 nNumFmt = 0;
        if (!(any >>= nNumFmt))
            continue;

        ScPatternAttr aPattern( pInsDoc->GetPool() );
        aPattern.GetItemSet().Put( SfxUInt32Item(ATTR_VALUE_FORMAT, static_cast<UINT32>(nNumFmt)) );
        pInsDoc->ApplyPatternAreaTab(nCol, 1, nCol, nRowSize-1, nNewTab, aPattern);
    }

    SCCOL nEndCol = 0;
    SCROW nEndRow = 0;
    pInsDoc->GetCellArea( nNewTab, nEndCol, nEndRow );
    pInsDoc->SetClipArea( ScRange( 0, 0, nNewTab, nEndCol, nEndRow, nNewTab ) );

    SfxUndoManager* pMgr = GetViewData()->GetDocShell()->GetUndoManager();
    String aUndo = ScGlobal::GetRscString( STR_UNDO_DOOUTLINE );
    pMgr->EnterListAction( aUndo, aUndo );

    String aNewTabName;
    pDoc->CreateValidTabName(aNewTabName);
    if ( InsertTable(aNewTabName, nNewTab) )
        PasteFromClip( IDF_ALL, pInsDoc.get() );

    pMgr->LeaveListAction();
}

//
//			DB-Operationen (Sortieren, Filtern, Teilergebnisse) wiederholen
//

void ScDBFunc::RepeatDB( BOOL bRecord )
{
	SCCOL nCurX = GetViewData()->GetCurX();
	SCROW nCurY = GetViewData()->GetCurY();
	SCTAB nTab = GetViewData()->GetTabNo();
	ScDocument* pDoc = GetViewData()->GetDocument();
	ScDBData* pDBData = GetDBData();
	if (bRecord && !pDoc->IsUndoEnabled())
		bRecord = FALSE;

	ScQueryParam aQueryParam;
	pDBData->GetQueryParam( aQueryParam );
	BOOL bQuery = aQueryParam.GetEntry(0).bDoQuery;

	ScSortParam aSortParam;
	pDBData->GetSortParam( aSortParam );
	BOOL bSort = aSortParam.bDoSort[0];

	ScSubTotalParam aSubTotalParam;
	pDBData->GetSubTotalParam( aSubTotalParam );
	BOOL bSubTotal = aSubTotalParam.bGroupActive[0] && !aSubTotalParam.bRemoveOnly;

	if ( bQuery || bSort || bSubTotal )
	{
		BOOL bQuerySize = FALSE;
		ScRange aOldQuery;
		ScRange aNewQuery;
		if (bQuery && !aQueryParam.bInplace)
		{
			ScDBData* pDest = pDoc->GetDBAtCursor( aQueryParam.nDestCol, aQueryParam.nDestRow,
													aQueryParam.nDestTab, TRUE );
			if (pDest && pDest->IsDoSize())
			{
				pDest->GetArea( aOldQuery );
				bQuerySize = TRUE;
			}
		}

		SCTAB nDummy;
		SCCOL nStartCol;
		SCROW nStartRow;
		SCCOL nEndCol;
		SCROW nEndRow;
		pDBData->GetArea( nDummy, nStartCol, nStartRow, nEndCol, nEndRow );

		//!		Undo nur benoetigte Daten ?

		ScDocument* pUndoDoc = NULL;
		ScOutlineTable* pUndoTab = NULL;
		ScRangeName* pUndoRange = NULL;
		ScDBCollection* pUndoDB = NULL;

		if (bRecord)
		{
			SCTAB nTabCount = pDoc->GetTableCount();
			pUndoDoc = new ScDocument( SCDOCMODE_UNDO );
			ScOutlineTable* pTable = pDoc->GetOutlineTable( nTab );
			if (pTable)
			{
				pUndoTab = new ScOutlineTable( *pTable );

				SCCOLROW nOutStartCol;							// Zeilen/Spaltenstatus
				SCCOLROW nOutStartRow;
				SCCOLROW nOutEndCol;
				SCCOLROW nOutEndRow;
				pTable->GetColArray()->GetRange( nOutStartCol, nOutEndCol );
				pTable->GetRowArray()->GetRange( nOutStartRow, nOutEndRow );

				pUndoDoc->InitUndo( pDoc, nTab, nTab, TRUE, TRUE );
				pDoc->CopyToDocument( static_cast<SCCOL>(nOutStartCol), 0, nTab, static_cast<SCCOL>(nOutEndCol), MAXROW, nTab, IDF_NONE, FALSE, pUndoDoc );
				pDoc->CopyToDocument( 0, nOutStartRow, nTab, MAXCOL, nOutEndRow, nTab, IDF_NONE, FALSE, pUndoDoc );
			}
			else
				pUndoDoc->InitUndo( pDoc, nTab, nTab, FALSE, TRUE );

			//	Datenbereich sichern - incl. Filter-Ergebnis
			pDoc->CopyToDocument( 0,nStartRow,nTab, MAXCOL,nEndRow,nTab, IDF_ALL, FALSE, pUndoDoc );

			//	alle Formeln wegen Referenzen
			pDoc->CopyToDocument( 0,0,0, MAXCOL,MAXROW,nTabCount-1, IDF_FORMULA, FALSE, pUndoDoc );

			//	DB- und andere Bereiche
			ScRangeName* pDocRange = pDoc->GetRangeName();
			if (pDocRange->GetCount())
				pUndoRange = new ScRangeName( *pDocRange );
			ScDBCollection* pDocDB = pDoc->GetDBCollection();
			if (pDocDB->GetCount())
				pUndoDB = new ScDBCollection( *pDocDB );
		}

		if (bSort && bSubTotal)
		{
			//	Sortieren ohne SubTotals

			aSubTotalParam.bRemoveOnly = TRUE;		// wird unten wieder zurueckgesetzt
			DoSubTotals( aSubTotalParam, FALSE );
		}

		if (bSort)
		{
			pDBData->GetSortParam( aSortParam );			// Bereich kann sich geaendert haben
			Sort( aSortParam, FALSE, FALSE);
		}
		if (bQuery)
		{
			pDBData->GetQueryParam( aQueryParam );			// Bereich kann sich geaendert haben
			ScRange aAdvSource;
			if (pDBData->GetAdvancedQuerySource(aAdvSource))
			{
				pDoc->CreateQueryParam(
					aAdvSource.aStart.Col(), aAdvSource.aStart.Row(),
					aAdvSource.aEnd.Col(), aAdvSource.aEnd.Row(),
					aAdvSource.aStart.Tab(), aQueryParam );
				Query( aQueryParam, &aAdvSource, FALSE );
			}				
			else
				Query( aQueryParam, NULL, FALSE );

			//	bei nicht-inplace kann die Tabelle umgestellt worden sein
			if ( !aQueryParam.bInplace && aQueryParam.nDestTab != nTab )
				SetTabNo( nTab );
		}
		if (bSubTotal)
		{
			pDBData->GetSubTotalParam( aSubTotalParam );	// Bereich kann sich geaendert haben
			aSubTotalParam.bRemoveOnly = FALSE;
			DoSubTotals( aSubTotalParam, FALSE );
		}

		if (bRecord)
		{
			SCTAB nDummyTab;
            SCCOL nDummyCol;
			SCROW nDummyRow, nNewEndRow;
			pDBData->GetArea( nDummyTab, nDummyCol,nDummyRow, nDummyCol,nNewEndRow );

			const ScRange* pOld = NULL;
			const ScRange* pNew = NULL;
			if (bQuerySize)
			{
				ScDBData* pDest = pDoc->GetDBAtCursor( aQueryParam.nDestCol, aQueryParam.nDestRow,
														aQueryParam.nDestTab, TRUE );
				if (pDest)
				{
					pDest->GetArea( aNewQuery );
					pOld = &aOldQuery;
					pNew = &aNewQuery;
				}
			}

			GetViewData()->GetDocShell()->GetUndoManager()->AddUndoAction(
				new ScUndoRepeatDB( GetViewData()->GetDocShell(), nTab,
										nStartCol, nStartRow, nEndCol, nEndRow,
										nNewEndRow,
										nCurX, nCurY,
										pUndoDoc, pUndoTab,
										pUndoRange, pUndoDB,
										pOld, pNew ) );
		}

		GetViewData()->GetDocShell()->PostPaint( 0,0,nTab, MAXCOL,MAXROW,nTab,
													PAINT_GRID | PAINT_LEFT | PAINT_TOP | PAINT_SIZE );
	}
	else		// "Keine Operationen auszufuehren"
		ErrorMessage(STR_MSSG_REPEATDB_0);
}




